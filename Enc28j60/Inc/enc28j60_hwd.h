/*
 * enc28j60_hardware.h
 *
 *  Created on: Feb 8, 2025
 *      Author: NK Kalambay
 */

#ifndef _ENC28J60_HWD_H_
#define _ENC28J60_HWD_H_

#include <stdbool.h>

typedef void (*spiChipSel) (void);
typedef void (*spiChipDSl) (void);
typedef void (*slaveWrite) (uint8_t * ptrData, uint16_t const u16length);
typedef void (*intHandler) (void);
typedef uint16_t (*slaveRead) (uint8_t * ptrData, uint16_t u16length);
typedef	void (*delayMs) (uint32_t u32milliSeconds);

#define BANK_0	(uint8_t) 	(0)
#define BANK_1 	(uint8_t) 	(0x01 << 6)
#define BANK_2	(uint8_t)	(0x01 << 7)
#define BANK_3	(uint8_t) 	(0x03 << 6)

typedef enum _bank
{
	bank_0 		= BANK_0,
	bank_1 		= BANK_1,
	bank_2 		= BANK_2,
	bank_3		= BANK_3,
	undefBank
}encj28j60_bank;

typedef enum _ether_wake_up_on_interrutpt_flag
{
	BCWOLIF		= (1 << 0),
	MCWOLIF 	= (1 << 1),
	HTWOLIF 	= (1 << 2),
	MPWOLIF 	= (1 << 3),
	PMWOLIF 	= (1 << 4),
	AWOLIF		= (1 << 6),
	UCWOLIF		= (1 << 7)
}enc28j60_wUp_int_flag;

typedef enum _ether_interrutpt_flag
{
	RXERIF		= (1 << 0),
	TXERIF 		= (1 << 1),
	WOLIF 		= (1 << 2),
	TXIF	 	= (1 << 3),
	LINKIF	 	= (1 << 4),
	DMAIF		= (1 << 5),
	PKTIF		= (1 << 6),

}enc28j60_eth_int_flag;

typedef enum _phy_interrupt_flag
{
	PGIF		= (1 << 2),
	PLNKIF		= (1 << 4),
}enc28j60_phy_int_flag;

typedef enum _rx_filter_control
{
	RX_BROADCAST		= 1 << 0,
	RX_MULTICAST		= 1 << 1,
	RX_HASH_TABLE		= 1 << 2,
	RX_MAGIC_PACKET		= 1 << 3,
	RX_PATTERN_MATCH	= 1 << 4,
	RX_CRC_CHECK		= 1 << 5,
	RX_AND_OR_CHECK		= 1 << 6,
	RX_UNICAST			= 1 << 7,
}rx_filter_control;

typedef struct spiDriver
{
	spiChipSel fncPtrCS;
	spiChipDSl fncPtrChipDS;
	slaveRead  fncPtrRead;
	slaveWrite fncPtrWrite;
}spiDriver;

typedef struct __attribute__((packed))
{
	uint8_t EIE;
	uint8_t EIR;
	uint8_t ESTAT;
	uint8_t ECON2;
	uint8_t ECON1;
}enc28j60_common_regs;

typedef struct __attribute__((packed))
{
	union
	{
		uint8_t u8Rregisters[32];
		struct
		{
			uint8_t ERDPTL;
			uint8_t ERDPTH;
			uint8_t EWRPTL;
			uint8_t EWRPTH;
			uint8_t ETXSTL;
			uint8_t ETXSTH;
			uint8_t ETXNDL;
			uint8_t ETXNDH;
			uint8_t ERXSTL;
			uint8_t ERXSTH;
			uint8_t ERXNDL;
			uint8_t ERXNDH;
			uint8_t ERXRDPTL;
			uint8_t ERXRDPTH;
			uint8_t ERXWRPTL;
			uint8_t ERXWRPTH;
			uint8_t EDMASTL;
			uint8_t EDMASTH;
			uint8_t EDMANDL;
			uint8_t EDMANDH;
			uint8_t EDMADSTL;
			uint8_t EDMADSTH;
			uint8_t EDMACSL;
			uint8_t EDMACSH;
			uint8_t undefined1;
			uint8_t undefined2;
			uint8_t Reversed1;
			enc28j60_common_regs commonRegs;
		};
	};
}enc28j60_bank_0;

typedef struct __attribute__((packed))
{
	union
	{
		uint8_t u8Rregisters[32];
		struct
		{
			uint8_t EHT0;
			uint8_t EHT1;
			uint8_t EHT2;
			uint8_t EHT3;
			uint8_t EHT4;
			uint8_t EHT5;
			uint8_t EHT6;
			uint8_t EHT7;
			uint8_t EPMM0;
			uint8_t EPMM1;
			uint8_t EPMM2;
			uint8_t EPMM3;
			uint8_t EPMM4;
			uint8_t EPMM5;
			uint8_t EPMM6;
			uint8_t EPMM7;
			uint8_t EPMCSL;
			uint8_t EPMCSH;
			uint8_t undefined1;
			uint8_t undefined2;
			uint8_t EPMOL;
			uint8_t EPMOH;
			uint8_t EWOLIE;
			uint8_t EWOLIR;
			uint8_t ERXFCON;
			uint8_t EPKTCNT;
			uint8_t Reserved1;
			enc28j60_common_regs commonRegs;
		};
	};
}enc28j60_bank_1;

typedef struct __attribute__((packed))
{
	union
	{
		uint8_t u8Rregisters[32];
		struct
		{
			uint8_t MACON1;
			uint8_t MACON2;
			uint8_t MACON3;
			uint8_t MACON4;
			uint8_t MABBIPG;
			uint8_t undefined1;
			uint8_t MAIPGL;
			uint8_t MAIPGH;
			uint8_t MACLCON1;
			uint8_t MACLCON2;
			uint8_t MAMXFLL;
			uint8_t MAMXFLH;
			uint8_t Reserved1;
			uint8_t MAPHSUP;
			uint8_t Reserved2;
			uint8_t undefined2;
			uint8_t Reserved3;
			uint8_t MICON;
			uint8_t MICMD;
			uint8_t undefined3;
			uint8_t MIREGADR;
			uint8_t Reserved4;
			uint8_t MIWRL;
			uint8_t MIWRH;
			uint8_t MIRDL;
			uint8_t MIRDH;
			uint8_t Reserved5;
			enc28j60_common_regs commonRegs;
		};
	};
}enc28j60_bank_2;

typedef struct __attribute__((packed))
{
	union
	{
		uint8_t u8Rregisters[32];
		struct
		{
			uint8_t MAADR1;
			uint8_t MAADR0;
			uint8_t MAADR3;
			uint8_t MAADR2;
			uint8_t MAADR5;
			uint8_t MAADR4;
			uint8_t EBSTSD;
			uint8_t EBSTCON;
			uint8_t EBSTCSL;
			uint8_t EBSTCSH;
			uint8_t MISTAT;
			uint8_t	undefined[7];
			uint8_t EREVID;
			uint8_t undefined8;
			uint8_t undefined9;
			uint8_t ECOCON;
			uint8_t Reserved1;
			uint8_t EFLOCON;
			uint8_t EPAUSL;
			uint8_t EPAUSH;
			uint8_t Reserved2;
			enc28j60_common_regs commonRegs;
		};
	};
}enc28j60_bank_3;

typedef struct __attribute__((packed)) _phy_register
{
	uint8_t PHCON1;
	uint8_t PHSTAT1;
	uint8_t PHID1;
	uint8_t PHID2;
	uint8_t PHCON2;
	uint8_t PHSTAT2;
	uint8_t PHIE;
	uint8_t PHIR;
	uint8_t PHLCON;
}phy_registers;

typedef struct __attribute__((packed)) _enc28j60_opcode
{
	uint8_t u8readControlRegister;
	uint8_t u8readBufferMemory;
	uint8_t u8writeControlRegister;
	uint8_t u8WriteBufferMemory;
	uint8_t u8BitFieldSet;
	uint8_t u8BitFieldClear;
	uint8_t u8SoftReset;
}enc28j60_opcode;

typedef union __attribute__((packed)) _enc28j60_reg_value
{
	uint16_t u16Val;
	struct
	{
		uint8_t u8ValLo;
		uint8_t u8ValHi;
	};
}enc28j60_reg_value;

typedef union __attribute__((packed)) _DestAddr
{
	uint8_t macDstAddr[6];
	struct
	{
		uint8_t u8DestMac0;
		uint8_t u8DestMac1;
		uint8_t u8DestMac2;
		uint8_t u8DestMac3;
		uint8_t u8DestMac4;
		uint8_t u8DestMac5;
	};
}DestAddr;

typedef union __attribute__((packed)) _SrcAddr
{
	uint8_t macSrcAddr[6];
	struct
	{
		uint8_t u8Mac0;
		uint8_t u8Mac1;
		uint8_t u8Mac2;
		uint8_t u8Mac3;
		uint8_t u8Mac4;
		uint8_t u8Mac5;
	};
}SrcAddr;

typedef union __attribute__((packed)) _pktLength
{
	uint16_t u16PktLen;
	uint16_t type;
	struct
	{
		uint8_t pktLo;
		uint8_t pktHi;
	};
}pktLength;

typedef union __attribute__((packed)) _addrPtr
{
	uint16_t u16Ptr;
	struct
	{
		uint8_t ptrLo;
		uint8_t ptrHi;
	};
}addrPtr;

typedef struct __attribute__((packed)) _enc28j60_rx_packet
{
	//Next Packet Pointer
	uint8_t nxtPktAddr[2];

	uint8_t rxStatVect[4];

	//Packet Data
	pktLength rxPktLen;

	//Data and all possible padding
	uint8_t	data[1024];

	//Pointer Keeper
	addrPtr ptrAddr;

}enc28j60_rx_packet;

typedef struct __attribute__((packed)) _enc28j60_tx_packet
{
	//Packet Data
	pktLength txPktLen;

	//Data buffer
	uint8_t	data[512];
}enc28j60_tx_packet;

typedef struct _enc28j60_driver
{
	spiDriver spi;
	intHandler fncPtrcallBack;
	delayMs fncPtrDelayFunc;
	enc28j60_bank_0 const bank0;
	enc28j60_bank_1 const bank1;
	enc28j60_bank_2 const bank2;
	enc28j60_bank_3 const bank3;
	phy_registers const phyReg;
	enc28j60_opcode const opcode;
	encj28j60_bank bnBank;

	//Address
	enc28j60_reg_value const rxBufStartAddr;
	enc28j60_reg_value const rxBufEndAddr;
	enc28j60_reg_value rxLockAddr;
	enc28j60_reg_value const txBufStartAddr;
	enc28j60_reg_value const txBufEndAddr;
	enc28j60_reg_value const MxmPkSize;

	//Mac Address
	SrcAddr mac;

	enc28j60_rx_packet rxPkt;
	enc28j60_tx_packet txPkt;

}enc28j60Drv;

void enc28j60_initDr(enc28j60Drv * dev, spiChipSel cs, spiChipDSl dCS, slaveRead rd, slaveWrite wr, intHandler hdle, delayMs delay);
void enc28j60_strtDr(enc28j60Drv * dev);
void enc28j60_sftRst(enc28j60Drv * dev);

bool enc28j60_etherReceive(enc28j60Drv * dev);
bool enc28j60_etherTransmit(enc28j60Drv * dev, uint8_t * u8PtrData, const uint16_t length);

uint8_t enc28j60_getEtherInterrupt(enc28j60Drv * dev);
uint16_t enc28j60_getPhyInterrupt(enc28j60Drv * dev);
uint8_t enc28j60_getwakeUpInterrupt(enc28j60Drv * dev);

uint8_t enc28j60_getPhyRevNumber(enc28j60Drv * dev);
uint8_t enc28j60_readEtherReg(enc28j60Drv * dev, uint8_t u8Reg);
uint16_t enc28j60_readPhyReg(enc28j60Drv * dev, uint8_t addr);

uint8_t enc28j60_getPhyRevNumber(enc28j60Drv * dev);
bool enc28j60_getPhylinkHasBeenDown(enc28j60Drv * dev);
bool enc28j60_getPhyjabberStatusBit(enc28j60Drv * dev);
bool enc28j60_getPhyPolarityStatus(enc28j60Drv * dev);
bool enc28j60_getPhyDuplexStatus(enc28j60Drv * dev);
bool enc28j60_getPhyLinkStatus(enc28j60Drv * dev);
bool enc28j60_getPhyCollisionStatus(enc28j60Drv * dev);
bool enc28j60_getPhyIsRxStatus(enc28j60Drv * dev);
bool enc28j60_getPhyIsTxStatus(enc28j60Drv * dev);

void enc28j60_clear_interrupt(enc28j60Drv * dev, enc28j60_eth_int_flag flags);
void enc28j60_global_Int_Set(enc28j60Drv * dev);
void enc28j60_global_int_Clear(enc28j60Drv * dev);
void enc28j60_link_change_int_clear(enc28j60Drv * dev);

#endif /* _ENC28J60_HWD_H_ */
