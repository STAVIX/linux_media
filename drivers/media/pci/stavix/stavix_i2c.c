/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 only, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
 
#include "stavix.h"

#define xiic_tx_space(i2c) ((i2c)->tx_msg->len - (i2c)->tx_pos)
#define xiic_rx_space(i2c) ((i2c)->rx_msg->len - (i2c)->rx_pos)

static void __xiic_start_xfer(struct stavix_i2c *i2c);

static inline void xiic_irq_dis(struct stavix_i2c *i2c, u32 mask)
{
	struct stavix_dev *dev = i2c->dev;
	u32 ier = pci_read(STAVIX_I2C_BASE, STAVIX_IIER_OFFSET);
	pci_write(STAVIX_I2C_BASE, STAVIX_IIER_OFFSET, ier & ~mask);

}

static inline void xiic_irq_en(struct stavix_i2c *i2c, u32 mask)
{
	struct stavix_dev *dev = i2c->dev;
	u32 ier = pci_read(STAVIX_I2C_BASE, STAVIX_IIER_OFFSET);
	pci_write(STAVIX_I2C_BASE, STAVIX_IIER_OFFSET, ier | mask);
}

static inline void xiic_irq_clr(struct stavix_i2c *i2c, u32 mask)
{
	struct stavix_dev *dev = i2c->dev;
	u32 isr = pci_read(STAVIX_I2C_BASE, STAVIX_IISR_OFFSET);
	pci_write(STAVIX_I2C_BASE, STAVIX_IISR_OFFSET, isr & mask);
}

static inline void xiic_irq_clr_en(struct stavix_i2c *i2c, u32 mask)
{
	
	xiic_irq_clr(i2c, mask);
	xiic_irq_en(i2c, mask);
}

static void xiic_clear_rx_fifo(struct stavix_i2c *i2c)
{
	struct stavix_dev *dev = i2c->dev;
	u8 sr;
	for (sr = pci_read(STAVIX_I2C_BASE, STAVIX_SR_REG_OFFSET);
		!(sr & STAVIX_SR_RX_FIFO_EMPTY_MASK);
		sr = pci_read(STAVIX_I2C_BASE, STAVIX_SR_REG_OFFSET))
		{
			pci_read(STAVIX_I2C_BASE, STAVIX_DRR_REG_OFFSET);
		}
}

static void xiic_reinit(struct stavix_i2c *i2c)
{

	struct stavix_dev *dev = i2c->dev;


	pci_write(STAVIX_I2C_BASE, STAVIX_RESETR_OFFSET, STAVIX_RESET_MASK);

	pci_write(STAVIX_I2C_BASE, STAVIX_RFD_REG_OFFSET, IIC_RX_FIFO_DEPTH - 1);

	pci_write(STAVIX_I2C_BASE, STAVIX_CR_REG_OFFSET, STAVIX_CR_TX_FIFO_RESET_MASK);

	pci_write(STAVIX_I2C_BASE, STAVIX_CR_REG_OFFSET, STAVIX_CR_ENABLE_DEVICE_MASK);

	xiic_clear_rx_fifo(i2c);

	pci_write(STAVIX_I2C_BASE, STAVIX_DGIER_OFFSET, STAVIX_GINTR_ENABLE_MASK);

	xiic_irq_clr_en(i2c, STAVIX_INTR_ARB_LOST_MASK);
		
	pci_write(STAVIX_I2C_BASE,STAVIX_TSUSTA_REG_OFFSET,400);
	
	pci_write(STAVIX_I2C_BASE,STAVIX_TSUSTO_REG_OFFSET,400);
	
	pci_write(STAVIX_I2C_BASE,STAVIX_THDSTA_REG_OFFSET,400);
	
	pci_write(STAVIX_I2C_BASE,STAVIX_TSUDAT_REG_OFFSET,400);
	
	pci_write(STAVIX_I2C_BASE,STAVIX_TBUF_REG_OFFSET,400);
	
	pci_write(STAVIX_I2C_BASE,STAVIX_THIGH_REG_OFFSET,400);
	
	pci_write(STAVIX_I2C_BASE,STAVIX_TLOW_REG_OFFSET,400);
	
	pci_write(STAVIX_I2C_BASE,STAVIX_THDAT_REG_OFFSET,400);
}

static void xiic_deinit(struct stavix_i2c *i2c)
{
	struct stavix_dev *dev = i2c->dev;
	u8 cr;

	pci_write(STAVIX_I2C_BASE, STAVIX_RESETR_OFFSET, STAVIX_RESET_MASK);

	cr = pci_read(STAVIX_I2C_BASE, STAVIX_CR_REG_OFFSET);

	pci_write(STAVIX_I2C_BASE, STAVIX_CR_REG_OFFSET, cr & ~STAVIX_CR_ENABLE_DEVICE_MASK);

}

static void xiic_read_rx(struct stavix_i2c *i2c)
{
	struct stavix_dev *dev = i2c->dev;
	u8 bytes_in_fifo;
	int i;

	bytes_in_fifo = pci_read(STAVIX_I2C_BASE, STAVIX_RFO_REG_OFFSET) + 1;

	if (bytes_in_fifo > xiic_rx_space(i2c))
		bytes_in_fifo = xiic_rx_space(i2c);

	for (i = 0; i < bytes_in_fifo; i++){
		i2c->rx_msg->buf[i2c->rx_pos++] = pci_read(STAVIX_I2C_BASE, STAVIX_DRR_REG_OFFSET);

	}
	pci_write(STAVIX_I2C_BASE, STAVIX_RFD_REG_OFFSET,
		(xiic_rx_space(i2c) > IIC_RX_FIFO_DEPTH) ?
		IIC_RX_FIFO_DEPTH - 1 :  xiic_rx_space(i2c) - 1);

}

static int xiic_tx_fifo_space(struct stavix_i2c *i2c)
{
	struct stavix_dev *dev = i2c->dev;

	return IIC_TX_FIFO_DEPTH - pci_read(STAVIX_I2C_BASE, STAVIX_TFO_REG_OFFSET) - 1;
}

static void xiic_fill_tx_fifo(struct stavix_i2c *i2c)
{
	struct stavix_dev *dev = i2c->dev;

	u8 fifo_space = xiic_tx_fifo_space(i2c);
	int len = xiic_tx_space(i2c);

	len = (len > fifo_space) ? fifo_space : len;

	while (len--) {
		u16 data = i2c->tx_msg->buf[i2c->tx_pos++];
		if ((xiic_tx_space(i2c) == 0) && (i2c->nmsgs == 1)) {
			data |= STAVIX_TX_DYN_STOP_MASK;
		}
		pci_write(STAVIX_I2C_BASE, STAVIX_DTR_REG_OFFSET, data);

	}
}

static void xiic_wakeup(struct stavix_i2c *i2c, int code)
{
	i2c->tx_msg = NULL;
	i2c->rx_msg = NULL;
	i2c->nmsgs = 0;
	i2c->state = code;
	wake_up(&i2c->wq);
}


int xiic_irq_process(struct stavix_i2c *i2c)
{
	struct stavix_dev *dev = i2c->dev;
	u32 pend, isr, ier;
	u32 clr = 0;
	 
	mutex_lock(&i2c->lock);
	isr = pci_read(STAVIX_I2C_BASE, STAVIX_IISR_OFFSET);
	ier = pci_read(STAVIX_I2C_BASE, STAVIX_IIER_OFFSET);
	pend = isr & ier;

	if ((pend & STAVIX_INTR_ARB_LOST_MASK) ||
		((pend & STAVIX_INTR_TX_ERROR_MASK) &&
		!(pend & STAVIX_INTR_RX_FULL_MASK))) {

		xiic_reinit(i2c); 

		if (i2c->rx_msg)
			xiic_wakeup(i2c, STATE_ERROR);
		if (i2c->tx_msg)
			xiic_wakeup(i2c, STATE_ERROR);

	}
	if (pend & STAVIX_INTR_RX_FULL_MASK) {


		clr |= STAVIX_INTR_RX_FULL_MASK;
		if (!i2c->rx_msg) {
			xiic_clear_rx_fifo(i2c);
			goto out;
		}

		xiic_read_rx(i2c);
		if (xiic_rx_space(i2c) == 0) {

			i2c->rx_msg = NULL;

			clr |= (isr & STAVIX_INTR_TX_ERROR_MASK);

			if (i2c->nmsgs > 1) {
				i2c->nmsgs--;
				i2c->tx_msg++;
				__xiic_start_xfer(i2c);
			}
		}

	}
	if (pend & STAVIX_INTR_BNB_MASK) {

		clr |= STAVIX_INTR_BNB_MASK;


		xiic_irq_dis(i2c, STAVIX_INTR_BNB_MASK);

		if (!i2c->tx_msg)
			goto out;

		if ((i2c->nmsgs == 1) && !i2c->rx_msg &&
			xiic_tx_space(i2c) == 0)
			xiic_wakeup(i2c, STATE_DONE);
		else
			xiic_wakeup(i2c, STATE_ERROR);

	}
	if (pend & (STAVIX_INTR_TX_EMPTY_MASK | STAVIX_INTR_TX_HALF_MASK)) {

		clr |= (pend &
			(STAVIX_INTR_TX_EMPTY_MASK | STAVIX_INTR_TX_HALF_MASK));

		if (!i2c->tx_msg) {
			goto out;
		}

		xiic_fill_tx_fifo(i2c);


		if (!xiic_tx_space(i2c) && xiic_tx_fifo_space(i2c) >= 2) {

			if (i2c->nmsgs > 1) {
				i2c->nmsgs--;
				i2c->tx_msg++;
				__xiic_start_xfer(i2c);
			} else {
				xiic_irq_dis(i2c, STAVIX_INTR_TX_HALF_MASK);
			}
		} else if (!xiic_tx_space(i2c) && (i2c->nmsgs == 1)){
			xiic_irq_dis(i2c, STAVIX_INTR_TX_HALF_MASK);
			}

	}
out:
	pci_write(STAVIX_I2C_BASE, STAVIX_IISR_OFFSET, clr);
	mutex_unlock(&i2c->lock);
	return 0;
}


static int xiic_busy(struct stavix_i2c *i2c)
{
	struct stavix_dev *dev = i2c->dev;
	int tries = 3;
	int err = 1;
	u32 sr;

	if(i2c->tx_msg)
		return -EBUSY;
		
	sr = pci_read(STAVIX_I2C_BASE, STAVIX_SR_REG_OFFSET);
	err = (sr & STAVIX_SR_BUS_BUSY_MASK) ? -EBUSY : 0;
	while (err && tries--) {
		sr = pci_read(STAVIX_I2C_BASE, STAVIX_SR_REG_OFFSET);
		err = (sr & STAVIX_SR_BUS_BUSY_MASK) ? -EBUSY : 0;
		msleep(1);
	}
	return err;
}

static void xiic_start_recv(struct stavix_i2c *i2c)
{
	struct stavix_dev *dev = i2c->dev;
	u8 rx_watermark;
	struct i2c_msg *msg = i2c->rx_msg = i2c->tx_msg;


	xiic_irq_clr_en(i2c, STAVIX_INTR_RX_FULL_MASK | STAVIX_INTR_TX_ERROR_MASK);


	rx_watermark = msg->len;
	if (rx_watermark > IIC_RX_FIFO_DEPTH)
		rx_watermark = IIC_RX_FIFO_DEPTH;
	pci_write(STAVIX_I2C_BASE, STAVIX_RFD_REG_OFFSET, rx_watermark - 1);

	if (!(msg->flags & I2C_M_NOSTART))

		pci_write(STAVIX_I2C_BASE, STAVIX_DTR_REG_OFFSET,
			(msg->addr << 1) | STAVIX_READ_OPERATION |
			STAVIX_TX_DYN_START_MASK);

	xiic_irq_clr_en(i2c, STAVIX_INTR_BNB_MASK);

	pci_write(STAVIX_I2C_BASE, STAVIX_DTR_REG_OFFSET,
		msg->len | ((i2c->nmsgs == 1) ? STAVIX_TX_DYN_STOP_MASK : 0));

	if (i2c->nmsgs == 1)
		xiic_irq_clr_en(i2c, STAVIX_INTR_BNB_MASK);
	i2c->tx_pos = msg->len;
}

static void xiic_start_send(struct stavix_i2c *i2c)
{
	struct stavix_dev *dev = i2c->dev;
	struct i2c_msg *msg = i2c->tx_msg;

	xiic_irq_clr(i2c, STAVIX_INTR_TX_ERROR_MASK);

	if (!(msg->flags & I2C_M_NOSTART)) {
		u16 data = ((msg->addr << 1) & 0xfe) | STAVIX_WRITE_OPERATION |
			STAVIX_TX_DYN_START_MASK;
		if ((i2c->nmsgs == 1) && msg->len == 0)
			data |= STAVIX_TX_DYN_STOP_MASK;
		pci_write(STAVIX_I2C_BASE, STAVIX_DTR_REG_OFFSET, data);

	}

	xiic_fill_tx_fifo(i2c);

	xiic_irq_clr_en(i2c, STAVIX_INTR_TX_EMPTY_MASK | STAVIX_INTR_TX_ERROR_MASK |
		STAVIX_INTR_BNB_MASK);
}

static void __xiic_start_xfer(struct stavix_i2c *i2c)
{
	int first = 1;
	int fifo_space = xiic_tx_fifo_space(i2c);

	if (!i2c->tx_msg)
		return;

	i2c->rx_pos = 0;
	i2c->tx_pos = 0;
	i2c->state = STATE_START;
       while ((fifo_space >= 2) && (first || (i2c->nmsgs > 1))) {
                if (!first) {
                       i2c->nmsgs--;
                       i2c->tx_msg++;
                       i2c->tx_pos = 0;
                } else
                       first = 0;

                 if (i2c->tx_msg->flags & I2C_M_RD) {
                 
                         xiic_start_recv(i2c);
                        return;
                } else {
                        xiic_start_send(i2c);
                         if (xiic_tx_space(i2c) != 0) {
                               
                                break;
                        }
                 }

                 fifo_space = xiic_tx_fifo_space(i2c);
         }


	if (i2c->nmsgs > 1 || xiic_tx_space(i2c))
		xiic_irq_clr_en(i2c, STAVIX_INTR_TX_HALF_MASK);

}
#if 0
static void xiic_start_xfer(struct stavix_i2c *i2c)
{
	mutex_lock(&i2c->lock);
	xiic_reinit(i2c);
	__xiic_start_xfer(i2c);
	mutex_unlock(&i2c->lock);
}
#endif

static int xiic_xfer(struct i2c_adapter *i2c_adap, struct i2c_msg *msgs, int num)
{

	struct stavix_i2c *i2c = i2c_get_adapdata(i2c_adap);
	struct stavix_dev *dev = i2c->dev;
	int err;
	
	err = xiic_busy(i2c);
 	if (err){
		printk(KERN_INFO"xiic_busy!!\n");	
		goto out; 
	}
	i2c->tx_msg = msgs;
	i2c->nmsgs = num;
	mutex_lock(&i2c->lock); 
	xiic_reinit(i2c); 
	__xiic_start_xfer(i2c);
	mutex_unlock(&i2c->lock);
	if (wait_event_timeout(i2c->wq, (i2c->state == STATE_ERROR) || 
		(i2c->state == STATE_DONE), HZ)) { 
		err = (i2c->state == STATE_DONE) ? num : -EIO;
	} else {
		i2c->tx_msg = NULL;
		i2c->rx_msg = NULL;
		i2c->nmsgs = 0;
		err = -ETIMEDOUT;
		pr_err("xiic xfer i2c read error 2\n");

	}
	
out:
	return err;
}

static u32 i2c_functionality(struct i2c_adapter *i2c_adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

struct i2c_algorithm stavix_i2c_algo_template = {
	.master_xfer   = xiic_xfer,
	.functionality = i2c_functionality,
};

static int stavix_i2c_register(struct stavix_i2c *bus)
{
	struct stavix_dev *dev = bus->dev;
	struct i2c_adapter *i2c_adap;

	init_waitqueue_head(&bus->wq);
	mutex_init(&bus->lock);

	i2c_adap = &bus->i2c_adap;
	strcpy(i2c_adap->name, "stavix");
	i2c_adap->algo = &stavix_i2c_algo_template;
	i2c_adap->algo_data = (void*) bus;
	i2c_adap->dev.parent = &dev->pci_dev->dev;
	i2c_adap->owner = THIS_MODULE;
	i2c_set_adapdata(&bus->i2c_adap, bus);
	return i2c_add_adapter(&bus->i2c_adap);
}

static void stavix_i2c_unregister(struct stavix_i2c *bus)
{
	i2c_del_adapter(&bus->i2c_adap);
}

int stavix_i2c_init(struct stavix_dev *dev) 
{
	int ret = 0;

	dev->i2c_bus.dev = dev;
	ret = stavix_i2c_register(&dev->i2c_bus);
	if (ret) {
		stavix_i2c_unregister(&dev->i2c_bus);
		xiic_deinit(&dev->i2c_bus);//+++
	} else {
		xiic_reinit(&dev->i2c_bus); 
	}
	   
	return ret;
}

void stavix_i2c_exit(struct stavix_dev *dev)
{
	stavix_i2c_unregister(&dev->i2c_bus);
}
