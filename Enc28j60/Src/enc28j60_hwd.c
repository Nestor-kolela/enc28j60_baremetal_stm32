/*
 * enc28j60_hwd.c
 *
 *  Created on: February 8, 2025
 *      Author:  NK KALAMBAY
 */

#include <stdint.h>
#include <stdio.h>
#include "enc28j60_hwd.h"
#include "log.h"

static uint8_t const mac[6] = {0xED, 0x6A, 0xAF, 0x16, 0x7C, 0x04};

static void enc28j60_SoftReset(enc28j60Drv * dev);
static void enc28j60_writeReg(enc28j60Drv * dev, uint8_t u8Reg, uint8_t u8Value);
//static uint8_t enc28j60_readEtherReg(enc28j60Drv * dev, uint8_t u8Reg);
static uint8_t enc28j60_readMacMIIReg(enc28j60Drv * dev, uint8_t u8Reg);
//static void enc28j60_BitFieldSet(enc28j60Drv * dev, uint8_t u8Reg, uint8_t u8data);
void enc28j60_BitFieldClear(enc28j60Drv * dev, uint8_t u8Reg, uint8_t u8data);



static void enc28j60_writePhyReg(enc28j60Drv * dev, uint8_t addr, uint16_t data);
static uint16_t enc28j60_getPhyPartNumber(enc28j60Drv * dev);
//static uint8_t enc28j60_getPhyRevNumber(enc28j60Drv * dev);
static uint32_t enc28j60_getPhyIdentifier(enc28j60Drv * dev);
#if 0
static bool enc28j60_getPhylinkHasBeenDown(enc28j60Drv * dev);
static bool enc28j60_getPhyjabberStatusBit(enc28j60Drv * dev);
static bool enc28j60_getPhyPolarityStatus(enc28j60Drv * dev);
static bool enc28j60_getPhyDuplexStatus(enc28j60Drv * dev);
static bool enc28j60_getPhyLinkStatus(enc28j60Drv * dev);
static bool enc28j60_getPhyCollisionStatus(enc28j60Drv * dev);
static bool enc28j60_getPhyIsRxStatus(enc28j60Drv * dev);
static bool enc28j60_getPhyIsTxStatus(enc28j60Drv * dev);

#endif
static void enc2860_phyInit(enc28j60Drv * dev);
static void enc2860_macInit(enc28j60Drv * dev);
static void enc28j60_rxSetFilters(enc28j60Drv * dev, rx_filter_control filter);

static encj28j60_bank convertRegValToBank(uint8_t value);
static uint8_t convertBankToBits(encj28j60_bank bBank);

void enc28j60_initDr(enc28j60Drv * dev, spiChipSel cs, spiChipDSl dCS, slaveRead rd,
		slaveWrite wr, intHandler hdle, delayMs delay)
{
	dev->spi.fncPtrCS 		= cs;
	dev->spi.fncPtrChipDS 	= dCS;
	dev->spi.fncPtrRead 	= rd;
	dev->spi.fncPtrWrite 	= wr;
	dev->fncPtrcallBack 	= hdle;
	dev->fncPtrDelayFunc 	= delay;
}

void enc28j60_strtDr(enc28j60Drv * dev)
{
	enc28j60_sftRst(dev);

	dev->fncPtrDelayFunc(1);

	//Wait for clock to become stable
	while(!(enc28j60_readEtherReg(dev, dev->bank0.commonRegs.ESTAT) & (0x01))) continue;

	//Just for information, but no need for anything further.
	(void) enc28j60_getPhyRevNumber(dev);
	(void) enc28j60_getPhyPartNumber(dev);
	(void) enc28j60_getPhyIdentifier(dev);

	//Receive buffer = 6K meaning from 0x800 to 0x1FFF
	//Start address for RX
	enc28j60_writeReg(dev, dev->bank0.ERXSTL, (uint8_t) dev->rxBufStartAddr.u8ValLo);
	enc28j60_writeReg(dev, dev->bank0.ERXSTH, (uint8_t) dev->rxBufStartAddr.u8ValHi);

	//ERDPTL and ERDPTH point to the address where we are reading from
	enc28j60_writeReg(dev, dev->bank0.ERDPTL, (uint8_t) dev->rxPkt.ptrAddr.ptrLo);
	enc28j60_writeReg(dev, dev->bank0.ERDPTH, (uint8_t) dev->rxPkt.ptrAddr.ptrHi);

	//End address for RX
	enc28j60_writeReg(dev, dev->bank0.ERXNDL, (uint8_t) (dev->rxBufEndAddr.u8ValLo));
	enc28j60_writeReg(dev, dev->bank0.ERXNDH, (uint8_t) (dev->rxBufEndAddr.u8ValHi));

	//ERXRDPTL and ERXRDPTH are used for protection and reporting status size
	enc28j60_writeReg(dev, dev->bank0.ERXRDPTL, (uint8_t) dev->rxBufEndAddr.u8ValLo);
	enc28j60_writeReg(dev, dev->bank0.ERXRDPTH, (uint8_t) dev->rxBufEndAddr.u8ValHi);

	//For TX Start
	enc28j60_writeReg(dev, dev->bank0.ETXSTL, (uint8_t) dev->txBufStartAddr.u8ValLo);
	enc28j60_writeReg(dev, dev->bank0.ETXSTH, (uint8_t) dev->txBufStartAddr.u8ValHi);

	//For TX End
	enc28j60_writeReg(dev, dev->bank0.ETXNDL, (uint8_t) dev->txBufEndAddr.u8ValLo);
	enc28j60_writeReg(dev, dev->bank0.ETXNDH, (uint8_t) dev->txBufEndAddr.u8ValHi);

	enc2860_phyInit(dev);

	//Program and Initialize the MAC
	enc2860_macInit(dev);

	//Program receive filters
	enc28j60_rxSetFilters(dev, RX_UNICAST | RX_CRC_CHECK | RX_MULTICAST | RX_BROADCAST); //enc28j60_rxSetFilters(dev, RX_UNITCAST | RX_CRC_CHECK | RX_MULTICAST | RX_BROADCAST); //Do unicast to stop everything
	//enc28j60_rxSetFilters(dev, RX_CRC_CHECK | RX_UNICAST);
	//enc28j60_rxSetFilters(dev, RX_UNITCAST | RX_CRC_CHECK | RX_PATTERN_MATCH
		//	| RX_MAGIC_PACKET | RX_HASH_TABLE | RX_MULTICAST | RX_BROADCAST);

	//Enable all interrupts and let's see.
	enc28j60_writeReg(dev, dev->bank0.commonRegs.EIE, 0xFF);

	//Are we going to do increment?
	enc28j60_BitFieldSet(dev, dev->bank0.commonRegs.ECON2, (1 << 7));

	//Write to RXEN
	enc28j60_BitFieldSet(dev, dev->bank0.commonRegs.ECON1, (1 << 2));
}


static void enc2860_macInit(enc28j60Drv * dev)
{
	//Step 1) clear MARST bit in MACON2
	uint8_t u8TempValueHolder = enc28j60_readMacMIIReg(dev, dev->bank2.MACLCON2);
	u8TempValueHolder &= ~(1 << 7);
	enc28j60_writeReg(dev, dev->bank2.MACLCON2, u8TempValueHolder);

	//Step 2) Set the MARXEN bit in MACON1
	u8TempValueHolder = enc28j60_readMacMIIReg(dev, dev->bank2.MACON1);
	//Enable MAC to receive packets, Allow Flow Control for TX and RX
	u8TempValueHolder |= ((1 << 3) | (1 << 2) | 0x01);
	enc28j60_writeReg(dev, dev->bank2.MACON1, u8TempValueHolder);

	//Step 3) Configure the PADCFG, TXCRCEN and half duplex
	u8TempValueHolder = enc28j60_readMacMIIReg(dev, dev->bank2.MACON3);
	//u8TempValueHolder |= ((7 << 5) | (1 << 4)); half duplex
	u8TempValueHolder |= ((7 << 5) | (1 << 4) | 0x01);
	enc28j60_writeReg(dev, dev->bank2.MACON3, u8TempValueHolder);
	//Step 4) Configure bits in MACON4
	//We do nothing here.

	//Step 5) Write Tx maximum size
	enc28j60_writeReg(dev, dev->bank2.MAMXFLL, dev->MxmPkSize.u8ValLo);
	enc28j60_writeReg(dev, dev->bank2.MAMXFLH, dev->MxmPkSize.u8ValHi);

	//Step 6) 0x15 for full duplex
	enc28j60_writeReg(dev, dev->bank2.MABBIPG, 0x15);

	//Step 7) 0x12
	enc28j60_writeReg(dev, dev->bank2.MAIPGL, 0x12);

	//Step 8) Don't do anything as we are in full duplex mode

	//Step 9) Don't do anything here

	//Step 10) Let's write the MAC address

	enc28j60_writeReg(dev, dev->bank3.MAADR0, dev->mac.u8Mac0);
	enc28j60_writeReg(dev, dev->bank3.MAADR1, dev->mac.u8Mac1);
	enc28j60_writeReg(dev, dev->bank3.MAADR2, dev->mac.u8Mac2);
	enc28j60_writeReg(dev, dev->bank3.MAADR3, dev->mac.u8Mac3);
	enc28j60_writeReg(dev, dev->bank3.MAADR4, dev->mac.u8Mac4);
	enc28j60_writeReg(dev, dev->bank3.MAADR5, dev->mac.u8Mac5);

}

static void enc2860_phyInit(enc28j60Drv * dev)
{
	//Configure PHCON1
	uint16_t u16ReturnValue = enc28j60_readPhyReg(dev, dev->phyReg.PHCON1);
	u16ReturnValue |= (1 << 8); //for half duplex comment this out.
	enc28j60_writePhyReg(dev, dev->phyReg.PHCON1, u16ReturnValue);

	u16ReturnValue = enc28j60_readPhyReg(dev, dev->phyReg.PHIE);
	u16ReturnValue |= ((1 << 1) | (1 << 4));
	enc28j60_writePhyReg(dev, dev->phyReg.PHIE, u16ReturnValue);
}

bool enc28j60_intPnd(enc28j60Drv * dev)
{
	return dev->bInterruptFlag;
}

void enc28j60_intSet(enc28j60Drv * dev)
{
	dev->bInterruptFlag = true;
}

void enc28j60_intCls(enc28j60Drv * dev)
{
	dev->bInterruptFlag = false;
	//Clear the bits as well
}

uint8_t enc28j60_getEtherInterrupt(enc28j60Drv * dev)
{
	switch(dev->bnBank)
	{
	default:
		return 0;
	case bank_0:
		return enc28j60_readEtherReg(dev, dev->bank0.commonRegs.EIR);
		break;
	case bank_1:
		return enc28j60_readEtherReg(dev, dev->bank1.commonRegs.EIR);
		break;
	case bank_2:
		return enc28j60_readEtherReg(dev, dev->bank2.commonRegs.EIR);
		break;
	case bank_3:
		return enc28j60_readEtherReg(dev, dev->bank3.commonRegs.EIR);
		break;
	}
}

uint16_t enc28j60_getPhyInterrupt(enc28j60Drv * dev)
{
	return enc28j60_readPhyReg(dev, dev->phyReg.PHIR);
}

uint8_t enc28j60_getwakeUpInterrupt(enc28j60Drv * dev)
{
	return enc28j60_readEtherReg(dev, dev->bank1.EWOLIR);
}

void enc28j60_sftRst(enc28j60Drv * dev)
{
	enc28j60_SoftReset(dev);
}

bool enc28j60_etherTransmit(enc28j60Drv * dev, uint8_t * u8PtrData, const uint16_t length)
{

	//1) Start by writing to the initial address.
	enc28j60_writeReg(dev, dev->bank0.EWRPTH, (uint8_t)((dev->txBufStartAddr.u16Val & 0xFF00) >> 0x08));
	enc28j60_writeReg(dev, dev->bank0.EWRPTL, (uint8_t)(dev->txBufStartAddr.u16Val & 0x00FF));


	//1) Start by writing to the initial address.
	enc28j60_writeReg(dev, dev->bank0.ETXSTH, (uint8_t)((dev->txBufStartAddr.u16Val & 0xFF00) >> 0x08));
	enc28j60_writeReg(dev, dev->bank0.ETXSTL, (uint8_t)(dev->txBufStartAddr.u16Val & 0x00FF));


	//Start loading the data into the buffer.
	dev->spi.fncPtrCS();

	//2) Send WBM SPI command to write per control byte
	uint8_t u8Command = dev->opcode.u8WriteBufferMemory;
	//Send command to read buffer memory
	dev->spi.fncPtrWrite(&u8Command, 1);

	//Add the offset as well.
	dev->txPkt.txPktLen.u16PktLen = dev->txBufStartAddr.u16Val + length;

	uint8_t u8ControlByte = 0x00;
	dev->spi.fncPtrWrite(&u8ControlByte, 1);

	dev->spi.fncPtrWrite(u8PtrData, length);

	//Add one byte for the control byte

	// Then program the EXTND pointer
	enc28j60_writeReg(dev, dev->bank0.ETXNDH, (uint8_t)((dev->txPkt.txPktLen.u16PktLen & 0xFF00) >> 0x08));
	enc28j60_writeReg(dev, dev->bank0.ETXNDL, (uint8_t)(dev->txPkt.txPktLen.u16PktLen & 0x00FF));


	// Clear EIR.TXIF
	enc28j60_BitFieldClear(dev, dev->bank0.commonRegs.EIR, 1 << 3);

	// Set TXIE and INTIE for interrupt
	enc28j60_BitFieldSet(dev, dev->bank0.commonRegs.EIE, 1 << 3);
	//Global interrupt as well.
	enc28j60_BitFieldSet(dev, dev->bank0.commonRegs.EIE, 1 << 7);

	//Do the Errata work-around first
	//NB This is only possible when connected in Half-Duplex
	//enc28j60_BitFieldSet(dev, dev->bank0.commonRegs.ECON1, 1 << 7);
	//enc28j60_BitFieldClear(dev, dev->bank0.commonRegs.ECON1, 1 << 7);

	// Start Transmission by setting TXRST found in ECON1.
	enc28j60_BitFieldSet(dev, dev->bank0.commonRegs.ECON1, 1 << 3);

	dev->spi.fncPtrChipDS();

	return true;
}

bool enc28j60_etherReceive(enc28j60Drv * dev, uint8_t * u8PtrData, const uint16_t length)
{
	bool bReturnValue = true;
	//Write to the lock mechanism to prevent overwriting to the unread places
	addrPtr currentAddr;
	currentAddr.ptrLo = dev->rxPkt.ptrAddr.ptrLo;
	currentAddr.ptrHi = dev->rxPkt.ptrAddr.ptrHi;

	uint16_t u16CurrentAddr = currentAddr.ptrLo;
	u16CurrentAddr |= (dev->rxPkt.ptrAddr.ptrHi << 8);

	if(u16CurrentAddr % 2 == 0)
	{
		//This is an even number.
		if(u16CurrentAddr == dev->rxBufStartAddr.u16Val)
		{
			currentAddr.ptrLo = (uint8_t) (dev->rxBufEndAddr.u16Val & 0x00FF);
			currentAddr.ptrHi = (uint8_t) (dev->rxBufEndAddr.u16Val >> 0x08);
		}else
		{
			u16CurrentAddr -= 1; //we substract by one to get an even number
			currentAddr.ptrLo = (uint8_t) (u16CurrentAddr & 0x00FF);
			currentAddr.ptrHi = (uint8_t) (u16CurrentAddr >> 0x08);
		}
	}

	//This is the address of the packet that we are currently processing.
	enc28j60_writeReg(dev, dev->bank0.ERXRDPTL, currentAddr.ptrLo);
	enc28j60_writeReg(dev, dev->bank0.ERXRDPTH, currentAddr.ptrHi);

	//Start by writing to the Read address
	enc28j60_writeReg(dev, dev->bank0.ERDPTL, (uint8_t) dev->rxPkt.ptrAddr.ptrLo);
	enc28j60_writeReg(dev, dev->bank0.ERDPTH, (uint8_t) dev->rxPkt.ptrAddr.ptrHi);

	/*****************************************************************************************************
	 * Packet
	 * byte 0: next packet pointer low
	 * byte 1: next packet pointer high
	 *
	 * byte status: receive status vector
	 * status[0]
	 * status[1]
	 * status[2]
	 * status[3]
	 *
	 * Then the data
	 *****************************************************************************************************/

	//Then check for any more relevant information
	dev->spi.fncPtrCS();

	uint8_t u8Command = dev->opcode.u8readBufferMemory;
	//Send command to read buffer memory
	dev->spi.fncPtrWrite(&u8Command, 1);

	//Start reading the actual data
	//The next packet pointer is saved, then used in the next interrupt
	(void) dev->spi.fncPtrRead(dev->rxPkt.nxtPktAddr, 2);
	dev->rxPkt.ptrAddr.u16Ptr = *(dev->rxPkt.nxtPktAddr + 0);
	dev->rxPkt.ptrAddr.u16Ptr |= *(dev->rxPkt.nxtPktAddr + 1) << 8;

	if(dev->rxPkt.ptrAddr.u16Ptr >= 8192)
	{
		//dMesgPrint(DEBUG_ERROR, "Address can never be greater than 8192\r\n");
		bReturnValue = false;
	}

	dMesgPrint(DEBUG_INFO, "Next Point address --> %d\r\n", ((dev->rxPkt.ptrAddr.ptrHi << 8) | dev->rxPkt.ptrAddr.ptrLo));

	//Get the receive status vector
	(void) dev->spi.fncPtrRead(dev->rxPkt.rxStatVect, 4);

	//For now we compute the length of the packet manually instead of using c-structs
	dev->rxPkt.rxPktLen.u16PktLen = *(dev->rxPkt.rxStatVect);
	dev->rxPkt.rxPktLen.u16PktLen |= *(dev->rxPkt.rxStatVect + 1) << 8;

	uint16_t u16Flags = *(dev->rxPkt.rxStatVect + 2);
	u16Flags |= *(dev->rxPkt.rxStatVect + 3) << 8;

	for(uint8_t i = 0; i < 16; i++)
	{
		if(u16Flags & (1 << i))
		{
			switch(i)
			{
			default:
				break;
			case 0:
				dMesgPrint(DEBUG_INFO, "ETH --> Long Event/Drop Event\r\n");
				break;
			case 2:
				dMesgPrint(DEBUG_INFO, "ETH --> Carrier Event Previously Seen\r\n");
				break;
			case 4:
				dMesgPrint(DEBUG_ERROR, "ETH --> CRC Error\r\n");
				break;
			case 5:
				dMesgPrint(DEBUG_ERROR, "ETH --> Length Check Error\r\n");
				break;
			case 6:
				dMesgPrint(DEBUG_ERROR, "ETH --> Length out of range\r\n");
				break;
			case 7:
				dMesgPrint(DEBUG_INFO, "ETH --> RX OK\r\n");
				break;
			case 8:
				dMesgPrint(DEBUG_INFO, "ETH --> RX Multi-cast PKT\r\n");
				break;
			case 9:
				dMesgPrint(DEBUG_INFO, "ETH --> RX Broadcast PKT\r\n");
				break;
			case 10:
				dMesgPrint(DEBUG_INFO, "ETH --> Dribble Nibble\r\n");
				break;
			case 11:
				dMesgPrint(DEBUG_INFO, "ETH --> RX Control Frame\r\n");
				break;
			case 12:
				dMesgPrint(DEBUG_INFO, "ETH --> RX Pause Control Frame\r\n");
				break;
			case 13:
				dMesgPrint(DEBUG_INFO, "ETH --> RX unknown opcode\r\n");
				break;
			case 14:
				dMesgPrint(DEBUG_INFO, "ETH --> RX VLAN Type Detected\r\n");
				break;
			}
		}
	}

	if(dev->rxPkt.rxPktLen.u16PktLen == 0)
	{
		for(uint8_t i = 0; i < 4; i++)
		{
			dMesgPrint(DEBUG_ERROR, "Status vector registers %d: %d\r\n", i, *(dev->rxPkt.rxStatVect + i));
		}

		dMesgPrint(DEBUG_ERROR, "The packet length can never be 0\r\n");
		bReturnValue = false;
	}
	dev->rxPkt.rxPktLen.u16PktLen -= 4;

	if(dev->rxPkt.rxPktLen.u16PktLen >= 1518)
	{
		//The length can never be 65535, right?
		for(uint8_t i = 0; i < 4; i++)
		{
			dMesgPrint(DEBUG_ERROR, "Status vector registers %d: %d\r\n", i, *(dev->rxPkt.rxStatVect + i));
		}
		dMesgPrint(DEBUG_ERROR, "Packet length is probably the result of (0 - 4) which is wrong 0\r\n");
		dev->rxPkt.rxPktLen.u16PktLen = 0;
		bReturnValue = false;
	}

	//There is data available from here.
	dev->spi.fncPtrRead(dev->rxPkt.data, dev->rxPkt.rxPktLen.u16PktLen);

	//We are done
	dev->spi.fncPtrChipDS();

	//Clear the flag for interrupts.
	enc28j60_BitFieldSet(dev, dev->bank0.commonRegs.ECON2, (1 << 6));

	return bReturnValue;
}

static void enc28j60_rxSetFilters(enc28j60Drv * dev, rx_filter_control filter)
{
	uint8_t u8CastValue = (uint8_t) filter;
	enc28j60_writeReg(dev, dev->bank1.ERXFCON, u8CastValue);
}

static void enc28j60_bankChange(enc28j60Drv * dev, encj28j60_bank bBank)
{
	uint8_t u8Command;
	uint8_t u8data = 0x03; //clear the last two bits, meaning we are going to bank 0

	switch(dev->bnBank)
	{
		default:
			return;
		case bank_0:
			u8Command =	(dev->opcode.u8BitFieldClear) | (0x1F & dev->bank0.commonRegs.ECON1);
			break;
		case bank_1:
			u8Command =	(dev->opcode.u8BitFieldClear) | (0x1F & dev->bank1.commonRegs.ECON1);
			break;
		case bank_2:
			u8Command =	(dev->opcode.u8BitFieldClear) | (0x1F & dev->bank2.commonRegs.ECON1);
			break;
		case bank_3:
			u8Command =	(dev->opcode.u8BitFieldClear) | (0x1F & dev->bank3.commonRegs.ECON1);
			break;
	}

	dev->spi.fncPtrCS();
	dev->spi.fncPtrWrite(&u8Command, 1);
	dev->spi.fncPtrWrite(&u8data, 1);
	dev->spi.fncPtrChipDS();

	uint8_t u8CmdNextBank;
	u8CmdNextBank = (dev->opcode.u8BitFieldSet) | (0x1F & dev->bank0.commonRegs.ECON1);

	uint8_t u8BitsToSets = convertBankToBits(bBank);
	//Zero is the only value with all clear bits, so there is no need for any processing.
	if(u8BitsToSets != 0 && u8BitsToSets <= 3u)
	{
		dev->spi.fncPtrCS();
		dev->spi.fncPtrWrite(&u8CmdNextBank, 1);
		dev->spi.fncPtrWrite(&u8BitsToSets, 1);
		dev->spi.fncPtrChipDS();
		dev->bnBank = bBank;
	}

}

static encj28j60_bank convertRegValToBank(uint8_t value)
{
	encj28j60_bank bReturnValue = undefBank;
	switch((value >> 06))
	{
		default:
			break;
		case 0:
			bReturnValue = bank_0;
			break;
		case 1:
			bReturnValue = bank_1;
			break;
		case 2:
			bReturnValue = bank_2;
			break;
		case 3:
			bReturnValue = bank_3;
			break;
	}
	return bReturnValue;
}

static uint8_t convertBankToBits(encj28j60_bank bBank)
{
	uint8_t u8TempValue = (uint8_t) bBank;
	u8TempValue >>= 6u;
	return u8TempValue;
}
uint8_t enc28j60_readEtherReg(enc28j60Drv * dev, uint8_t u8Reg)
//static uint8_t enc28j60_readEtherReg(enc28j60Drv * dev, uint8_t u8Reg)
{
	encj28j60_bank bBank = convertRegValToBank(u8Reg);
	if(bBank != dev->bnBank) enc28j60_bankChange(dev, bBank);
	uint8_t u8Command = (dev->opcode.u8readControlRegister) | (0x1F & u8Reg);
	uint8_t u8ReturnValue;
	dev->spi.fncPtrCS();
	dev->spi.fncPtrWrite(&u8Command, 1);
	(void) dev->spi.fncPtrRead(&u8ReturnValue, 1);
	dev->spi.fncPtrChipDS();
	return u8ReturnValue;
}

static uint8_t enc28j60_readMacMIIReg(enc28j60Drv * dev, uint8_t u8Reg)
{
	encj28j60_bank bBank = convertRegValToBank(u8Reg);
	if(bBank != dev->bnBank) enc28j60_bankChange(dev, bBank);
	uint8_t u8Command = (dev->opcode.u8readControlRegister) | (0x1F & u8Reg);
	uint8_t u8ReturnValue;
	dev->spi.fncPtrCS();
	dev->spi.fncPtrWrite(&u8Command, 1);
	(void) dev->spi.fncPtrRead(&u8ReturnValue, 1); //At this point it is dummy data
	(void) dev->spi.fncPtrRead(&u8ReturnValue, 1); //Good Data
	dev->spi.fncPtrChipDS();
	return u8ReturnValue;
}
static void enc28j60_writeReg(enc28j60Drv * dev, uint8_t u8Reg, uint8_t u8Value)
{
	encj28j60_bank bBank = convertRegValToBank(u8Reg);
	if(bBank != dev->bnBank) enc28j60_bankChange(dev, bBank);
	uint8_t u8Command = (dev->opcode.u8writeControlRegister) | (0x1F & u8Reg);
	dev->spi.fncPtrCS();
	dev->spi.fncPtrWrite(&u8Command, 1);
	dev->spi.fncPtrWrite(&u8Value, 1);
	dev->spi.fncPtrChipDS();
}

void enc28j60_BitFieldSet(enc28j60Drv * dev, uint8_t u8Reg, uint8_t u8data)
//static void enc28j60_BitFieldSet(enc28j60Drv * dev, uint8_t u8Reg, uint8_t u8data)
{
	encj28j60_bank bBank = convertRegValToBank(u8Reg);
	if(bBank != dev->bnBank) enc28j60_bankChange(dev, bBank);
	uint8_t u8Command = (dev->opcode.u8BitFieldSet) | (0x1F & u8Reg);
	dev->spi.fncPtrCS();
	dev->spi.fncPtrWrite(&u8Command, 1);
	dev->spi.fncPtrWrite(&u8data, 1);
	dev->spi.fncPtrChipDS();
}

void enc28j60_BitFieldClear(enc28j60Drv * dev, uint8_t u8Reg, uint8_t u8data)
//static void enc28j60_BitFieldClear(enc28j60Drv * dev, uint8_t u8Reg, uint8_t u8data)
{
	encj28j60_bank bBank = convertRegValToBank(u8Reg);
	if(bBank != dev->bnBank) enc28j60_bankChange(dev, bBank);
	uint8_t u8Command = (dev->opcode.u8BitFieldClear) | (0x1F & u8Reg);
	dev->spi.fncPtrCS();
	dev->spi.fncPtrWrite(&u8Command, 1);
	dev->spi.fncPtrWrite(&u8data, 1);
	dev->spi.fncPtrChipDS();
}

static void enc28j60_SoftReset(enc28j60Drv * dev)
{
	uint8_t u8Command = (dev->opcode.u8SoftReset);
	dev->spi.fncPtrCS();
	dev->spi.fncPtrWrite(&u8Command, 1);
	dev->spi.fncPtrChipDS();
}

uint16_t enc28j60_readPhyReg(enc28j60Drv * dev, uint8_t addr)
//static uint16_t enc28j60_readPhyReg(enc28j60Drv * dev, uint8_t addr)
{
	enc28j60_writeReg(dev, dev->bank2.MIREGADR, addr);
	uint8_t u8ReturnValuefrSet = enc28j60_readMacMIIReg(dev, dev->bank2.MICMD);

	u8ReturnValuefrSet |= (1 << 0);
	enc28j60_writeReg(dev, dev->bank2.MICMD, u8ReturnValuefrSet);

	dev->fncPtrDelayFunc(1);

	while(enc28j60_readMacMIIReg(dev, dev->bank3.MISTAT) & (0x01)) continue;

	uint8_t u8ReturnValueFrClr = enc28j60_readMacMIIReg(dev, dev->bank2.MICMD);
	u8ReturnValueFrClr &= ~(1 << 0);
	enc28j60_writeReg(dev, dev->bank2.MICMD, u8ReturnValueFrClr);

	uint16_t u16Value;
	u16Value = enc28j60_readMacMIIReg(dev, dev->bank2.MIRDH) << 8;
	u16Value |= enc28j60_readMacMIIReg(dev, dev->bank2.MIRDL);

	return u16Value;
}

static void enc28j60_writePhyReg(enc28j60Drv * dev, uint8_t addr, uint16_t data)
{
	enc28j60_writeReg(dev, dev->bank2.MIREGADR, addr);
	enc28j60_writeReg(dev, dev->bank2.MIWRL, (uint8_t)(data & 0xFF));
	enc28j60_writeReg(dev, dev->bank2.MIWRH, (uint8_t)(data >> 8));
	dev->fncPtrDelayFunc(1);
	while(enc28j60_readMacMIIReg(dev, dev->bank3.MISTAT) & (0x01)) continue;
}

static uint16_t enc28j60_getPhyPartNumber(enc28j60Drv * dev)
{
	uint16_t u16returnValue;
	u16returnValue = enc28j60_readPhyReg(dev, dev->phyReg.PHID2);
	return ((u16returnValue & 0x03F0) >> 4);
}

uint8_t enc28j60_getPhyRevNumber(enc28j60Drv * dev)
{
	uint16_t u16returnValue;
	u16returnValue = enc28j60_readPhyReg(dev, dev->phyReg.PHID2);
	return ((uint8_t) u16returnValue & 0x000F);
}

static uint32_t enc28j60_getPhyIdentifier(enc28j60Drv * dev)
{
	uint16_t u32returnValue;
	u32returnValue = (uint32_t) enc28j60_readPhyReg(dev, dev->phyReg.PHID1);

	u32returnValue >>= 0x03;

	u32returnValue |= ((uint32_t) enc28j60_readPhyReg(dev, dev->phyReg.PHID2) << 0x09);

	return u32returnValue;
}

#if 0
static bool enc28j60_getPhylinkHasBeenDown(enc28j60Drv * dev)
{
	uint16_t u16returnValue;
	u16returnValue = enc28j60_readPhyReg(dev, dev->phyReg.PHSTAT1);
	return (u16returnValue & (1 << 2));
}

static bool enc28j60_getPhyjabberStatusBit(enc28j60Drv * dev)
{
	uint16_t u16returnValue;
	u16returnValue = enc28j60_readPhyReg(dev, dev->phyReg.PHSTAT1);
	return (u16returnValue & (1 << 1));
}

static bool enc28j60_getPhyPolarityStatus(enc28j60Drv * dev)
{
	uint16_t u16returnValue;
	u16returnValue = enc28j60_readPhyReg(dev, dev->phyReg.PHSTAT2);
	return (u16returnValue & (1 << 4));
}

static bool enc28j60_getPhyDuplexStatus(enc28j60Drv * dev)
{
	uint16_t u16returnValue;
	u16returnValue = enc28j60_readPhyReg(dev, dev->phyReg.PHSTAT2);
	return (u16returnValue & (1 << 9));
}

static bool enc28j60_getPhyLinkStatus(enc28j60Drv * dev)
{
	uint16_t u16returnValue;
	u16returnValue = enc28j60_readPhyReg(dev, dev->phyReg.PHSTAT2);
	return (u16returnValue & (1 << 10));
}

static bool enc28j60_getPhyCollisionStatus(enc28j60Drv * dev)
{
	uint16_t u16returnValue;
	u16returnValue = enc28j60_readPhyReg(dev, dev->phyReg.PHSTAT2);
	return (u16returnValue & (1 << 11));
}

static bool enc28j60_getPhyIsRxStatus(enc28j60Drv * dev)
{
	uint16_t u16returnValue;
	u16returnValue = enc28j60_readPhyReg(dev, dev->phyReg.PHSTAT2);
	return (u16returnValue & (1 << 12));
}

static bool enc28j60_getPhyIsTxStatus(enc28j60Drv * dev)
{
	uint16_t u16returnValue;
	u16returnValue = enc28j60_readPhyReg(dev, dev->phyReg.PHSTAT2);
	return (u16returnValue & (1 << 13));
}
#endif

enc28j60Drv dev =
{
	//1) SPi Driver functions
	.spi 					= { .fncPtrCS = NULL, .fncPtrChipDS = NULL, .fncPtrWrite = NULL, .fncPtrRead = NULL},
	//2) Call back function
	.fncPtrcallBack			= NULL,
	//3) Delay function call back
	.fncPtrDelayFunc		= NULL,
	//4) Bank 0 constant definitions.
	.bank0 =
	{
		.ERDPTL				= BANK_0 | 0x00,
		.ERDPTH				= BANK_0 | 0x01,
		.EWRPTL				= BANK_0 | 0x02,
		.EWRPTH				= BANK_0 | 0x03,
		.ETXSTL				= BANK_0 | 0x04,
		.ETXSTH				= BANK_0 | 0x05,
		.ETXNDL				= BANK_0 | 0x06,
		.ETXNDH				= BANK_0 | 0x07,
		.ERXSTL				= BANK_0 | 0x08,
		.ERXSTH				= BANK_0 | 0x09,
		.ERXNDL				= BANK_0 | 0x0A,
		.ERXNDH				= BANK_0 | 0x0B,
		.ERXRDPTL			= BANK_0 | 0x0C,
		.ERXRDPTH			= BANK_0 | 0x0D,
		.ERXWRPTL			= BANK_0 | 0x0E,
		.ERXWRPTH			= BANK_0 | 0x0F,
		.EDMASTL			= BANK_0 | 0x10,
		.EDMASTH			= BANK_0 | 0x11,
		.EDMANDL			= BANK_0 | 0x12,
		.EDMANDH			= BANK_0 | 0x13,
		.EDMADSTL			= BANK_0 | 0x14,
		.EDMADSTH			= BANK_0 | 0x15,
		.EDMACSL			= BANK_0 | 0x16,
		.EDMACSH			= BANK_0 | 0x17,
		.undefined1			= BANK_0 | 0x18,
		.undefined2			= BANK_0 | 0x19,
		.Reversed1			= BANK_0 | 0x1A,
		.commonRegs			= {
			.EIE		 	= BANK_0 | 0x1B,
			.EIR 			= BANK_0 | 0x1C,
			.ESTAT			= BANK_0 | 0x1D,
			.ECON2 			= BANK_0 | 0x1E,
			.ECON1			= BANK_0 | 0x1F
		}
	},
	//5) Bank 1 constant definitions.
	.bank1 =
	{
		.EHT0				= BANK_1 | 0x00,
		.EHT1				= BANK_1 | 0x01,
		.EHT2				= BANK_1 | 0x02,
		.EHT3				= BANK_1 | 0x03,
		.EHT4				= BANK_1 | 0x04,
		.EHT5				= BANK_1 | 0x05,
		.EHT6				= BANK_1 | 0x06,
		.EHT7				= BANK_1 | 0x07,
		.EPMM0				= BANK_1 | 0x08,
		.EPMM1				= BANK_1 | 0x09,
		.EPMM2				= BANK_1 | 0x0A,
		.EPMM3				= BANK_1 | 0x0B,
		.EPMM4				= BANK_1 | 0x0C,
		.EPMM5				= BANK_1 | 0x0D,
		.EPMM6				= BANK_1 | 0x0E,
		.EPMM7				= BANK_1 | 0x0F,
		.EPMCSL				= BANK_1 | 0x10,
		.EPMCSH				= BANK_1 | 0x11,
		.undefined1			= BANK_1 | 0x12,
		.undefined2			= BANK_1 | 0x13,
		.EPMOL				= BANK_1 | 0x14,
		.EPMOH				= BANK_1 | 0x15,
		.EWOLIE				= BANK_1 | 0x16,
		.EWOLIR				= BANK_1 | 0x17,
		.ERXFCON			= BANK_1 | 0x18,
		.EPKTCNT			= BANK_1 | 0x19,
		.Reserved1			= BANK_1 | 0x1A,
		.commonRegs			= {
			.EIE	 		= BANK_1 | 0x1B,
			.EIR 			= BANK_1 | 0x1C,
			.ESTAT			= BANK_1 | 0x1D,
			.ECON2 			= BANK_1 | 0x1E,
			.ECON1			= BANK_1 | 0x1F
		}
	},
	//6) Bank 2 constant definitions.
	.bank2 =
	{
		.MACON1				= BANK_2 | 0x00,
		.MACON2				= BANK_2 | 0x01,
		.MACON3				= BANK_2 | 0x02,
		.MACON4				= BANK_2 | 0x03,
		.MABBIPG			= BANK_2 | 0x04,
		.undefined1			= BANK_2 | 0x05,
		.MAIPGL				= BANK_2 | 0x06,
		.MAIPGH				= BANK_2 | 0x07,
		.MACLCON1			= BANK_2 | 0x08,
		.MACLCON2			= BANK_2 | 0x09,
		.MAMXFLL			= BANK_2 | 0x0A,
		.MAMXFLH			= BANK_2 | 0x0B,
		.Reserved1			= BANK_2 | 0x0C,
		.MAPHSUP			= BANK_2 | 0x0D,
		.Reserved2			= BANK_2 | 0x0E,
		.undefined2			= BANK_2 | 0x0F,
		.Reserved3			= BANK_2 | 0x10,
		.MICON				= BANK_2 | 0x11,
		.MICMD				= BANK_2 | 0x12,
		.undefined3			= BANK_2 | 0x13,
		.MIREGADR			= BANK_2 | 0x14,
		.Reserved4			= BANK_2 | 0x15,
		.MIWRL				= BANK_2 | 0x16,
		.MIWRH				= BANK_2 | 0x17,
		.MIRDL				= BANK_2 | 0x18,
		.MIRDH				= BANK_2 | 0x19,
		.Reserved5			= BANK_2 | 0x1A,
		.commonRegs 		= {
			.EIE 			= BANK_2 | 0x1B,
			.EIR 			= BANK_2 | 0x1C,
			.ESTAT			= BANK_2 | 0x1D,
			.ECON2 			= BANK_2 | 0x1E,
			.ECON1			= BANK_2 | 0x1F
		}
	},
	//7) Bank 3 constant definitions.
	.bank3 =
	{
		.MAADR1				= BANK_3 | 0x00,
		.MAADR0				= BANK_3 | 0x01,
		.MAADR3				= BANK_3 | 0x02,
		.MAADR2				= BANK_3 | 0x03,
		.MAADR5				= BANK_3 | 0x04,
		.MAADR4				= BANK_3 | 0x05,
		.EBSTSD				= BANK_3 | 0x06,
		.EBSTCON			= BANK_3 | 0x07,
		.EBSTCSL			= BANK_3 | 0x08,
		.EBSTCSH			= BANK_3 | 0x09,
		.MISTAT				= BANK_3 | 0x0A,
		.undefined			= {
			[0]				= BANK_3 | 0x0B,
			[1]				= BANK_3 | 0x0C,
			[2]				= BANK_3 | 0x0D,
			[3]				= BANK_3 | 0x0E,
			[4]				= BANK_3 | 0x0F,
			[5]				= BANK_3 | 0x10,
			[6]				= BANK_3 | 0x11
		},
		.EREVID				= BANK_3 | 0x12,
		.undefined8			= BANK_3 | 0x13,
		.undefined9			= BANK_3 | 0x14,
		.ECOCON				= BANK_3 | 0x15,
		.Reserved1			= BANK_3 | 0x16,
		.EFLOCON			= BANK_3 | 0x17,
		.EPAUSL				= BANK_3 | 0x18,
		.EPAUSH				= BANK_3 | 0x19,
		.Reserved2			= BANK_3 | 0x1A,
		.commonRegs			= {
			.EIE 			= BANK_3 | 0x1B,
			.EIR 			= BANK_3 | 0x1C,
			.ESTAT			= BANK_3 | 0x1D,
			.ECON2 			= BANK_3 | 0x1E,
			.ECON1			= BANK_3 | 0x1F
		}
	},
	//8) PHY Register Definitions
	.phyReg =
	{
		.PHCON1 	= 0x00,
		.PHSTAT1 	= 0x01,
		.PHID1 		= 0x02,
		.PHID2 		= 0x03,
		.PHCON2 	= 0x10,
		.PHSTAT2 	= 0x11,
		.PHIE		= 0x12,
		.PHIR		= 0x13,
		.PHLCON		= 0x14
	},
	//9) Opcode definitions
	.opcode =
	{
		.u8readControlRegister 	= (0b000 << 0x05),
		.u8readBufferMemory 	= (0b001 << 0x05) | (0b11010),
		.u8writeControlRegister = (0b010 << 0x05),
		.u8WriteBufferMemory 	= (0b011 << 0x05) | (0b11010),
		.u8BitFieldSet 			= (0b100 << 0x05),
		.u8BitFieldClear 		= (0b101 << 0x05),
		.u8SoftReset 			= (0b111 << 0x05) | (0b11111)
	},
	//10) Interrupt flag clear
	.bInterruptFlag = false,
	//12) Bank for keeping track of current bank
	.bnBank = bank_0,
	//13) RX Buffer boundaries and sizes
	//Errata recommends that RX to be from 0 to a certain value.
	.rxBufStartAddr			= {.u16Val 	= 0x0000},
	.rxBufEndAddr			= {.u16Val 	= 0x17FF},
	.rxLockAddr				= {.u16Val	= 0x0000},

	//14) Maximum TX packet size
	.txBufStartAddr			= {.u16Val	= 0x1800},
	.txBufEndAddr			= {.u16Val	= 0x1FFF},
	//15) Maximum TX packet size
	.MxmPkSize				= {.u16Val  = 1500},
	//16) MAC Address
	.mac					= {
		.u8Mac0 			= mac[0],
		.u8Mac1 			= mac[1],
		.u8Mac2 			= mac[2],
		.u8Mac3 			= mac[3],
		.u8Mac4 			= mac[4],
		.u8Mac5 			= mac[5]
	},

	.rxPkt					= {
		.nxtPktAddr 		= {0},
		.rxStatVect 		= {0},
		.rxPktLen			= {.u16PktLen = 0},
		.data				= {0},
		.ptrAddr			= {.u16Ptr = 0x0000}
	},

	.txPkt					= {
		.txDstMacAddr		= {
			.u8DestMac0		= 0xFF,
			.u8DestMac1		= 0xFF,
			.u8DestMac2		= 0xFF,
			.u8DestMac3		= 0xFF,
			.u8DestMac4		= 0xFF,
			.u8DestMac5		= 0xFF
		},

		.txPktLen			=
		{
			.u16PktLen		= 0
		},
		.data				= {0}
	},

};
