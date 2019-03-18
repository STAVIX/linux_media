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
#include <linux/dma-mapping.h>
#include <linux/bitops.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "stavix.h" 

char *gDataBuffer = NULL;           
char *gBDBuffer = NULL; 
dma_addr_t gDataBufferHW;
dma_addr_t gBDBufferHW;

void sg_dma_reg_init(struct stavix_dev *dev) 
{
	u32 ctrl_reg;
	ctrl_reg = pci_read(SG_DMA_BASE, SG_DMA_REG_CONTROL);
	ctrl_reg &= ~SG_DMA_CR_COALESCE_MAX;
	ctrl_reg |= BD_NUM << SG_DMA_CR_COALESCE_SHIFT;
	ctrl_reg |= SG_DMA_XR_IRQ_ALL_MASK; 
	ctrl_reg |= SG_DMA_CYCLIC_MASK;
	pci_write(SG_DMA_BASE, SG_DMA_REG_CONTROL, ctrl_reg);
	
}

static int sg_dma_init_chan_bd(struct sg_dma_channel *chan) 
{
	int i;
	INIT_LIST_HEAD(&chan->free_seg_list);
	
	for (i = 0; i < SG_PACKETS; i++) {                                           
		chan->seg_v[i].hw.next_desc = AXI_PCIE_SG_ADDR + SEG_SIZE * ((i + 1) % (SG_PACKETS)) ;	
		
		chan->seg_v[i].hw.buf_addr = AXI_PCIE_DATA_ADDR + TS_PACKET_SIZE * i ;
		
		chan->seg_v[i].hw.control = TS_PACKET_SIZE & SG_DMA_BD_BUFFER_LEN; 
		chan->seg_v[i].hw.control |=  SG_DMA_BD_SOF_EOF; 
		chan->seg_v[i].hw.status &=  0x00000000; 
		list_add_tail(&chan->seg_v[i].node, &chan->free_seg_list); 
	}
	return 0;
	
}


static void sg_dma_start(struct sg_dma_channel *chan) 
{
	
	u32 val, ctrl_reg;

	struct stavix_dev *dev = chan->dev;
	
	ctrl_reg = pci_read(SG_DMA_BASE, SG_DMA_REG_CONTROL);
	ctrl_reg |= SG_DMA_CR_RUNSTOP_MASK;
	pci_write(SG_DMA_BASE, SG_DMA_REG_CONTROL, ctrl_reg);

	chan->err = sg_dma_poll_timeout(chan->dev, SG_DMA_REG_STATUS, val,
				      !(val & SG_DMA_SR_HALTED_MASK), 10,
				      SG_DMA_LOOP_COUNT);

}



static int sg_dma_channel_reset(struct stavix_dev *dev)
{
	int err = 0;
	u32 val, ctrl_reg;

	ctrl_reg = pci_read(SG_DMA_BASE, SG_DMA_REG_CONTROL);
	pci_write(SG_DMA_BASE, SG_DMA_REG_CONTROL, ctrl_reg |
		       SG_DMA_CR_RESET_MASK);

	err = sg_dma_poll_timeout(dev, SG_DMA_REG_CONTROL, val,
				      !(val & SG_DMA_CR_RESET_MASK), 10,
				      SG_DMA_LOOP_COUNT);

	if (err) {
		dev_err(&dev->pci_dev->dev, "reset timeout, cr %x, sr %x\n",
			pci_read(SG_DMA_BASE, SG_DMA_REG_CONTROL),
			pci_read(SG_DMA_BASE, SG_DMA_REG_STATUS));
		return -EBUSY;
	}

	return err;
}


static void replace_tasklet_schedule(struct stavix_dev *dev) 
{
	struct stavix_adapter *adapter = dev->adapter;
	
	u8* data;
	u8 tid,k;
	int i;
	u32 cdesc;
	
	spin_lock(&dev->adap_lock);		
	cdesc = pci_read(SG_DMA_BASE, SG_DMA_REG_CURDESC);
	k = (cdesc - AXI_PCIE_SG_ADDR)/SEG_SIZE/BD_NUM;
	adapter->dma.buf_cnt = k;

	if(k==0)
	k=0x04;
	else if(k==1)
	k=0x05;
	else if(k==2)
	k=0x06;
	else if(k==3)
	k=0x07;
	else 
	k=k-4;
	
	data = adapter->dma.buf[k];

	for(i = 0; i < TS_NUM; i++) {
		tid = data[0]&0x07;	
		data[0] = 0x47;	
		dvb_dmx_swfilter_packets(&(dev->adapter[tid].demux), data, 1);
		data =  data + 192;
	}
	spin_unlock(&dev->adap_lock);	
}


int sg_dma_irq_process(struct stavix_dev *dev, u32 status)
{	
	
 	struct stavix_adapter *adapter = dev->adapter;
	if (status & SG_DMA_XR_IRQ_IOC_MASK) {
		 
		replace_tasklet_schedule(dev);	
		
	}
	if(status & SG_DMA_XR_IRQ_ERROR_MASK) {
		
	spin_lock_irq(&dev->adapter->adap_lock);
	sg_dma_channel_reset(dev);
	sg_dma_init_chan_bd(&dev->adapter->dma);
	sg_dma_reg_init(dev); 
	
	pci_write(SG_DMA_BASE, SG_DMA_MCRX_CDESC(0), (AXI_PCIE_SG_ADDR + SEG_SIZE*(adapter->dma.buf_cnt)));	
	sg_dma_start(&dev->adapter->dma);
	pci_write(SG_DMA_BASE, SG_DMA_MCRX_TDESC(0), 0xFFFFFFFF);
	spin_unlock_irq(&dev->adapter->adap_lock); 
		
	}

	return 0;
		
}

void sg_dma_enable(struct stavix_adapter *adap) 
{
	u32 status, control;
	struct stavix_dev *dev;
	adap->dma.tasklet_on =true;
	dev = adap->dev;
	status = pci_read(SG_DMA_BASE, SG_DMA_REG_STATUS);
	control = pci_read(SG_DMA_BASE, SG_DMA_REG_CONTROL);
	
}

void sg_dma_disable(struct stavix_adapter *adap) 
{
	adap->dma.tasklet_on =false;

}

void sg_dma_enable_tasklet(struct stavix_adapter *adap) 
{
	struct sg_dma_tx_descriptor  *tail_desc;
	struct sg_dma_channel *chan1;
	struct stavix_dev *dev = adap->dev; 

	chan1 = &adap->dma;
	
	if (chan1->err)
		return;
	spin_lock_irq(&adap->adap_lock);


	tail_desc = list_last_entry(&chan1->free_seg_list,
				     struct sg_dma_tx_descriptor, node);
	
	pci_write(SG_DMA_BASE, SG_DMA_MCRX_CDESC(adap->nr), (tail_desc->hw.next_desc));

	sg_dma_start(chan1); 
	if (chan1->err)
		return;

	pci_write(SG_DMA_BASE, SG_DMA_MCRX_TDESC(adap->nr), 0xFFFFFFFF );
	spin_unlock_irq(&adap->adap_lock);

}




void sg_dma_free(struct stavix_dev *dev) 
{
	int i;

	pci_free_consistent(dev->pci_dev, 
			2<<20, gBDBuffer, gBDBufferHW);
	pci_free_consistent(dev->pci_dev, 
			SG_DMA_PAGE_SIZE, gDataBuffer, gDataBufferHW);
	gBDBuffer = NULL;
	gDataBuffer = NULL;
	
	for (i = 0; i < 8; i++) 
		INIT_LIST_HEAD(&dev->adapter[i].dma.free_seg_list);
	
}

static void axi_pci_trans(struct stavix_dev *dev)
{
	size_t pntr0, pntr1;
	
	pntr0 =  (size_t) (gBDBufferHW);
	pntr1 =  (size_t) (gDataBufferHW);	
    pci_write (STAVIX_PCIE_BASE, AXIBAR2PCIEBAR_0L, (pntr0 >> 0)  & 0xFFFFFFFF); 
    pci_write (STAVIX_PCIE_BASE, AXIBAR2PCIEBAR_0U, (pntr0 >> 32) & 0xFFFFFFFF); 
    pci_write (STAVIX_PCIE_BASE, AXIBAR2PCIEBAR_1L, (pntr1 >> 0)  & 0xFFFFFFFF); 
    pci_write (STAVIX_PCIE_BASE, AXIBAR2PCIEBAR_1U, (pntr1 >> 32) & 0xFFFFFFFF); 
	
}

int sg_dma_init(struct stavix_dev *dev) 
{
	struct stavix_adapter *adapter = dev->adapter; 
	int i, j;
	
	
	
	gDataBuffer = pci_alloc_consistent(dev->pci_dev, 
			SG_DMA_PAGE_SIZE, &gDataBufferHW); 
		
	
	gBDBuffer = pci_alloc_consistent(dev->pci_dev, 
			2<<20, &gBDBufferHW); 

	if (NULL == gDataBuffer || NULL == gBDBuffer)
		goto err;			
					  
	for (i = 0; i < dev->info->adapters; i++) {	
		adapter->dma.buf[0] = gDataBuffer;
		adapter->dma.cnt = 0;
		for (j = 1; j < SG_DMA_BUFFERS + 1; j++)
			adapter->dma.buf[j] = adapter->dma.buf[j-1] + SG_DMA_BUF_SIZE;
		
		adapter->dma.seg_v = (struct sg_dma_tx_descriptor *)gBDBuffer ; 	
		sg_dma_init_chan_bd(&adapter->dma);		
		adapter->dma.buf_cnt = 0; 
		adapter->dma.offset = 0; 
		adapter->dma.reach_tail = false; 
		adapter->dma.err = false; 
		spin_lock_init(&adapter->adap_lock);
		adapter++;
	}
	
	sg_dma_channel_reset(dev);
	sg_dma_reg_init(dev);
	spin_lock_init(&dev->adap_lock);
		
	axi_pci_trans(dev);

	return 0;
err:
	dev_err(&dev->pci_dev->dev, "dma: memory alloc failed\n");
	return -ENOMEM;
}

