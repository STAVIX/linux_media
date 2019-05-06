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

#ifndef _STAVIX_REGS_H_
#define _STAVIX_REGS_H_
#include <linux/bitops.h> 
#include <linux/iopoll.h> 

#define STAVIX_PCIE_BASE		0x00000000
#define AXI_PCIE_SG_ADDR    0x10000000   
#define AXI_PCIE_DATA_ADDR  0x20000000   
#define AXIBAR2PCIEBAR_0U   0x208        
#define AXIBAR2PCIEBAR_0L   0x20C        
#define AXIBAR2PCIEBAR_1U   0x210       
#define AXIBAR2PCIEBAR_1L   0x214       


#define STAVIX_GPIO_BASE	0x00010000
#define STAVIX_RESETN_BASE2	0x00050000

					
#define STAVIX_GPIO_PIN(_bank, _pin)	BIT(_bank*2+_pin) 
#define STAVIX_GPIO_DATA	0x00
#define STAVIX_GPIO_TRI	0x04	   


#define STAVIX_I2C_BASE	 	  (0x00020000)
#define STAVIX_I2C_REG_OFFSET  (0x100)
#define STAVIX_CR_REG_OFFSET   (0x00+STAVIX_I2C_REG_OFFSET)	
#define STAVIX_SR_REG_OFFSET   (0x04+STAVIX_I2C_REG_OFFSET)	
#define STAVIX_DTR_REG_OFFSET  (0x08+STAVIX_I2C_REG_OFFSET)	
#define STAVIX_DRR_REG_OFFSET  (0x0C+STAVIX_I2C_REG_OFFSET)	
#define STAVIX_ADR_REG_OFFSET  (0x10+STAVIX_I2C_REG_OFFSET)	
#define STAVIX_TFO_REG_OFFSET  (0x14+STAVIX_I2C_REG_OFFSET)	
#define STAVIX_RFO_REG_OFFSET  (0x18+STAVIX_I2C_REG_OFFSET)	
#define STAVIX_TBA_REG_OFFSET  (0x1C+STAVIX_I2C_REG_OFFSET)	
#define STAVIX_RFD_REG_OFFSET  (0x20+STAVIX_I2C_REG_OFFSET)	
#define STAVIX_GPO_REG_OFFSET  (0x24+STAVIX_I2C_REG_OFFSET)	
#define STAVIX_TSUSTA_REG_OFFSET  (0x28+STAVIX_I2C_REG_OFFSET)
#define STAVIX_TSUSTO_REG_OFFSET  (0x2C+STAVIX_I2C_REG_OFFSET)
#define STAVIX_THDSTA_REG_OFFSET  (0x30+STAVIX_I2C_REG_OFFSET)
#define STAVIX_TSUDAT_REG_OFFSET  (0x34+STAVIX_I2C_REG_OFFSET)
#define STAVIX_TBUF_REG_OFFSET    (0x38+STAVIX_I2C_REG_OFFSET)
#define STAVIX_THIGH_REG_OFFSET   (0x3C+STAVIX_I2C_REG_OFFSET)
#define STAVIX_TLOW_REG_OFFSET    (0x40+STAVIX_I2C_REG_OFFSET)
#define STAVIX_THDAT_REG_OFFSET   (0x44+STAVIX_I2C_REG_OFFSET)

#define STAVIX_CR_ENABLE_DEVICE_MASK        0x01	
#define STAVIX_CR_TX_FIFO_RESET_MASK        0x02	
#define STAVIX_CR_MSMS_MASK                 0x04	
#define STAVIX_CR_DIR_IS_TX_MASK            0x08	
#define STAVIX_CR_NO_ACK_MASK               0x10	
#define STAVIX_CR_REPEATED_START_MASK       0x20	
#define STAVIX_CR_GENERAL_CALL_MASK         0x40	


#define STAVIX_SR_GEN_CALL_MASK             0x01	
#define STAVIX_SR_ADDR_AS_SLAVE_MASK        0x02	
#define STAVIX_SR_BUS_BUSY_MASK             0x04	
#define STAVIX_SR_MSTR_RDING_SLAVE_MASK     0x08	
#define STAVIX_SR_TX_FIFO_FULL_MASK         0x10	
#define STAVIX_SR_RX_FIFO_FULL_MASK         0x20	
#define STAVIX_SR_RX_FIFO_EMPTY_MASK        0x40	
#define STAVIX_SR_TX_FIFO_EMPTY_MASK        0x80	


#define STAVIX_INTR_ARB_LOST_MASK           0x01	
#define STAVIX_INTR_TX_ERROR_MASK           0x02	
#define STAVIX_INTR_TX_EMPTY_MASK           0x04	
#define STAVIX_INTR_RX_FULL_MASK            0x08	
#define STAVIX_INTR_BNB_MASK                0x10	
#define STAVIX_INTR_AAS_MASK                0x20	
#define STAVIX_INTR_NAAS_MASK               0x40	
#define STAVIX_INTR_TX_HALF_MASK            0x80	

#define IIC_RX_FIFO_DEPTH         16	
#define IIC_TX_FIFO_DEPTH         16	

#define STAVIX_TX_INTERRUPTS                           \
(STAVIX_INTR_TX_ERROR_MASK | STAVIX_INTR_TX_EMPTY_MASK | STAVIX_INTR_TX_HALF_MASK)

#define STAVIX_TX_RX_INTERRUPTS (STAVIX_INTR_RX_FULL_MASK | STAVIX_TX_INTERRUPTS)

#define STAVIX_READ_OPERATION  1
#define STAVIX_WRITE_OPERATION 0

#define STAVIX_TX_DYN_START_MASK            0x0100 
#define STAVIX_TX_DYN_STOP_MASK             0x0200 

#define STAVIX_DGIER_OFFSET    0x1C 
#define STAVIX_IISR_OFFSET     0x20 
#define STAVIX_IIER_OFFSET     0x28 
#define STAVIX_RESETR_OFFSET   0x40 

#define STAVIX_RESET_MASK             0xAUL

#define STAVIX_GINTR_ENABLE_MASK      0x80000000UL

#define SG_DMA_BASE  0x00030000
#define SG_DMA_REG_CONTROL			0x30
#define SG_DMA_REG_STATUS			0x34
#define SG_DMA_REG_CURDESC			0x38
#define SG_DMA_REG_TAILDESC			0x40


#define SG_DMA_MM2S_CTRL_OFFSET	0x00
#define SG_DMA_S2MM_CTRL_OFFSET	0x30

#define SG_DMA_CR_RUNSTOP_MASK		BIT(0)  
#define SG_DMA_CR_RESET_MASK		BIT(2)

#define SG_DMA_CR_DELAY_SHIFT		24
#define SG_DMA_CR_COALESCE_SHIFT	16
#define SG_DMA_CR_DELAY_MAX		GENMASK(31, 24)  
#define SG_DMA_CR_COALESCE_MAX	GENMASK(23, 16)
#define DELAY_NUM 255

#define SG_DMA_SR_HALTED_MASK	BIT(0)
#define SG_DMA_SR_IDLE_MASK		BIT(1) 

#define SG_DMA_XR_IRQ_IOC_MASK		BIT(12)
#define SG_DMA_XR_IRQ_DELAY_MASK	BIT(13)
#define SG_DMA_XR_IRQ_ERROR_MASK	BIT(14)
#define SG_DMA_XR_IRQ_ALL_MASK	GENMASK(14, 12)
#define SG_DMA_CYCLIC_MASK 		BIT(4)

#define SG_DMA_BD_STS_ALL_MASK	GENMASK(31, 28)
#define SG_DMA_BD_SOP		BIT(27)
#define SG_DMA_BD_EOP		BIT(26)

#define SG_DMA_MCRX_CDESC(x)	((x==0)? 0x38 : (0x70 + (x-1) * 0x20))  
#define SG_DMA_MCRX_TDESC(x)	((x==0)? 0x40 : (0x78 + (x-1) * 0x20)) 

#define SG_DMA_BD_HSIZE_MASK    GENMASK(15, 0)
#define SG_DMA_BD_STRIDE_MASK   GENMASK(15, 0)
#define SG_DMA_BD_VSIZE_MASK    GENMASK(31, 19)


#define SG_DMA_BD_BUFFER_LEN    GENMASK(22, 0)
#define SG_DMA_BD_SOF_EOF       GENMASK(27, 26)

#define SG_DMA_BD_STRIDE_SHIFT   0
#define SG_DMA_BD_VSIZE_SHIFT    19


#define SG_DMA_MAX_TRANS_LEN	GENMASK(22, 0) 

#define SG_DMA_LOOP_COUNT		1000000

#define SG_DMA_COALESCE_MAX		128 

#define s2mm_mcdmarx_control(axcache, aruser) \
			     ((aruser << 28) | (axcache << 24))

#define sg_dma_poll_timeout(dev, reg, val, cond, delay_us, timeout_us) \
	readl_poll_timeout_atomic(dev->lmmio + reg, val, \
			   cond, delay_us, timeout_us)

#define STAVIX_INT_BASE	0x00040000
#define STAVIX_INT_ISR	0x00
#define STAVIX_INT_IPR	0x04
#define STAVIX_INT_IER	0x08
#define STAVIX_INT_IAR	0x0C
#define STAVIX_INT_SIE	0x10
#define STAVIX_INT_CIE	0x14 
#define STAVIX_INT_IVR	0x18 
#define STAVIX_INT_MER	0x1C
#define STAVIX_INT_IMR	0x20

#endif
