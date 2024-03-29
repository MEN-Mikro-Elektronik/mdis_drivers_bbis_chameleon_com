/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: bb_chameleon.c
 *      Project: CHAMELEON board handler
 *
 *       Author: kp
 *
 *  Description: Generic CHAMELEON Base Board handler
 *
 *  Provides a BBIS driver interface for boards with chameleon FPGA.
 *  Supports "manual enumeration" (default) and "automatic enumeration"
 *  (if optional descriptor key AUTOENUM=1).
 *
 *
 *  Manual Enumeration
 *  ==================
 *  Each chameleon variant needs an adapted descriptor.
 *  Descriptor specifies which BBIS slot is assigned to which module within
 *  the chameleon FPGA:
 *
 *  For each module to be supported by MDIS, the
 *  DEVICE_ID_<n> descriptor key must be present, where <n> is the decimal
 *  representation of the BBIS slot number (0..15).
 *
 *  The value of DEVICE_ID_<n> is a 16-bit word, where the MSB is the
 *  chameleon module code (see chameleon.h) and the LSB is the instance
 *  number of that module.
 *
 *  In addition, this BBIS driver needs information about PCI bus and
 *  device number of the FPGA
 *
 *  For example, on EM01-EC01, a descriptor can look like this:
 *
 *  CHAM_EM01_EC01 {
 *     ...
 *     PCI_BUS_NUMBER 	 	 = U_INT32  0x00
 *     PCI_DEVICE_NUMBER	 = U_INT32  0x1d
 *	   DEVICE_ID_0 		     = U_INT32  0x0800 	# CAN index 0
 *	   DEVICE_ID_1 		     = U_INT32  0x0801 	# CAN index 1
 *	   DEVICE_IDV2_2     	 = U_INT32  0x2200 	# GPIO instance 0
 *	   DEVICE_IDV2_3     	 = U_INT32  0x2201 	# GPIO instance 1
 *	   GROUP_4 {
 *         GROUP_ID      = U_INT32  0x0001 	# group Nr. as in cham table
 *	       DEVICE_IDV2_0 = U_INT32  0x3500 	# IDE,     group 1, instance 0
 *	       DEVICE_IDV2_1 = U_INT32  0x4400 	# IDETGT , group 1, instance 0
 *	       DEVICE_IDV2_2 = U_INT32  0x4600 	# IDEDISK, group 1, instance 0
 *     }
 *	   GROUP_5 {
 *         GROUP_ID      = U_INT32  0x0002 	# group Nr. as in cham table
 *	       DEVICE_IDV2_0 = U_INT32  0x2c00 	# DISP, group 2, instance 0
 *	       DEVICE_IDV2_1 = U_INT32  0x2b00 	# SDRAM, group 2, instance 0
 *     }
 *	   DEVICE_IDV2_6     	 = U_INT32  0x2202 	# GPIO instance 2
 *  }
 *
 * Note: If one of the modules specified with DEVICE_ID_<n> could not be
 * found, only this slot is unusuable.
 *
 *
 *  Automatic Enumeration
 *  =====================
 *  If the descriptor key AUTOENUM is set to 1, the CHAMELEON BBIS performs
 *  an automatic detection of all modules implemented in the FPGA. Thereby,
 *  the found modules in the FPGA will be assigned to the BBIS slot
 *  numbers (0..BBIS_MAX_DEVS) in the same order they were found.
 *  In this case, the DEVICE_ID_<n> descriptor keys are ignored.
 *
 *  Optional, the descriptor key AUTOENUM_EXCLUDING can be used to exclude
 *  certain modules from the automatic enumeration by specifying their
 *  chameleon module codes (see chameleon.h).
 *
 *  Automatic enumeration of groups:
 *  For chameleon V2 tables, the driver supports device groups. For every group
 *  an own slot is assigned. The order of the devices in the table have to be
 *  exactly the same as expected by the driver handling the group. The first
 *  module of a group defines the functionality and ID of the group.
 *
 *  If a group has to be excluded from AUTOENUM, the AUTOENUM_EXCLUDING key has
 *  to hold the first member of the group. The following members of the group
 *  are excluded automatically by this driver.
 *
 *  Example for an automatic enumeration:
 *
 *  Descriptor keys:
 *	  AUTOENUM 			   = U_INT32 1 					# automatic enumeration
 *	  AUTOENUM_EXCLUDINGV2 = BINARY 0x23,0x19,0x34,0x2C # excluded device IDs
 *    or (if AUTOENUM_EXCLUDINGV2 is not found)
 *    AUTOENUM_EXCLUDING   = BINARY 0x0a,0x07,0x20,0x25 # excluded module codes
 *
 *  FPGA modules
 *  nbr  | devId | group | name         | BBIS slot number
 *  -----+-------+-------+--------------+-----------------
 *  0    |  0x23 |   0   | Z035_SYSTEM  | (excluded)
 *  1    |  0x19 |   0   | Z025_UART    | (excluded)
 *  2    |  0x22 |   0   | Z034_GPIO    | 0
 *  3    |  0x1D |   0   | Z029_CAN     | 1
 *  4    |  0x1D |   0   | Z029_CAN     | 2
 *  5..8 |  0x34 |   0   | Z052_GIRQ    | (excluded)
 *  9    |  0x2C |   1   | Z044_DISP    | (excluded)
 *  10   |  0x35 |   2   | Z053_IDE     | 3
 *  11   |  0x2B |   1   | Z043_SDRAM   | (excluded)
 *  12   |  0x2B |   2   | Z043_SDRAM   | 3
 *  13   |  0x44 |   2   | Z068_IDETGT  | 3
 *  14   |  0x46 |   2   | Z070_IDEDISK | 3
 *
 *
 *
 *     Required: chameleon library
 *     Switches: _ONE_NAMESPACE_PER_DRIVER_
 *
 *---------------------------------------------------------------------------
 * Copyright 2003-2019, MEN Mikro Elektronik GmbH
 ******************************************************************************/
/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#define _NO_BBIS_HANDLE		/* bb_defs.h: don't define BBIS_HANDLE struct */

#include <MEN/men_typs.h>   /* system dependend definitions   */
#include <MEN/mdis_com.h>
#include <MEN/dbg.h>        /* debug functions                */
#include <MEN/oss.h>        /* oss functions                  */
#include <MEN/desc.h>       /* descriptor functions           */
#include <MEN/bb_defs.h>    /* bbis definitions				  */
#include <MEN/mdis_err.h>   /* MDIS error codes               */
#include <MEN/mdis_api.h>   /* MDIS global defs               */
#include <MEN/chameleon.h>  /* chameleon defs                 */

#if !defined(MAC_MEM_MAPPED) && !defined(MAC_IO_MAPPED)
#define MAC_MEM_MAPPED
#endif
#include <MEN/maccess.h>

/*-----------------------------------------+
  |  DEFINES                                 |
  +-----------------------------------------*/
/* debug settings */
#define DBG_MYLEVEL		h->debugLevel
#define DBH             h->debugHdl
#define CHAMELEON_BBIS_DEBUG

/* ISA variant */
#ifdef CHAM_ISA
#	define BUSTYPE OSS_BUSTYPE_ISA
#	define BBNAME				"CHAMELEON_ISA"
#	define TABLE_IRQ			0xffffffff
/* default (PCI variant) */
#else
#	define BUSTYPE OSS_BUSTYPE_PCI
#	define BBNAME				"CHAMELEON"
#endif

#define CHAMELEON_BBIS_MAX_DEVS	256			/* max number of devices supported */
#define CHAMELEON_BBIS_MAX_GRPS	15			/* max number of groups supported */
#define CHAMELEON_NO_DEV		0xfffd		/* flags devId[x] invalid */
#define CHAMELEON_BBIS_GROUP	0xfffe		/* flags devId[x] is a group */
#define MAX_EXCL_MODCODES		0xff		/* number of max excluded module codes */
#define MAX_PCI_PATH			16		    /* max number of bridges to devices */
#define PCI_SECONDARY_BUS_NUMBER	0x19	/* PCI bridge config */

#define BBCHAM_GIRQ_SPACE_SIZE		0x20		/* 32 byte register + reserved */
#define BBCHAM_GIRQ_IRQ_REQ			0x00		/* interrupt request register */
#define BBCHAM_GIRQ_IRQ_EN			0x08		/* interrupt enable register  */
#define BBCHAM_GIRQ_API_VER			0x10		/* register contains api version */
#define BBCHAM_GIRQ_API_VER_OFF		24			/* topmost byte */
#define BBCHAM_GIRQ_IN_USE			0x14		/* in use register */
#define BBCHAM_GIRQ_IN_USE_BIT		0x1			/* in use bit */

/* switch between io and mem maccess macros */
#define _MREAD_D32(ret,ma,offs) {			\
    if( h->tblType == OSS_ADDRSPACE_IO ){               \
      ret = __BB_CHAMELEON_IoReadD32((MACCESS)ma,offs); \
    } else {                                            \
      ret = MREAD_D32(ma,offs);				\
    }                                                   \
  }

#define _MWRITE_D32(ma,offs,val) {			\
    if( h->tblType == OSS_ADDRSPACE_IO ){		\
      __BB_CHAMELEON_IoWriteD32((MACCESS)ma,offs,val);	\
    } else {						\
      MWRITE_D32(ma,offs,val);				\
    }							\
  }

/*-----------------------------------------+
  |  TYPEDEFS                                |
  +-----------------------------------------*/
typedef struct {
  u_int32 grpId;									/* group ID from Table */
  u_int16	devId[CHAMELEON_BBIS_MAX_DEVS];         /* from DEVICE_IDV2_n */
  u_int16 idx[CHAMELEON_BBIS_MAX_DEVS];			/* index (when more than one dev with same ID in group) */
  void*   dev[CHAMELEON_BBIS_MAX_DEVS]; 			/* info of each module */
  u_int32 devGotSize[CHAMELEON_BBIS_MAX_DEVS];	/* mem allocated for each dev */
  int32 	devCount;								/* num of devices in group */
}BBIS_CHAM_GRP;

typedef struct {
  MDIS_IDENT_FUNCT_TBL idFuncTbl;	/* id function table		*/
  CHAM_FUNCTBL	chamFuncTbl[2];	/* chameleon V2 function table */
  /* [0=OSS_ADDRSPACE_MEM], [1=OSS_ADDRSPACE_IO] */
  u_int32     ownMemSize;			/* own memory size			*/
  OSS_HANDLE* osHdl;				/* os specific handle		*/
  DESC_HANDLE *descHdl;			/* descriptor handle pointer*/
  u_int32     debugLevel;			/* debug level for BBIS     */
  DBG_HANDLE  *debugHdl;			/* debug handle				*/
  /* PCIbus */
#ifndef CHAM_ISA
  u_int32 	pciDomainNbr;		/* PCI domain number of FPGA */
  u_int32		pciBusNbr;			/* PCI bus number of FPGA	*/
  u_int32		pciDevNbr;			/* PCI device number of FPGA	*/
  u_int32		pciFuncNbr;			/* PCI function number of FPGA	*/
  u_int8		pciPath[MAX_PCI_PATH]; /* PCI path from desc		*/
  u_int32		pciPathLen;			/* number of bytes in pciPath	*/
  /* ISAbus */
#else
  u_int32		isaAddr;		/* ISA base address */
  u_int32		isaIrqNbr;		/* ISA device IRQ number */
#endif /* CHAM_ISA */
  u_int16		devId[CHAMELEON_BBIS_MAX_DEVS]; /* copy of DEVICE_IDV2_n */
  int16   	inst[CHAMELEON_BBIS_MAX_DEVS];	/* instance (V2) else -1 */
  u_int32 	idx[CHAMELEON_BBIS_MAX_DEVS];	/* index of cham device */
  void*		dev[CHAMELEON_BBIS_MAX_DEVS];	/* info of module */
  u_int32 	devGotSize[CHAMELEON_BBIS_MAX_DEVS];/* mem allocated for each dev */
  int32		devCount;						/* num of slots occupied */
  u_int32		tblType;			/* 0=OSS_ADDRSPACE_MEM, 1=OSS_ADDRSPACE_IO */
  u_int32		girqType;			/* 0=OSS_ADDRSPACE_MEM, 1=OSS_ADDRSPACE_IO */
  char 		*girqPhysAddr;		/* GIRQ unit physical address */
  char 		*girqVirtAddr;		/* GIRQ unit virtual address */
  u_int32		girqApiVersion;		/* GIRQ application feature register */
  u_int32		autoEnum;			/* <>0: auomatic enumeration */
  u_int8      exclModCodes[MAX_EXCL_MODCODES]; /* excluded module codes */
  u_int32		exclModCodesNbr;		/* number of excluded module codes */
  int32       			devCountInit;       /* devCount value from *_Init for multiple calls of *_BrdInit */
  OSS_SPINL_HANDLE 		*slHdl;				/* spin lock handle */
#ifdef VXWORKS
  OSS_SPINL_HANDLE 		vxSpinlock;			/* vxWorks only: spinlock struct (not pointer to it!) */
#endif
  CHAMELEONV2_INFO	chamInfo;		/* global chameleon device info */
} BBIS_HANDLE;

/* include files which need BBIS_HANDLE */
#include <MEN/bb_entry.h>			/* bbis jumptable */
#include <MEN/bb_chameleon.h>		/* chameleon bbis header file */

static const char IdentString[]=MENT_XSTR(MAK_REVISION);

/*-----------------------------------------+
  |  GLOBALS                                 |
  +-----------------------------------------*/
#ifdef OSS_VXBUS_SUPPORT
IMPORT VXB_DEVICE_ID 	sysGetMdisBusCtrlID(void);
#endif
/*-----------------------------------------+
  |  PROTOTYPES                              |
  +-----------------------------------------*/
/* init/exit */
static int32 CHAMELEON_Init(OSS_HANDLE*, DESC_SPEC*, BBIS_HANDLE**);
static int32 CHAMELEON_BrdInit(BBIS_HANDLE*);
static int32 CHAMELEON_BrdExit(BBIS_HANDLE*);
static int32 CHAMELEON_Exit(BBIS_HANDLE**);
/* info */
static int32 CHAMELEON_BrdInfo(u_int32, ...);
static int32 CHAMELEON_CfgInfo(BBIS_HANDLE*, u_int32, ...);
/* interrupt handling */
static int32 CHAMELEON_IrqEnable(BBIS_HANDLE*, u_int32, u_int32);
static int32 CHAMELEON_IrqSrvInit(BBIS_HANDLE*, u_int32);
static void  CHAMELEON_IrqSrvExit(BBIS_HANDLE*, u_int32);
/* exception handling */
static int32 CHAMELEON_ExpEnable(BBIS_HANDLE*,u_int32, u_int32);
static int32 CHAMELEON_ExpSrv(BBIS_HANDLE*,u_int32);
/* get module address */
static int32 CHAMELEON_SetMIface(BBIS_HANDLE*, u_int32, u_int32, u_int32);
static int32 CHAMELEON_ClrMIface(BBIS_HANDLE*,u_int32);
static int32 CHAMELEON_GetMAddr(BBIS_HANDLE*, u_int32, u_int32, u_int32, void**, u_int32*);
/* getstat/setstat */
static int32 CHAMELEON_SetStat(BBIS_HANDLE*, u_int32, int32, INT32_OR_64);
static int32 CHAMELEON_GetStat(BBIS_HANDLE*, u_int32, int32, INT32_OR_64*);
/* unused */
static int32 CHAMELEON_Unused(void);
/* miscellaneous */
static char* Ident( void );
static int32 Cleanup(BBIS_HANDLE *h, int32 retCode);
static int32 CfgInfoSlot( BBIS_HANDLE *h, va_list argptr );

#ifndef CHAM_ISA
static int32 ParsePciPath(
			  BBIS_HANDLE *brdHdl,
			  u_int32 *pciBusNbrP );

static int32 PciParseDev(
			 BBIS_HANDLE *brdHdl,
			 u_int32 pciBusNbr,
			 u_int32 pciDevNbr,
			 int32 *vendorIDP,
			 int32 *deviceIDP,
			 int32 *headerTypeP,
			 int32 *secondBusP);

static int32 PciCfgErr(
		       BBIS_HANDLE *brdHdl,
		       char *funcName,
		       int32 error,
		       u_int32 pciBusNbr,
		       u_int32 pciDevNbr,
		       u_int32 reg );
#endif /* CHAM_ISA */


/**************************** CHAMELEON_GetEntry ***********************************
 *
 *  Description:  Initialize drivers jump table.
 *
 *---------------------------------------------------------------------------
 *  Input......:  bbisP     pointer to the not initialized structure
 *  Output.....:  *bbisP    initialized structure
 *  Globals....:  ---
 ****************************************************************************/
extern void __BB_CHAMELEON_GetEntry( BBIS_ENTRY *bbisP )
{
  /* init/exit */
  bbisP->init         =   CHAMELEON_Init;
  bbisP->brdInit      =   CHAMELEON_BrdInit;
  bbisP->brdExit      =   CHAMELEON_BrdExit;
  bbisP->exit         =   CHAMELEON_Exit;
  bbisP->fkt04        =   CHAMELEON_Unused;
  /* info */
  bbisP->brdInfo      =   CHAMELEON_BrdInfo;
  bbisP->cfgInfo      =   CHAMELEON_CfgInfo;
  bbisP->fkt07        =   CHAMELEON_Unused;
  bbisP->fkt08        =   CHAMELEON_Unused;
  bbisP->fkt09        =   CHAMELEON_Unused;
  /* interrupt handling */
  bbisP->irqEnable    =   CHAMELEON_IrqEnable;
  bbisP->irqSrvInit   =   CHAMELEON_IrqSrvInit;
  bbisP->irqSrvExit   =   CHAMELEON_IrqSrvExit;
  bbisP->setIrqHandle =   NULL;

  bbisP->fkt14        =   CHAMELEON_Unused;
  /* exception handling */
  bbisP->expEnable    =   CHAMELEON_ExpEnable;
  bbisP->expSrv       =   CHAMELEON_ExpSrv;
  bbisP->fkt17        =   CHAMELEON_Unused;
  bbisP->fkt18        =   CHAMELEON_Unused;
  bbisP->fkt19        =   CHAMELEON_Unused;
  /* */
  bbisP->fkt20        =   CHAMELEON_Unused;
  bbisP->fkt21        =   CHAMELEON_Unused;
  bbisP->fkt22        =   CHAMELEON_Unused;
  bbisP->fkt23        =   CHAMELEON_Unused;
  bbisP->fkt24        =   CHAMELEON_Unused;
  /*  getstat / setstat / address setting */
  bbisP->setStat      =   CHAMELEON_SetStat;
  bbisP->getStat      =   CHAMELEON_GetStat;
  bbisP->setMIface    =   CHAMELEON_SetMIface;
  bbisP->clrMIface    =   CHAMELEON_ClrMIface;
  bbisP->getMAddr     =   CHAMELEON_GetMAddr;
  bbisP->fkt30        =   CHAMELEON_Unused;
  bbisP->fkt31        =   CHAMELEON_Unused;
}

/****************************** CHAMELEON_Init *******************************
 *
 *  Description:  Allocate and return board handle.
 *
 *                - initializes the board handle
 *                - reads and saves board descriptor entries
 *
 *                The following descriptor keys are used:
 *
 *                Deskriptor key           Default          Range
 *                -----------------------  ---------------  -------------
 *                DEBUG_LEVEL_DESC         OSS_DBG_DEFAULT  see dbg.h
 *                DEBUG_LEVEL              OSS_DBG_DEFAULT  see dbg.h
 *                PCI variant only:
 *				    PCI_BUS_NUMBER         -                0..255
 *                  PCI_BUS_PATH           -                binary array
 *				    PCI_DEVICE_NUMBER      -                0...31
 *                  PCI_BUS_SLOT           -                0..max
 *                  PCI_FUNCTION_NUMBER                     0..7
 *                ISA variant only:
 *                  DEVICE_ADDR            -                0..max
 *                  DEVICE_ADDR_IO         0                0,1
 *                  IRQ_NUMBER             TABLE_IRQ        0(=no IRQ)..max
 *                DEVICE_ID_n  (n=0..15)   -                0...31
 *                GROUP_n/DEVICE_IDV2_n  (n=0..15)          0...31
 *                AUTOENUM                 0                0,1
 *                AUTOENUM_EXCLUDING       -                see chameleon.h
 *
 *---------------------------------------------------------------------------
 *  Input......:  osHdl     pointer to os specific structure
 *                descSpec  pointer to os specific descriptor specifier
 *                hP		pointer to not initialized board handle structure
 *  Output.....:  *hP		initialized board handle structure
 *				  return    0 | error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 CHAMELEON_Init(
			    OSS_HANDLE      *osHdl,
			    DESC_SPEC       *descSpec,
			    BBIS_HANDLE     **hP )
{
  BBIS_HANDLE	*h = NULL;
  u_int32     gotsize, i, g;
  int32       status;
  u_int32		value;
  BBIS_CHAM_GRP *devGrp = NULL;

  /* PCIbus */
#ifndef CHAM_ISA
  u_int32     mechSlot;
#endif /* CHAM_ISA */

  /*-------------------------------+
    | initialize the board structure |
    +-------------------------------*/
  /* get memory for the board structure */
  *hP = h = (BBIS_HANDLE*) (OSS_MemGet( osHdl, sizeof(BBIS_HANDLE), &gotsize ));
  if ( h == NULL )
    return ERR_OSS_MEM_ALLOC;

  /* cleanup the turkey */
  OSS_MemFill( osHdl, sizeof(BBIS_HANDLE), (char*)h, 0x00 );

  /* store data into the board structure */
  h->ownMemSize = gotsize;
  h->osHdl = osHdl;

  /*------------------------------+
    |  init id function table       |
    +------------------------------*/
  /* drivers ident function */
  h->idFuncTbl.idCall[0].identCall = Ident;
  /* libraries ident functions */
  h->idFuncTbl.idCall[1].identCall = DESC_Ident;
  h->idFuncTbl.idCall[2].identCall = OSS_Ident;
  /* terminator */
  h->idFuncTbl.idCall[3].identCall = NULL;

  /*------------------------------+
    |  prepare debugging            |
    +------------------------------*/
  DBG_MYLEVEL = OSS_DBG_DEFAULT;	/* set OS specific debug level */
  DBGINIT((NULL,&DBH));

  DBGWRT_1((DBH,"BB - %s_Init\n",BBNAME));

  /*------------------------------+
    |  scan descriptor              |
    +------------------------------*/
  /* init descHdl */
  status = DESC_Init( descSpec, osHdl, &h->descHdl );
  if (status)
    return( Cleanup(h,status) );

  /* get DEBUG_LEVEL_DESC */
  status = DESC_GetUInt32(h->descHdl, OSS_DBG_DEFAULT, &value,
			  "DEBUG_LEVEL_DESC");
  if ( status && (status!=ERR_DESC_KEY_NOTFOUND) )
    return( Cleanup(h,status) );

  /* set debug level for DESC module */
  DESC_DbgLevelSet(h->descHdl, value);

  /* get DEBUG_LEVEL */
  status = DESC_GetUInt32( h->descHdl, OSS_DBG_DEFAULT,
			   &(h->debugLevel),
			   "DEBUG_LEVEL");
  if ( status && (status!=ERR_DESC_KEY_NOTFOUND) )
    return( Cleanup(h,status) );

  /* PCIbus */
#ifndef CHAM_ISA
  /*---- get PCI domain/bus/device number ----*/

  /* PCI_DOMAIN_NUMBER - optional (default: 0) */
  status = DESC_GetUInt32( h->descHdl, 0, &h->pciDomainNbr, "PCI_DOMAIN_NUMBER");
  if( status && (status!=ERR_DESC_KEY_NOTFOUND) )
    return( Cleanup(h,status) );
  if(status!=ERR_DESC_KEY_NOTFOUND){
    DBGWRT_3((DBH, " read PCI_DOMAIN_NUMBER=0x%x",h->pciDomainNbr));
  }

  /* PCI_BUS_NUMBER - required if PCI_BUS_PATH not given  */
  status = DESC_GetUInt32( h->descHdl, 0, &h->pciBusNbr, "PCI_BUS_NUMBER");
  if( status && (status!=ERR_DESC_KEY_NOTFOUND) )
    return( Cleanup(h,status) );
  if(status!=ERR_DESC_KEY_NOTFOUND){
    DBGWRT_3((DBH, " read PCI_BUS_NUMBER=0x%x",h->pciBusNbr));
  }

  if( status == ERR_DESC_KEY_NOTFOUND ){
    /* PCI_BUS_PATH - required if PCI_DEVICE_NUMBER not given */
    h->pciPathLen = MAX_PCI_PATH;
    status = DESC_GetBinary( h->descHdl, (u_int8*)"", 0, h->pciPath, &h->pciPathLen, "PCI_BUS_PATH");
    if( status && (status!=ERR_DESC_KEY_NOTFOUND) )
      return( Cleanup(h,status) );
#ifdef DBG
    if(status!=ERR_DESC_KEY_NOTFOUND){
      DBGWRT_3((DBH, " read PCI_BUS_PATH="));
      for(i=0; i<h->pciPathLen; i++){
        DBGWRT_3((DBH, "0x%x (dev=0x%x, func=0x%x)", h->pciPath[i], h->pciPath[i]&0x1f,  h->pciPath[i]>>5));
      }
      DBGWRT_3((DBH, "\n"));
    }
#endif

    if( status ){
      DBGWRT_ERR((DBH, "*** BB - %s_Init: Found neither Desc Key "
		  "PCI_BUS_PATH nor PCI_BUS_NUMBER !\n",	BBNAME));
      return( Cleanup(h,status) );
    }

#if ( defined(VXWORKS) && !defined(VXW_PCI_DOMAIN_SUPPORT) )
    /* ts: tweak for F50P + vxW64. TODO: clean up when also supporting F50P on vxW69 !!! */
    DBGWRT_3((DBH, " CAUTION: strange VxWorks tweak from ts\n"));
    DESC_GetUInt32( h->descHdl, 0, &mechSlot, "PCI_BUS_SLOT");
    h->pciDomainNbr = 0;
    h->pciPathLen = 1;
    h->pciPath[0] = 0x11 - mechSlot;
    DBGWRT_3((DBH, " PCI_BUS_PATH=0x%x", h->pciPath[0]));
#endif

    /*--------------------------------------------------------+
      |  parse the PCI_PATH to determine bus number of devices  |
      +--------------------------------------------------------*/
    if( (status = ParsePciPath( h, &h->pciBusNbr )) )
      return( Cleanup(h,status));

  } /* if( status == ERR_DESC_KEY_NOTFOUND ) */
  else {
    if( status == ERR_SUCCESS) {
      DBGWRT_1((DBH,"BB - %s: Using main PCI Bus Number from desc %d on PCI Domain %d\n",
		BBNAME, h->pciBusNbr, h->pciDomainNbr ));
    }
    else {
      return( Cleanup(h,status) );
    }
  }

  /* PCI_DEVICE_NUMBER - required if PCI_BUS_SLOT not given  */
  status = DESC_GetUInt32( h->descHdl, 0xffff, &h->pciDevNbr,
			   "PCI_DEVICE_NUMBER");
  if( status && (status!=ERR_DESC_KEY_NOTFOUND) )
    return( Cleanup(h,status) );
  if(status!=ERR_DESC_KEY_NOTFOUND){
    DBGWRT_3((DBH, " read PCI_DEVICE_NUMBER=0x%x",h->pciDevNbr));
  }

  if(status==ERR_DESC_KEY_NOTFOUND){

    /* PCI_BUS_SLOT - required if PCI_DEVICE_NUMBER not given */
    status = DESC_GetUInt32( h->descHdl, 0, &mechSlot, "PCI_BUS_SLOT");
    if( status && (status!=ERR_DESC_KEY_NOTFOUND) )
      return( Cleanup(h,status) );
    if(status!=ERR_DESC_KEY_NOTFOUND){
      DBGWRT_3((DBH, " read PCI_BUS_SLOT=0x%x",mechSlot));
    }

    if( status==ERR_DESC_KEY_NOTFOUND ){
      DBGWRT_ERR((DBH, "*** BB - %s_Init: Found neither Desc Key "
		  "PCI_BUS_SLOT nor PCI_DEVICE_NUMBER !\n", BBNAME));
    }

    if( status )
      return( Cleanup(h,status) );

    /* convert PCI slot into PCI device id */
    if( (status = OSS_PciSlotToPciDevice( osHdl, h->pciBusNbr, mechSlot, (int32*)&h->pciDevNbr)) )
      return( Cleanup(h,status) );

    DBGWRT_2(( DBH, "conv. PCI slot %d to PCI device id 0x%x\n", mechSlot, h->pciDevNbr ));
  }

  /* PCI_FUNCTION_NUMBER (optional)  */
  status = DESC_GetUInt32( h->descHdl, 0, &h->pciFuncNbr, "PCI_FUNCTION_NUMBER");
  if( status && (status!=ERR_DESC_KEY_NOTFOUND) )
    return( Cleanup(h,status) );

  /* ISAbus */
#else
  /* get DEVICE_ADDR */
  status = DESC_GetUInt32( h->descHdl, 0, &h->isaAddr,
			   "DEVICE_ADDR");
  if ( status ){
    DBGWRT_ERR((DBH, "*** BB - %s_Init: Desc Key DEVICE_ADDR "
		"not found\n", BBNAME));
    return( Cleanup(h,status) );
  }

  /*
   * get DEVICE_ADDR_IO (optional)
   * default value 0 = OSS_ADDRSPACE_MEM
   */
  status = DESC_GetUInt32( h->descHdl, 0, &h->tblType,
			   "DEVICE_ADDR_IO");
  if ( status && (status!=ERR_DESC_KEY_NOTFOUND) )
    return( Cleanup(h,status) );

  /* get IRQ_NUMBER (optional) */
  status = DESC_GetUInt32( h->descHdl, TABLE_IRQ, &h->isaIrqNbr,
			   "IRQ_NUMBER");
  if ( status && (status!=ERR_DESC_KEY_NOTFOUND) )
    return( Cleanup(h,status) );
#endif /* CHAM_ISA */

  /* get AUTOENUM (optional) */
  status = DESC_GetUInt32( h->descHdl, 0, &h->autoEnum, "AUTOENUM");
  if( status && (status!=ERR_DESC_KEY_NOTFOUND) )
    return( Cleanup(h,status) );

  h->devCount     = 0;
  h->devCountInit = 0;

  /* no device found yet */
  for( i=0; i<CHAMELEON_BBIS_MAX_DEVS; i++ ) {
    h->devId[i]      = CHAMELEON_NO_DEV;
  }

  /* automatic enumeration */
  if( h->autoEnum ){

    u_int8 empty = 0;

    /* get AUTOENUM_EXCLUDING (optional) */
    h->exclModCodesNbr = MAX_EXCL_MODCODES;
    status = DESC_GetBinary( h->descHdl, &empty, 0, h->exclModCodes,
			     &h->exclModCodesNbr, "AUTOENUM_EXCLUDINGV2");
    if( status == ERR_DESC_KEY_NOTFOUND ) {
      h->exclModCodesNbr = MAX_EXCL_MODCODES;
      status = DESC_GetBinary( h->descHdl, &empty, 0, h->exclModCodes,
			       &h->exclModCodesNbr, "AUTOENUM_EXCLUDING");
      if( !status ) {
	for( i=0; i < h->exclModCodesNbr; i++ ) {
	  h->exclModCodes[i]	= (u_int8)CHAM_ModCodeToDevId(h->exclModCodes[i]);
	}
      } else if( status == ERR_DESC_KEY_NOTFOUND ) {
	h->exclModCodesNbr = 0;
      }
    }

    if( status && status != ERR_DESC_KEY_NOTFOUND)
      return( Cleanup(h,status) );

  } else {	/* manual enumeration? */

		/* get DEVICE_ID(V2)_n, group 0*/
    for( i=0; i < CHAMELEON_BBIS_MAX_DEVS; i++ ){

      if( (status = DESC_GetUInt32( h->descHdl, 0, &value,
				    "DEVICE_IDV2_%d", i)) == ERR_SUCCESS )
	{
	  h->devId[i] = (u_int16)((value & 0xffffff00) >> 8);
	  h->inst[i]  = (int16)(value & 0xff);
	  h->idx[i]   = 0;
	} else if( (status = DESC_GetUInt32( h->descHdl, 0, &value,
					     "DEVICE_ID_%d", i)) == ERR_SUCCESS )
	{
	  u_int16 modId	= (u_int16)((value & 0xffffff00) >> 8);
	  h->inst[i]  = -1;
	  h->idx[i]   = value & 0xff;
	  h->devId[i] = CHAM_ModCodeToDevId( modId  );
	}
      if( status == ERR_SUCCESS)
	{
	  h->devCount++;
	  DBGWRT_2(( DBH, " DEVICE_ID(V2)_%d = 0x%x\n", i, h->devId[i] ));
	}
    }
    /* get GROUP_n/DEVICE_IDV2_n */
    for( g=0; g < CHAMELEON_BBIS_MAX_GRPS &&
	   h->devCount < CHAMELEON_BBIS_MAX_DEVS; g++ )
      {
	status = DESC_GetUInt32( h->descHdl, 0, &value,
				 "GROUP_%d/GROUP_ID", g);

	if( status == ERR_SUCCESS )
	  {
	    /* group exists in descriptor? get memory for group */
	    h->dev[g] = (BBIS_CHAM_GRP *)OSS_MemGet( h->osHdl,
						     sizeof( BBIS_CHAM_GRP ),
						     &h->devGotSize[g]);
	    if( !h->dev[g] ) {
	      DBGWRT_ERR((DBH, "*** %s_Init: no ressources\n",
			  BBNAME));
	      return(  Cleanup(h,ERR_BBIS_DESC_PARAM) );
	    }
	    OSS_MemFill( h->osHdl, sizeof( BBIS_CHAM_GRP ), (char*)h->dev[g], 0x00 );
	    devGrp = (BBIS_CHAM_GRP*)h->dev[g];

	    devGrp->devCount = 0;
	    devGrp->grpId    = value;
	    /* anounce group */
	    h->devId[g]  = CHAMELEON_BBIS_GROUP ;
	    h->devCount++;
	  } else {
	  continue;
	}

	for( i=0; i < CHAMELEON_BBIS_MAX_DEVS; i++ )
	  {

	    status = DESC_GetUInt32( h->descHdl, 0, &value,
				     "GROUP_%d/DEVICE_IDV2_%d", g, i);

	    if( status == ERR_SUCCESS)
	      {
		devGrp->devId[i] = (u_int16)((value & 0xffffff00) >> 8);
		devGrp->idx[i]   = (int16)value & 0xff;
		devGrp->devCount++;
		DBGWRT_2(( DBH, " GROUP_%d/DEVICE_IDV2_%d = 0x%x\n",
			   g, i, value ));
	      } else {
	      devGrp->devId[i]      = CHAMELEON_NO_DEV;
	    }
	  }
      }
    /*--- check if any device specified ---*/
    if( h->devCount == 0 ){
      DBGWRT_ERR((DBH, "*** %s_Init: No devices in descriptor!\n",
		  BBNAME));
      return( Cleanup(h,ERR_BBIS_DESC_PARAM) );
    }
  }

  /*------------------------------+
    |  create spinlock              |
    +-----------------------------*/

#ifdef VXWORKS
  h->slHdl = &h->vxSpinlock;
#endif

  status = OSS_SpinLockCreate( h->osHdl, &h->slHdl);
  if (status) {
    DBGWRT_ERR((DBH, "*** BB - %s_Init: OSS_SpinLockCreate() failed! "
		"Error 0x%0x\n",
		BBNAME, status ));
    return( Cleanup(h,status));
  }


  /* store current devCount value to ignore repeated calls of *_BrdInit
   * starting at updated count
   */
  h->devCountInit = h->devCount;
  return 0;
}

/****************************** CHAMELEON_BrdInit ***************************
 *
 *  Description:  Board initialization.
 *
 *  Look for chameleon FPGA.
 *  For each module specified in descriptor, look for that module and save
 *  information about it.
 *
 *---------------------------------------------------------------------------
 *  Input......:  h			pointer to board handle structure
 *  Output.....:  return    0 | error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 CHAMELEON_BrdInit(
			       BBIS_HANDLE     *h )
{
  CHAMELEONV2_HANDLE *chamHdl = NULL;	/* chameleon V2 handle */
  CHAMELEONV2_UNIT chamUnit;		/* unit info of current module */
  int32 chErr, error=0, i, u, n;
  u_int32 exclude;
  BBIS_CHAM_GRP *lGrp = NULL;
  u_int32 gotSize = 0, un;
  u_int8 groupBaseDevIncluded=0;

  DBGWRT_1((DBH, "BB - %s_BrdInit\n",BBNAME));
  /* PCIbus */
#ifndef CHAM_ISA
  DBGWRT_2((DBH," pci Domain: %d \n", h->pciDomainNbr));
#endif

  /* init chameleon lib - mem */
  if( (chErr = CHAM_InitMem( &h->chamFuncTbl[0] )) != CHAMELEON_OK ){
    DBGWRT_ERR((DBH, "*** %s_BrdInit: CHAM_InitMem error 0x%x!\n",
		BBNAME, chErr));
    error = ERR_BBIS_ILL_SLOT;
    goto ABORT_NO_CHAM;
  }

  /* init chameleon lib - io */
  if( (chErr = CHAM_InitIo( &h->chamFuncTbl[1] )) != CHAMELEON_OK ){
    DBGWRT_ERR((DBH, "*** %s_BrdInit: CHAM_InitIo error 0x%x!\n",
		BBNAME, chErr));
    error = ERR_BBIS_ILL_SLOT;
    goto ABORT_NO_CHAM;
  }

  /* PCIbus */
#ifndef CHAM_ISA

  /* try to find mem mapped table */
  h->tblType = OSS_ADDRSPACE_MEM;
  chErr = h->chamFuncTbl[OSS_ADDRSPACE_MEM].InitPci( h->osHdl,
						     OSS_MERGE_BUS_DOMAIN(h->pciBusNbr, h->pciDomainNbr),
						     h->pciDevNbr,
                                                     h->pciFuncNbr,
                                                     &chamHdl );

  if( chErr == CHAMELEONV2_TABLE_NOT_FOUND ){
    DBGWRT_2((DBH," no mem mapped table found, try to find io mapped table\n"));
    /* try to find io mapped table */
    h->tblType = OSS_ADDRSPACE_IO;
    chErr = h->chamFuncTbl[OSS_ADDRSPACE_IO].InitPci( h->osHdl,
						      OSS_MERGE_BUS_DOMAIN(h->pciBusNbr, h->pciDomainNbr),
						      h->pciDevNbr, h->pciFuncNbr, &chamHdl );
  }

  if( chErr != CHAMELEON_OK ){
    DBGWRT_ERR((DBH, "*** %s_BrdInit: CHAM_InitPci error 0x%x! "
		"(PciBus 0x%x, PciDev 0x%x)\n",
		BBNAME, chErr, h->pciBusNbr, h->pciDevNbr));
    error = ERR_BBIS_ILL_SLOT;
    goto ABORT;
  }
  /* ISAbus */
#else
  /* using mem/io function table according specified address type
     (DEVICE_ADDR_IO desc key) */
  if( (chErr = h->chamFuncTbl[h->tblType].InitInside( h->osHdl,
						      (void*)(U_INT32_OR_64)(h->isaAddr), &chamHdl ))
      != CHAMELEON_OK )
    {
      DBGWRT_ERR((DBH, "*** %s_BrdInit: CHAM_InitInside error 0x%x! "
		  "(isaAddr=0x%x)\n",
		  BBNAME, chErr, h->isaAddr));
      error = ERR_BBIS_ILL_SLOT;
      goto ABORT;
    }
#endif /* CHAM_ISA */

#ifdef DBG
  {
    /*
     * DBG: Always print chameleon-V2 table info about
     * the table of the bus where the caller resides
     */
    CHAMELEONV2_TABLE tbl;
    if( (error = h->chamFuncTbl[h->tblType].TableIdent( chamHdl, 0, &tbl )) ){
      DBGWRT_ERR((DBH, "*** %s_BrdInit: CHAM_TableIdent error 0x%x!\n",
		  BBNAME, error));
      error = ERR_BBIS;
      goto ABORT;
    }
    /* PCIbus */
#ifndef CHAM_ISA
    DBGWRT_ERR((DBH, "--- %s_BrdInit: PciDev=%d/%d/%d/%d: file=%s, model=%c, rev=0x%02x\n",
		BBNAME, h->pciDomainNbr, h->pciBusNbr, h->pciDevNbr, h->pciFuncNbr,
		tbl.file, tbl.model, tbl.revision));

    /* ISAbus */
#else
    DBGWRT_ERR((DBH, "--- %s_BrdInit: isaAddr=0x%x: file=%s, model=%c, rev=0x%02x\n",
		BBNAME, h->isaAddr,
		tbl.file, tbl.model, tbl.revision));
#endif /* CHAM_ISA */
  }
#endif /* DBG */

  /* restore current devCount value to init value counter.
   * *_BrdInit may be called multiple times and shall be started at equal counter
   */
  h->devCount = h->devCountInit;

  /* automatic enumeration? */
  if( h->autoEnum ){
    u_int8 excludedGroups[CHAMELEON_BBIS_MAX_GRPS];
    DBGWRT_2((DBH," perform automatic enumeration\n"));

    excludedGroups[0] = 0;

    for( u=0; h->devCount < CHAMELEON_BBIS_MAX_DEVS; u++ ){

      /* get unit info */
      chErr = h->chamFuncTbl[h->tblType].UnitIdent( chamHdl, u, &chamUnit );

      /* no unit? => leave loop */
      if( chErr == CHAMELEONV2_NO_MORE_ENTRIES )
	break;

      /* group? */
      if( chamUnit.group != 0 ){
	groupBaseDevIncluded = 0;
	/* group already existant? */
	for( n=0; n < h->devCount; n++) {
	  if( h->devId[n] == CHAMELEON_BBIS_GROUP &&
	      ((BBIS_CHAM_GRP *)h->dev[n])->grpId == chamUnit.group){
	    groupBaseDevIncluded = 1;
	    break;
	  }
	}
      }

      exclude = 0;

      /* no group OR base device of group not yet included */
      if( (chamUnit.group == 0) || (groupBaseDevIncluded == 0) ){

	/* excluding specified module codes */
	for( un=0; un < h->exclModCodesNbr; un++ )
	{
		if( chamUnit.devId == h->exclModCodes[un] ){
			DBGWRT_2((DBH," unit %d: devId=0x%x excluded\n", u, h->exclModCodes[un] ));

			exclude = 1;
			if( chamUnit.group != 0 ) {
				/* group, exclude also rest of group members */
				for( i=0; i < CHAMELEON_BBIS_MAX_GRPS - 1; i++) {
					if( excludedGroups[i] == 0 ) {
						/* end of list, first module of group */
						excludedGroups[i] = (u_int8)chamUnit.group;
						excludedGroups[i+1] = 0; /* mark end of list */
						break;
					}
				}
			}
	    break;
		}
	}

	/* excluding members of groups marked for excluding */
	if( !exclude && chamUnit.group != 0 ) {
	  for( i=0; excludedGroups[i] != 0 && 			/* end of list? */
		 i < CHAMELEON_BBIS_MAX_GRPS - 1; i++)
	    {
	      if( excludedGroups[i] == chamUnit.group ) {
		exclude = 1;
		break;
	      }
	    }
	}
      }

      /* module should be used? no group */
      if( !exclude && chamUnit.group == 0 )
	{
	  DBGWRT_2(( DBH, " DEVICE_IDV2_%d = 0x%x\n",
		     h->devCount, chamUnit.devId ));

	  h->dev[h->devCount] = OSS_MemGet( h->osHdl,
					    sizeof(CHAMELEONV2_UNIT),
					    &h->devGotSize[h->devCount]);
	  if( !h->dev[h->devCount] ) {
	    DBGWRT_ERR((DBH, "*** %s_Init: no ressources f. chamUnit\n", BBNAME));
	    error = ERR_OSS_MEM_ALLOC;
	    goto ABORT;
	  }
	  h->devId[h->devCount] = chamUnit.devId;

	  OSS_MemCopy( h->osHdl, sizeof( chamUnit ),
		       (char*)&chamUnit,
		       (char*)h->dev[h->devCount]);
	  h->devCount++;
	  /* module should be used? group */
	} else if( !exclude ) {
	/* group already existant? */
	for( n=0; n < h->devCount; n++) {
	  if( h->devId[n] == CHAMELEON_BBIS_GROUP &&
	      ((BBIS_CHAM_GRP *)h->dev[n])->grpId == chamUnit.group)
	    {
	      lGrp = (BBIS_CHAM_GRP *)h->dev[n];
	      if( lGrp->devCount < CHAMELEON_BBIS_MAX_DEVS ) {
		/* attach module to group */
		lGrp->dev[lGrp->devCount] = OSS_MemGet( h->osHdl,
							sizeof(CHAMELEONV2_UNIT),
							&lGrp->devGotSize[lGrp->devCount]);
		if( !lGrp->dev[lGrp->devCount] ) {
		  DBGWRT_ERR((DBH, "*** %s_Init: no ressources f. chamUnit in group\n", BBNAME));
		  error = ERR_OSS_MEM_ALLOC;
		  goto ABORT;
		}
		OSS_MemCopy( h->osHdl, sizeof( chamUnit ),
			     (char*)&chamUnit,
			     (char*)lGrp->dev[lGrp->devCount]);
		lGrp->devId[lGrp->devCount] = chamUnit.devId;
		lGrp->devCount++;
		DBGWRT_2(( DBH, " GROUP_%d/DEVICE_IDV2_%d = 0x%x\n",
			   chamUnit.group, lGrp->devCount, chamUnit.devId ));
	      } else {
		DBGWRT_ERR((DBH, "*** %s_BrdInit: too many devices"
			    " in group %d\n", BBNAME, lGrp->grpId));
	      }
	      break;
	    }
	}

	if( n == h->devCount &&
	    h->devCount < CHAMELEON_BBIS_MAX_DEVS )
	  {
	    /* no group yet for this module, get mem for new group */
	    h->dev[h->devCount] = (BBIS_CHAM_GRP *)OSS_MemGet( h->osHdl,
							       sizeof( BBIS_CHAM_GRP ),
							       &h->devGotSize[h->devCount]);
	    if( !h->dev[h->devCount] ) {
	      DBGWRT_ERR((DBH, "*** %s_BrdInit: no ressources\n",
			  BBNAME));
	      error = ERR_OSS_MEM_ALLOC;
	      goto ABORT;
	    }
	    OSS_MemFill( h->osHdl, sizeof( BBIS_CHAM_GRP ), (char*)h->dev[h->devCount], 0x00 );

	    lGrp = (BBIS_CHAM_GRP*)h->dev[h->devCount];
	    lGrp->grpId    = chamUnit.group;
	    for( i=0; i<CHAMELEON_BBIS_MAX_DEVS; i++ ) {
	      lGrp->devId[i]      = CHAMELEON_NO_DEV;
	    }


	    /* anounce group */
	    h->devId[h->devCount] = CHAMELEON_BBIS_GROUP;
	    h->devCount++;

	    /* add module to new group */
	    lGrp->dev[0] = OSS_MemGet( h->osHdl,
				       sizeof(CHAMELEONV2_UNIT),
				       &lGrp->devGotSize[0]);
	    if( !lGrp->dev[0] ) {
	      DBGWRT_ERR((DBH, "*** %s_Init: no ressources f. chamUnit in group\n", BBNAME));
	      error = ERR_OSS_MEM_ALLOC;
	      goto ABORT;
	    }
	    OSS_MemCopy( h->osHdl, sizeof( CHAMELEONV2_UNIT ),
			 (char*)&chamUnit,
			 (char*)lGrp->dev[0]);
	    lGrp->devId[0] = chamUnit.devId;
	    lGrp->devCount = 1;
	    DBGWRT_2(( DBH, " GROUP_%d/DEVICE_IDV2_%d = 0x%x\n", chamUnit.group, 1, chamUnit.devId ));
	  }
      }
    }
  } else {
    /* locate modules */
    CHAMELEONV2_FIND	chamFind;
    int idx;

    chamFind.variant  = -1;
    chamFind.busId    = -1;
    chamFind.bootAddr = -1;

    for( i=0; i < CHAMELEON_BBIS_MAX_DEVS; i++ ){

      if( h->devId[i] == CHAMELEON_NO_DEV )
	continue;

      /* do we handle a group? */
      if( h->devId[i] == CHAMELEON_BBIS_GROUP )
	{
	  /* run through group */
	  lGrp = (BBIS_CHAM_GRP *)h->dev[i];
	  chamFind.group = (int16)lGrp->grpId;
	  chamFind.instance = -1; /* not used, instead use index of dev */

	  for( n=0; n < lGrp->devCount && n < CHAMELEON_BBIS_MAX_DEVS; n++ ){
	    chamFind.devId	  = lGrp->devId[n];
	    idx 			  = lGrp->idx[n];

	    lGrp->dev[n] = OSS_MemGet( h->osHdl,
				       sizeof(CHAMELEONV2_UNIT),
				       &lGrp->devGotSize[n]);
	    if( !lGrp->dev[n] ) {
	      DBGWRT_ERR((DBH, "*** %s_Init: no ressources f. chamUnit in group\n", BBNAME));
	      error = ERR_OSS_MEM_ALLOC;
	      goto ABORT;
	    }

	    DBGWRT_2((DBH," looking for devId=0x%x grp %d idx %d\n", lGrp->devId[n], lGrp->grpId, idx));

	    if( (chErr = h->chamFuncTbl[h->tblType].InstanceFind(
								 chamHdl, idx, chamFind,
								 (CHAMELEONV2_UNIT*)lGrp->dev[n],
								 NULL, NULL))
		!= CHAMELEONV2_UNIT_FOUND )
	      {
		DBGWRT_ERR((DBH, "*** %s_BrdInit: can't find "
			    "devId=0x%x group=%d index %d\n",
			    BBNAME, lGrp->devId[n], lGrp->grpId, idx ));

		/* flag slot unusuable */
		h->devId[i] = CHAMELEON_NO_DEV;
	      }
	  }

	} else { /* normal device, no group */
	chamFind.devId	  = h->devId[i];
	chamFind.group    = 0;
	chamFind.instance = h->inst[i];
	idx 			  = h->idx[i];

	DBGWRT_2((DBH," looking for devId=0x%x index %d\n",
		  chamFind.devId, idx ));

	if( (chErr = h->chamFuncTbl[h->tblType].InstanceFind(
							     chamHdl, idx, chamFind,
							     &chamUnit, NULL, NULL))
	    == CHAMELEONV2_UNIT_FOUND )
	  {
	    h->dev[i] = OSS_MemGet( h->osHdl,
				    sizeof( CHAMELEONV2_UNIT ),
				    &gotSize);
	    if( !h->dev[i] ) {
	      DBGWRT_ERR((DBH, "*** %s_BrdInit: no ressources\n",
			  BBNAME));
	      error = ERR_OSS_MEM_ALLOC;
	      goto ABORT;
	    }
	    h->devGotSize[i] = gotSize;

	    OSS_MemCopy( h->osHdl, sizeof( chamUnit ),
			 (char*)&chamUnit,
			 (char*)h->dev[i] );
	  } else {
	  DBGWRT_ERR((DBH, "*** %s_BrdInit: can't find devId=0x%x "
		      "group 0 instance %d (chErr = 0x%x)\n",
		      BBNAME, chamFind.devId, chamFind.instance, chErr ));

	  h->devId[i] = CHAMELEON_NO_DEV;	/* flag slot unusuable */
	}
      }
    }
  }
#ifdef CHAMELEON_BBIS_DEBUG
  {
    for( i=0; i < CHAMELEON_BBIS_MAX_DEVS; i++ ){
      if( h->devId[i] == CHAMELEON_BBIS_GROUP ){
	lGrp = (BBIS_CHAM_GRP*)h->dev[i];
	for( n=0; n < CHAMELEON_BBIS_MAX_DEVS; n++ ){
	  if( lGrp->devId[n] != CHAMELEON_NO_DEV )
	    {
	      DBGWRT_2((DBH," DMP: GRP_%d/DEVICE_%d: grpId %d devId 0x%x inst %d addr %08p size 0x%08x\n",
			i, n, lGrp->grpId,
			((CHAMELEONV2_UNIT*)lGrp->dev[n])->devId,
			((CHAMELEONV2_UNIT*)lGrp->dev[n])->instance,
			((CHAMELEONV2_UNIT*)lGrp->dev[n])->addr,
			((CHAMELEONV2_UNIT*)lGrp->dev[n])->size ));
	    }
	}
      }
      else if( h->devId[i] != CHAMELEON_NO_DEV )
	{
	  DBGCMD( CHAMELEONV2_UNIT *lUnit = (CHAMELEONV2_UNIT*)h->dev[i] );
	  DBGWRT_2((DBH," DMP: DEVICE_%d: devId 0x%x inst %d addr %08p size 0x%08x\n",
		    i, lUnit->devId, lUnit->instance, lUnit->addr, lUnit->size ));
	}
    }
  }
#endif /* CHAMELEON_BBIS_DEBUG */

  /*------------------------------------------------------------+
    | Get global info to determine BAR mapping                    |
    +------------------------------------------------------------*/
  if( (error = h->chamFuncTbl[h->tblType].Info( chamHdl, &h->chamInfo ) )){
    DBGWRT_ERR((DBH, "*** %s_BrdInit: CHAM_Info error 0x%x\n",
		BBNAME, error));
    error = ERR_BBIS;
    goto ABORT;
  }

  /*------------------------------------------------------------+
    | GIRQ UNIT: check if FPGA has girq unit                      |
    +------------------------------------------------------------*/
  {
    CHAMELEONV2_FIND	_find;
    CHAMELEONV2_UNIT	_unit;
    u_int32				irqenLower;
    u_int32				irqenUpper;

    OSS_MemFill( h->osHdl, sizeof( _find ), (char*)&_find, 0x00 );
    _find.devId = CHAM_ModCodeToDevId(CHAMELEON_16Z052_GIRQ);

    /* get GIRQ address */
    if( (chErr = h->chamFuncTbl[h->tblType].InstanceFind(
							 chamHdl, 0, _find, &_unit, NULL, NULL))
	== CHAMELEONV2_UNIT_FOUND )
      {
	h->girqPhysAddr = (char*)_unit.addr;
	h->girqType = h->chamInfo.ba[_unit.bar].type;

	/* map address - address space MEM and bus type PCI
	   must be adapted if it will be used for i.e. M199 */
	error = OSS_MapPhysToVirtAddr( h->osHdl, h->girqPhysAddr,
				       BBCHAM_GIRQ_SPACE_SIZE,
				       h->girqType, /* 0=mem, 1=io */
				       BUSTYPE,
				       _unit.busId /* pci bus number */,
				       (void**) &h->girqVirtAddr );
	if( error )
	  {
	    DBGWRT_ERR((DBH," *** %s_Init: OSS_MapPhysToVirtAddr() girqPhysAddr %08p failed\n",
			BBNAME, h->girqPhysAddr ));
	    goto ABORT;
	  }/*if*/

	_MREAD_D32(irqenLower, h->girqVirtAddr, BBCHAM_GIRQ_IRQ_EN);
	_MREAD_D32(irqenUpper, h->girqVirtAddr, BBCHAM_GIRQ_IRQ_EN + 4);
	_MREAD_D32(h->girqApiVersion, h->girqVirtAddr, BBCHAM_GIRQ_API_VER );
#ifdef	_BIG_ENDIAN_
	irqenLower = OSS_SWAP32( irqenLower );
	irqenUpper = OSS_SWAP32( irqenUpper );
	h->girqApiVersion = OSS_SWAP32( h->girqApiVersion );
#endif
	/* get api version from topmost byte */
	h->girqApiVersion = h->girqApiVersion >> BBCHAM_GIRQ_API_VER_OFF;

	DBGWRT_1((DBH, "%s_BrdInit: girq found at phys %08p virt %08p - "
		  "IRQEN current setting %08x %08x, api version 0x%08x\n",
		  BBNAME, h->girqPhysAddr, h->girqVirtAddr, irqenLower,
		  irqenUpper, h->girqApiVersion ));
      }
    else
      {
	DBGWRT_1((DBH, "%s_BrdInit: has no GIRQ unit\n", BBNAME ));
      }
  }

 ABORT:
  /* when chameleon library initialized: terminate it */
  if( chamHdl )
    h->chamFuncTbl[h->tblType].Term( &chamHdl );

 ABORT_NO_CHAM:
  if( (error != ERR_SUCCESS) &&
      h->girqVirtAddr )
    {
      /* unmap if it was mapped before */
      int32 err2 = OSS_UnMapVirtAddr( h->osHdl, (void**)&h->girqVirtAddr,
				      BBCHAM_GIRQ_SPACE_SIZE, h->girqType );
      if( err2 )
	{
	  DBGWRT_ERR((DBH,"*** %s_Init: OSS_UnMapVirtAddr() girqVirtAddr %08p failed\n",
		      BBNAME, h->girqVirtAddr ) );
	}
    }

  return error;
}

/****************************** CHAMELEON_BrdExit ****************************
 *
 *  Description:  Board deinitialization.
 *
 *                Do nothing
 *
 *---------------------------------------------------------------------------
 *  Input......:  h			pointer to board handle structure
 *  Output.....:  return    0 | error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 CHAMELEON_BrdExit(
			       BBIS_HANDLE     *h )
{
  int32 error = 0;
  u_int32 i,j;
  BBIS_CHAM_GRP *lGrp;
  DBGWRT_1((DBH, "BB - %s_BrdExit\n",BBNAME));

  if( h->girqVirtAddr )
    {
      error = OSS_UnMapVirtAddr( h->osHdl, (void**)&h->girqVirtAddr,
				 BBCHAM_GIRQ_SPACE_SIZE, h->girqType );
      if( error )
	{
	  DBGWRT_ERR((DBH,"*** %s_Init: OSS_UnMapVirtAddr() girqVirtAddr %08p failed\n",
		      BBNAME, h->girqVirtAddr ) );
	  goto CLEANUP;
	}
    }

  /*---------------------------------+
    |  free memory alloced by BrdInit |
    +--------------------------------*/
  /* release memory for devices and groups */
  for( i = 0; i < CHAMELEON_BBIS_MAX_DEVS; i++ ) {
    if( h->devId[i] == CHAMELEON_BBIS_GROUP ) {
      /* free dev[] in group */
      lGrp = (BBIS_CHAM_GRP*)h->dev[i];
      for( j = 0; j < CHAMELEON_BBIS_MAX_DEVS; j++ ) {
    	  if( lGrp->dev[j] ){
    		  OSS_MemFree(h->osHdl, lGrp->dev[j], lGrp->devGotSize[j]);
    		  lGrp->dev[j] = NULL;
    	  }
      }
    }
  }
 CLEANUP:
  return( error );
}

/****************************** CHAMELEON_Exit *******************************
 *
 *  Description:  Cleanup memory.
 *
 *                - deinitializes the bbis handle
 *
 *---------------------------------------------------------------------------
 *  Input......:  hP		pointer to board handle structure
 *  Output.....:  *hP		NULL
 *                return    0 | error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 CHAMELEON_Exit(
			    BBIS_HANDLE     **hP )
{
  BBIS_HANDLE	*h = *hP;
  int32		error = 0;

  DBGWRT_1((DBH, "BB - %s_Exit\n",BBNAME));

  /*------------------------------+
    |  de-init hardware             |
    +------------------------------*/
  /* nothing to do */

  /*------------------------------+
    |  cleanup memory               |
    +------------------------------*/
  error = Cleanup(h, error);
  *hP = NULL;

  return error;
}

/****************************** CHAMELEON_BrdInfo ****************************
 *
 *  Description:  Get information about hardware and driver requirements.
 *
 *                Following info codes are supported:
 *
 *                Code                      Description
 *                ------------------------  -----------------------------
 *                BBIS_BRDINFO_BUSTYPE      board bustype
 *                BBIS_BRDINFO_DEVBUSTYPE   device bustype
 *                BBIS_BRDINFO_FUNCTION     used optional functions
 *                BBIS_BRDINFO_NUM_SLOTS    number of slots
 *				  BBIS_BRDINFO_INTERRUPTS   interrupt characteristics
 *                BBIS_BRDINFO_ADDRSPACE    address characteristic
 *				  BBIS_BRDINFO_BRDNAME		name of the board hardware
 *
 *                The BBIS_BRDINFO_BUSTYPE code returns the bustype of
 *                the specified board. (here always PCI)
 *
 *                The BBIS_BRDINFO_DEVBUSTYPE code returns the bustype of
 *                the specified device - not the board bus type.
 * 				  (here always NONE)
 *
 *                The BBIS_BRDINFO_FUNCTION code returns the information
 *                if an optional BBIS function is supported or not.
 *
 *                The BBIS_BRDINFO_NUM_SLOTS code returns the number of
 *                devices used from the driver. (CHAMELEON BBIS: always 16)
 *
 *                The BBIS_BRDINFO_INTERRUPTS code returns the supported
 *                interrupt capability (BBIS_IRQ_DEVIRQ/BBIS_IRQ_EXPIRQ)
 *                of the specified device.
 *
 *                The BBIS_BRDINFO_ADDRSPACE code returns the address
 *                characteristic (OSS_ADDRSPACE_MEM/OSS_ADDRSPACE_IO)
 *                of the specified device.
 *                The 2011 introduced BBIS_CFGINFO_ADDRSPACE code for
 *                <BBIS>_CfgInfo can be used to specify the address
 *                characteristic of the specified device with the help
 *                of the board handle.
 *
 *				  The BBIS_BRDINFO_BRDNAME code returns the short hardware
 *                name and type of the board without any prefix or suffix.
 *                The hardware name must not contain any non-printing
 *                characters. The length of the returned string, including
 *                the terminating null character, must not exceed
 *                BBIS_BRDINFO_BRDNAME_MAXSIZE.
 *                Examples: D201 board, PCI device, Chameleon FPGA
 *
 *---------------------------------------------------------------------------
 *  Input......:  code      reference to the information we need
 *                ...       variable arguments
 *  Output.....:  *...      variable arguments
 *                return    0 | error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 CHAMELEON_BrdInfo(
			       u_int32 code,
			       ... )
{
  int32		status = ERR_SUCCESS;
  va_list     argptr;

  va_start(argptr,code);

  switch ( code ) {

    /* supported functions */
  case BBIS_BRDINFO_FUNCTION:
    {
      u_int32 funcCode = va_arg( argptr, u_int32 );
      u_int32 *used = va_arg( argptr, u_int32* );

      /* no optional BBIS function do anything */
      *used = FALSE;
      funcCode = 0; /*dummy*/
      break;
    }

    /* number of devices */
  case BBIS_BRDINFO_NUM_SLOTS:
    {
      u_int32 *numSlot = va_arg( argptr, u_int32* );

      /*
       * No board handle here, return maximum
       */
      *numSlot = CHAMELEON_BBIS_MAX_DEVS;
      break;
    }

    /* bus type */
  case BBIS_BRDINFO_BUSTYPE:
    {
      u_int32 *busType = va_arg( argptr, u_int32* );

      *busType = BUSTYPE;
      break;
    }

    /* device bus type */
  case BBIS_BRDINFO_DEVBUSTYPE:
    {
      u_int32 mSlot       = va_arg( argptr, u_int32 );
      u_int32 *devBusType = va_arg( argptr, u_int32* );

      /* MDIS4 for Windows: MK does not support OSS_BUSTYPE_CHAM */
#if ( defined(WINNT) && !defined(_MDIS5_) )
      *devBusType = OSS_BUSTYPE_NONE;
#else
      *devBusType = OSS_BUSTYPE_CHAM;
#endif
      mSlot = 0xffffffff; /*dummy*/
      break;
    }

    /* interrupt capability */
  case BBIS_BRDINFO_INTERRUPTS:
    {
      u_int32 mSlot = va_arg( argptr, u_int32 );
      u_int32 *irqP = va_arg( argptr, u_int32* );

      *irqP = BBIS_IRQ_DEVIRQ;
      mSlot = 0xffffffff; /*dummy*/
      break;
    }

    /* address space type */
  case BBIS_BRDINFO_ADDRSPACE:
    {
      u_int32 mSlot      = va_arg( argptr, u_int32 );
      u_int32 *addrSpace = va_arg( argptr, u_int32* );

      mSlot = mSlot; /* dummy access to avoid compiler warning */
      /* Note: BBIS_CFGINFO_ADDRSPACE overwrites this! */
      /* support for old chameleon io variants */
#ifdef OLD_IO_VARIANT
      *addrSpace = OSS_ADDRSPACE_IO;
#else
      *addrSpace = OSS_ADDRSPACE_MEM;
#endif
      break;
    }

    /* board name */
  case BBIS_BRDINFO_BRDNAME:
    {
      char	*brdName = va_arg( argptr, char* );
      char	*from;

      /*
       * build hw name
       */
      from = "Chameleon FPGA";
      while( (*brdName++ = *from++) );	/* copy string */
      break;
    }

    /* error */
  default:
    status = ERR_BBIS_UNK_CODE;
  }

  va_end( argptr );
  return status;
}

/****************************** CHAMELEON_CfgInfo ****************************
 *
 *  Description:  Get information about board configuration.
 *
 *                Following info codes are supported:
 *
 *                Code                      Description
 *                ------------------------  ------------------------------
 *                BBIS_CFGINFO_BUSNBR       PCI bus number
 *                BBIS_CFGINFO_PCI_DOMAIN	PCI domain number
 *                BBIS_CFGINFO_IRQ          interrupt parameters
 *                BBIS_CFGINFO_EXP          exception interrupt parameters
 *                BBIS_CFGINFO_SLOT			slot information
 *                BBIS_CFGINFO_ADDRSPACE    address characteristic
 *
 *                The BBIS_CFGINFO_BUSNBR code returns the number of the
 *                bus on which the specified device resides
 *
 *                The BBIS_CFGINFO_PCI_DOMAIN code returns the number of
 *                the PCI domain on which the specified device resides
 *                (introduced 2011)
 *
 *                The BBIS_CFGINFO_IRQ code returns the device interrupt
 *                vector, level and mode of the specified device.
 *
 *                The BBIS_CFGINFO_EXP code returns the exception interrupt
 *                vector, level and mode of the specified device.
 *
 *                The BBIS_CFGINFO_SLOT code returns the following
 *                information about the specified device slot:
 *                The slot is occupied or empty, the device id and device
 *                revision of the plugged device, the name of the slot and
 *                the name of the plugged device.
 *
 *                The optional BBIS_CFGINFO_ADDRSPACE code (introduced 2011)
 *                returns the address characteristic (OSS_ADDRSPACE_MEM/
 *                OSS_ADDRSPACE_IO) of the specified device with the help
 *                of the board handle and overwrites the address
 *                characteristic specified by BBIS_BRDINFO_ADDRSPACE.
 *
 *---------------------------------------------------------------------------
 *  Input......:  h			pointer to board handle structure
 *                code      reference to the information we need
 *                ...       variable arguments
 *  Output.....:  ...       variable arguments
 *                return    0 | error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 CHAMELEON_CfgInfo(
			       BBIS_HANDLE     *h,
			       u_int32         code,
			       ... )
{
  va_list		argptr;
  int32       status=0;

    DBGWRT_1((DBH, "BB - %s_CfgInfo\n",BBNAME));

  va_start(argptr,code);

  switch ( code ) {
    /* bus number */
  case BBIS_CFGINFO_BUSNBR:
    {
      u_int32 *busNbr = va_arg( argptr, u_int32* );
      u_int32 mSlot   = va_arg( argptr, u_int32 );

      if ( mSlot > CHAMELEON_BBIS_MAX_DEVS - 1 )
      {
    	  DBGWRT_ERR((DBH,"*** %s_CfgInfo: mSlot out of range! (mslot = 0x%08x)\n", BBNAME, mSlot));
    	  return ERR_BBIS_ILL_PARAM; /*safe, no resources allocated till here */
      }

      if (h->devId[mSlot] == CHAMELEON_NO_DEV )
    	  status = ERR_BBIS_ILL_SLOT;
      else
	/* PCIbus */
#ifndef CHAM_ISA
	*busNbr = h->pciBusNbr;
      /* ISAbus */
#else
      *busNbr = 0;
#endif /* CHAM_ISA */

      break;
    }

    /* domain number */
  case BBIS_CFGINFO_PCI_DOMAIN:
    {
      u_int32 *domainNbr = va_arg( argptr, u_int32* );
      u_int32 mSlot      = va_arg( argptr, u_int32 );

      if ( mSlot > CHAMELEON_BBIS_MAX_DEVS - 1 ) {
    	  DBGWRT_ERR((DBH,"*** %s_CfgInfo: mSlot out of range! (mslot = 0x%08x)\n", BBNAME, mSlot));
    	  return ERR_BBIS_ILL_PARAM; /*safe, no resources allocated till here */
      }

      if ( (h->devId[mSlot] == CHAMELEON_NO_DEV ))
	status = ERR_BBIS_ILL_SLOT;
      else
	/* PCIbus */
#ifndef CHAM_ISA
	*domainNbr = h->pciDomainNbr;
      /* ISAbus */
#else
      *domainNbr = 0;
#endif /* CHAM_ISA */

      break;
    }

    /* interrupt information */
  case BBIS_CFGINFO_IRQ:
    {
      u_int32 mSlot   = va_arg( argptr, u_int32 );
      u_int32 *vector = va_arg( argptr, u_int32* );
      u_int32 *level  = va_arg( argptr, u_int32* );
      u_int32 *mode   = va_arg( argptr, u_int32* );

      if ( mSlot > CHAMELEON_BBIS_MAX_DEVS - 1 ) {
    	  DBGWRT_ERR((DBH,"*** %s_CfgInfo: mSlot out of range! (mslot = 0x%08x)\n", BBNAME, mSlot));
    	  return ERR_BBIS_ILL_PARAM; /*safe, no resources allocated till here */
      }

      if (h->devId[mSlot] == CHAMELEON_NO_DEV )
      {
    	  status = ERR_BBIS_ILL_SLOT;
      }
      else {
	u_int16 chamTblInt=0;

	/* predefine to not BBIS_IRQ_NONE that if condition below works */
	*mode = !BBIS_IRQ_NONE;

	if( h->devId[mSlot] == CHAMELEON_BBIS_GROUP )
	  chamTblInt = ((CHAMELEONV2_UNIT*)((BBIS_CHAM_GRP*)h->dev[mSlot])->dev[0])->interrupt;
	else if( h->devId[mSlot] != CHAMELEON_NO_DEV )
	  chamTblInt = ((CHAMELEONV2_UNIT *)h->dev[mSlot])->interrupt;

	/* module does not have interrupt possibilities? */
	if( chamTblInt == 0x3F ){
	  *mode = BBIS_IRQ_NONE;
	}
	/* using irq level from chameleon table (may be overwritten below!) */
	else {
	  *level = chamTblInt;
	}

	/* ISA variant */
#ifdef CHAM_ISA
	/* IRQ_NUMBER specified in descriptor? */
	if( h->isaIrqNbr != TABLE_IRQ ){
	  /*
	   * Use irq level from descriptor key IRQ_NUMBER
	   * instead from table inside FPGA.
	   */

	  /* interrupt connected? */
	  if( h->isaIrqNbr ){
	    *level = h->isaIrqNbr;
	  }
	  /* no interrupt */
	  else{
	    *mode = BBIS_IRQ_NONE;
	  }
	}

	/* interrupt used? */
	if( *mode != BBIS_IRQ_NONE ){

	  // share always (for ser IRQs at SC24 LPC bus)
	  *mode = BBIS_IRQ_SHARED;

	  /* maybe an alternative? */
#if 0
	  /* non shared legacy IRQ0..15? */
	  if( *level < 16 )
	    *mode = BBIS_IRQ_EXCLUSIVE;
	  /* shared IRQ16..max */
	  else
	    *mode = BBIS_IRQ_SHARED;
#endif
	}

	/* default (PCI variant) */
#else
	*mode = BBIS_IRQ_SHARED;

#ifdef CHAMELEON_USE_PCITABLE
	/*
	 * Take irq level from PCI config space instead from
	 * table inside FPGA (normal use case, except e.g. EM08).
	 */
	OSS_PciGetConfig( h->osHdl,
			  OSS_MERGE_BUS_DOMAIN(h->pciBusNbr, h->pciDomainNbr),
			  h->pciDevNbr, 0, OSS_PCI_INTERRUPT_LINE, (int32*)level );

	/* no interrupt available ? */
	if ( *level == 0xff )
	  *mode = BBIS_IRQ_NONE;
#endif /* CHAMELEON_USE_PCITABLE */

#endif
	OSS_IrqLevelToVector( h->osHdl, BUSTYPE,
			      *level, (int32 *)vector );

	DBGWRT_2((DBH, " mSlot=%d : IRQ mode=0x%x,"
		  " level=0x%x, vector=0x%x\n",
		  mSlot, *mode, *level, *vector));
      }
      break;
    }

    /* exception interrupt information */
  case BBIS_CFGINFO_EXP:
    {
      u_int32 mSlot   = va_arg( argptr, u_int32 );
      u_int32 *vector = va_arg( argptr, u_int32* );
      u_int32 *level  = va_arg( argptr, u_int32* );
      u_int32 *mode   = va_arg( argptr, u_int32* );

      mSlot = mSlot;		/* dummy access to avoid compiler warning */
      *vector = *vector;	/* dummy access to avoid compiler warning */
      *level = *level;	/* dummy access to avoid compiler warning */
      *mode = 0;			/* no extra exception interrupt */
      break;
    }

    /* slot information for PnP support*/
  case BBIS_CFGINFO_SLOT:
    {
      status = CfgInfoSlot( h, argptr );
      break;
    }

    /* address space type */
  case BBIS_CFGINFO_ADDRSPACE:
    {
      u_int32 mSlot      = va_arg( argptr, u_int32 );
      u_int32 *addrSpace = va_arg( argptr, u_int32* );
      CHAMELEONV2_UNIT	*unitP;

      if (mSlot > CHAMELEON_BBIS_MAX_DEVS - 1) { /* klocwork 2nd id127 */
       		status = ERR_BBIS_ILL_SLOT;
    		break;
      }

      if (h->devId[mSlot] == CHAMELEON_NO_DEV ) {
    	  status = ERR_BBIS_ILL_SLOT;
    	  break;
      }

      if( h->devId[mSlot] == CHAMELEON_BBIS_GROUP )
	/* use first module of group */
	unitP = (CHAMELEONV2_UNIT*)(((BBIS_CHAM_GRP*)h->dev[mSlot])->dev[0]);
      else
	unitP = (CHAMELEONV2_UNIT*)h->dev[mSlot];

      /* Note: overwrites BBIS_BRDINFO_ADDRSPACE */
      *addrSpace = h->chamInfo.ba[unitP->bar].type;
      break;
    }

    /* error */
  default:
    DBGWRT_ERR((DBH,"*** %s_CfgInfo: code=0x%x not supported\n",
		BBNAME,code));
    va_end( argptr );
    return ERR_BBIS_UNK_CODE;
  }

  va_end( argptr );
  return status;
}

/****************************** CHAMELEON_IrqEnable **************************
 *
 *  Description:  Chameleon BBIS Interrupt enable / disable for the unit.
 *
 *---------------------------------------------------------------------------
 *  Input......:  h			pointer to board handle structure
 *                slot      unit slot number
 *                enable    interrupt setting
 *  Output.....:  return    0
 *  Globals....:  ---
 ****************************************************************************/
static int32 CHAMELEON_IrqEnable(
				 BBIS_HANDLE     *h,
				 u_int32         slot,
				 u_int32         enable )
{
  DBGCMD(	static const char functionName[] = "_IrqEnable:"; )
  int32	error = 0;
  u_int32 irqen = 0x00000000;
  u_int32 irqenLittleEndian = 0x00000000;
  u_int32 irqen_readback = 0x00000000;
  int i = 0;
  int slotShift;

  DBGWRT_1((DBH, "BB - %s %s: slot=%d; enable=%d\n", BBNAME,functionName,slot,enable ));

  if( h->girqVirtAddr )
    {

      int			offs		= 0;
      u_int32		girqInUse	= 0; /* GIRQ hardware Spin Lock */

      if( h->devId[slot] == CHAMELEON_BBIS_GROUP )
	slotShift = ((CHAMELEONV2_UNIT*)((BBIS_CHAM_GRP*)h->dev[slot])->dev[0])->interrupt;
      else if( h->devId[slot] != CHAMELEON_NO_DEV )
	slotShift = ((CHAMELEONV2_UNIT *)h->dev[slot])->interrupt;
      else
	{
	  error = ERR_BBIS_ILL_IRQPARAM;
	  DBGWRT_ERR((DBH, "*** BB - %s%s: no CHAMELEON_BBIS_GROUP\n", BBNAME,functionName ));
	  goto CLEANUP;
	}

      /* upper 32 bit ? */
      if( slotShift > 31 )
	{
	  /* next address - irq enable has 64 bit */
	  offs 		= 4;
	  slotShift  -= 32;
	}

      /* lock critical section by spinlock to be multiprocessor safe */
      error = OSS_SpinLockAcquire( h->osHdl, h->slHdl );
      if (error)
	{
	  DBGWRT_ERR((DBH, "*** BB - %s%s: OSS_SpinLockAcquire() failed!"
		      "Error 0x%0x\n",
		      BBNAME, functionName, error ));
	  goto CLEANUP;
	}

      /* GIRQ INUSE_STS bit available */
      if ( h->girqApiVersion ) {
	u_int32 girqCount = 0;

	/* check INUSE bit */
	_MREAD_D32(girqInUse, h->girqVirtAddr, BBCHAM_GIRQ_IN_USE);
#ifdef  _BIG_ENDIAN_
	girqInUse = OSS_SWAP32( girqInUse );
#endif
	/* GIRQ INUSE bit is 0 when no other device uses the register
	 * if bit is 1 wait until released
	 * release INUSE bit by writing 1 to the register
	 */
	while ( (girqInUse & BBCHAM_GIRQ_IN_USE_BIT) )
	  {
	    girqCount++;
	    DBGWRT_2((DBH, " GIRQ INUSE retry! count=%d\n",
		      girqCount ));

	    OSS_MikroDelay(h->osHdl, 10 );

	    /* check INUSE bit */
	    _MREAD_D32(girqInUse, h->girqVirtAddr, BBCHAM_GIRQ_IN_USE);
#ifdef	_BIG_ENDIAN_
	    girqInUse = OSS_SWAP32( girqInUse );
#endif
	  }

	DBGWRT_1((DBH, "BB - %s%s: GIRQ INUSE bit taken. Retry count=%0d\n",
		  BBNAME, functionName, girqCount ));
      }

      /* Verify and re-write if BBCHAM_GIRQ_IRQ_EN has changed in the meantime
       * This problem occured with async use of vxbmengirq, which can overwrite the BBCHAM_GIRQ_IRQ_EN register
       */
      for(i=0; i<10; i++)
      {
          /* set/reset slot corresponding irq enable bit */
          _MREAD_D32(irqen, h->girqVirtAddr, BBCHAM_GIRQ_IRQ_EN + offs);

#ifdef	_BIG_ENDIAN_
          irqenLittleEndian = OSS_SWAP32( irqen );
#else
          irqenLittleEndian = irqen;
#endif

          if( enable )
          {
	    irqenLittleEndian |= (0x00000001 << (slotShift));
          }
          else
          {
	    irqenLittleEndian &= ~(0x00000001 << (slotShift));
	  }

#ifdef _BIG_ENDIAN_
          irqen = OSS_SWAP32( irqenLittleEndian );
#else
          irqen = irqenLittleEndian;
#endif

          _MWRITE_D32(h->girqVirtAddr, BBCHAM_GIRQ_IRQ_EN + offs, irqen);

          /* wait and verify */
	  OSS_MikroDelay(h->osHdl, 100 );
	   _MREAD_D32(irqen_readback, h->girqVirtAddr, BBCHAM_GIRQ_IRQ_EN + offs);

	  if( irqen_readback == irqen )
	  {
	    break;
	  }
	  else
	  {
            DBGWRT_ERR((DBH, "*** BB - %s%s: BBCHAM_GIRQ_IRQ_EN has been overwritten, retry #%d\n", BBNAME,functionName, i ));
	  }
      }
      if( i >= 10 )
      {
        DBGWRT_ERR((DBH, "*** BB - %s%s: unable to set BBCHAM_GIRQ_IRQ_EN correctly!\n", BBNAME,functionName));
      }

      /* GIRQ INUSE_STS bit available */
      if ( h->girqApiVersion ) {

	/* set current bit for release */
	girqInUse = BBCHAM_GIRQ_IN_USE_BIT;
#ifdef _BIG_ENDIAN_
	girqInUse = OSS_SWAP32( girqInUse );
#endif

	/* release INUSE bit */
	_MWRITE_D32(h->girqVirtAddr, BBCHAM_GIRQ_IN_USE, girqInUse);
	DBGWRT_1((DBH, "BB - %s%s: GIRQ INUSE bit released.\n",
		  BBNAME, functionName ));
      }

      /* release spinlock */
      error = OSS_SpinLockRelease(h->osHdl, h->slHdl);
      if (error)
	{
	  DBGWRT_ERR((DBH, "*** BB - %s%s: OSS_SpinLockRelease() failed!"
    			  "Error 0x%0x\n", BBNAME, functionName, error ));
	  goto CLEANUP;
	}

      DBGWRT_1((DBH, "BB - %s%s: slot=%d enable=%d GIRQ @%08p is %08x slotShift %d\n", BBNAME,functionName,
		slot, enable, h->girqPhysAddr+BBCHAM_GIRQ_IRQ_EN+offs, irqenLittleEndian, slotShift ));
    }

 CLEANUP:
  return( error );
}

/****************************** CHAMELEON_IrqSrvInit **************************
 *
 *  Description:  Called at the beginning of an interrupt.
 *
 *                Do nothing
 *
 *---------------------------------------------------------------------------
 *  Input......:  h			pointer to board handle structure
 *                mSlot     module slot number
 *  Output.....:  return    BBIS_IRQ_UNK
 *  Globals....:  ---
 ****************************************************************************/
static int32 CHAMELEON_IrqSrvInit(
				  BBIS_HANDLE     *h,
				  u_int32         mSlot)
{
  IDBGWRT_1((DBH, "BB - %s_IrqSrvInit: mSlot=%d\n", BBNAME, mSlot ));

  return BBIS_IRQ_UNK;
}

/****************************** CHAMELEON_IrqSrvExit *************************
 *
 *  Description:  Called at the end of an interrupt.
 *
 *                Do nothing
 *
 *---------------------------------------------------------------------------
 *  Input......:  h			pointer to board handle structure
 *                mSlot     module slot number
 *  Output.....:  ---
 *  Globals....:  ---
 ****************************************************************************/
static void CHAMELEON_IrqSrvExit(
				 BBIS_HANDLE     *h,
				 u_int32         mSlot )
{
  IDBGWRT_1((DBH, "BB - %s_IrqSrvExit: mSlot=%d\n", BBNAME, mSlot ));
}

/****************************** CHAMELEON_ExpEnable **************************
 *
 *  Description:  Exception interrupt enable / disable.
 *
 *                Do nothing
 *
 *---------------------------------------------------------------------------
 *  Input......:  h			pointer to board handle structure
 *                mSlot     module slot number
 *                enable    interrupt setting
 *  Output.....:  return    0
 *  Globals....:  ---
 ****************************************************************************/
static int32 CHAMELEON_ExpEnable(
				 BBIS_HANDLE     *h,
				 u_int32         mSlot,
				 u_int32			enable)
{
  IDBGWRT_1((DBH, "BB - %s_ExpEnable: mSlot=%d\n",BBNAME,mSlot));

  return 0;
}

/****************************** CHAMELEON_ExpSrv ***********************************
 *
 *  Description:  Called at the beginning of an exception interrupt.
 *
 *                Do nothing
 *
 *---------------------------------------------------------------------------
 *  Input......:  h			pointer to board handle structure
 *                mSlot     module slot number
 *  Output.....:  return    BBIS_IRQ_NO
 *  Globals....:  ---
 ****************************************************************************/
static int32 CHAMELEON_ExpSrv(
			      BBIS_HANDLE     *h,
			      u_int32         mSlot )
{
  IDBGWRT_1((DBH, "BB - %s_ExpSrv: mSlot=%d\n",BBNAME,mSlot));

  return BBIS_IRQ_NO;
}

/****************************** CHAMELEON_SetMIface **********************
 *
 *  Description:  Set device interface.
 *
 *                Do nothing
 *
 *---------------------------------------------------------------------------
 *  Input......:  h			pointer to board handle structure
 *                mSlot     module slot number
 *                addrMode  MDIS_MODE_A08 | MDIS_MODE_A24
 *                dataMode  MDIS_MODE_PCI6 | MDIS_MODE_D32
 *  Output.....:  return    0
 *  Globals....:  ---
 ****************************************************************************/
static int32 CHAMELEON_SetMIface(
				 BBIS_HANDLE     *h,
				 u_int32         mSlot,
				 u_int32         addrMode,
				 u_int32         dataMode)
{
  DBGWRT_1((DBH, "BB - %s_SetMIface: mSlot=%d\n",BBNAME,mSlot));

  return 0;
}

/****************************** CHAMELEON_ClrMIface **************************
 *
 *  Description:  Clear device interface.
 *
 *                Do nothing
 *
 *---------------------------------------------------------------------------
 *  Input......:  h			pointer to board handle structure
 *                mSlot     module slot number
 *  Output.....:  return    0
 *  Globals....:  ---
 ****************************************************************************/
static int32 CHAMELEON_ClrMIface(
				 BBIS_HANDLE     *h,
				 u_int32         mSlot)
{
  DBGWRT_1((DBH, "BB - %s_ClrMIface: mSlot=%d\n",BBNAME,mSlot));

  return 0;
}

/****************************** CHAMELEON_GetMAddr *********************************
 *
 *  Description:  Get physical address description.
 *
 *                - check device number
 *                - assign address spaces
 *
 *---------------------------------------------------------------------------
 *  Input......:  h			pointer to board handle structure
 *                mSlot     module slot number
 *                addrMode  single device: ignored, group: MDIS_MA_CHAMELEON
 *                dataMode  single device: ignored, group: MDIS_MD_CHAM_n
 *                mAddr     pointer to address space
 *                mSize     size of address space
 *  Output.....:  return    0 | error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 CHAMELEON_GetMAddr(
				BBIS_HANDLE     *h,
				u_int32         mSlot,
				u_int32         addrMode,
				u_int32         dataMode,
				void            **mAddr,
				u_int32         *mSize )
{
  DBGWRT_1((DBH, "BB - %s_GetMAddr: mSlot=0x%04x\n",BBNAME,mSlot));


  /* prevent array index violation */
  if ( mSlot > CHAMELEON_BBIS_MAX_DEVS - 1 )
	  return ERR_BBIS_ILL_SLOT;

  if ( h->devId[mSlot] == CHAMELEON_NO_DEV )
	  return ERR_BBIS_ILL_SLOT;

  /* group device? */
  if( h->devId[mSlot] == CHAMELEON_BBIS_GROUP ) {
    if( ( addrMode != MDIS_MA_CHAMELEON ) && ( addrMode != MDIS_MA_BB_INFO_PTR ) ) {
      DBGWRT_ERR((DBH,"*** %s_GetMAddr: ill addr mode=0x%x for group!\n",
		  BBNAME, addrMode));
      return ERR_BBIS_ILL_ADDRMODE;
    }
    if( dataMode > MDIS_MD_CHAM_MAX ) {
      DBGWRT_ERR((DBH,"*** %s_GetMAddr: ill data mode=0x%x for group!\n",
		  BBNAME, dataMode));
      return ERR_BBIS_ILL_DATAMODE;
    }

    if ( addrMode == MDIS_MA_CHAMELEON ) {
      *mAddr = ((CHAMELEONV2_UNIT*)((BBIS_CHAM_GRP *)h->dev[mSlot])->dev[dataMode])->addr;
      *mSize = ((CHAMELEONV2_UNIT*)((BBIS_CHAM_GRP *)h->dev[mSlot])->dev[dataMode])->size;

      DBGWRT_3((DBH, "BB - %s_GetMAddr: conventional address mode\n",BBNAME));

    } else { /* for address mode MDIS_MA_BB_INFO_PTR, return whole chameleon unit */
      *mAddr = ((BBIS_CHAM_GRP *)h->dev[mSlot])->dev[dataMode];
      *mSize = sizeof(CHAMELEONV2_UNIT);

      DBGWRT_3((DBH, "BB - %s_GetMAddr: cham unit address mode; devId: %d, size: 0x%08x \n",
		BBNAME,((CHAMELEONV2_UNIT*)(*mAddr))->devId,((CHAMELEONV2_UNIT*)(*mAddr))->size));
    }

    /* single device */
  } else {
    if( ((addrMode == MDIS_MA_CHAMELEON) || (addrMode == MDIS_MA_BB_INFO_PTR)) &&
	(dataMode != MDIS_MD_CHAM_0) ){
      DBGWRT_ERR((DBH,"*** %s_GetMAddr: MDIS_MD_CHAM_%d requested for single dev!\n",
		  BBNAME, dataMode));
      return ERR_BBIS_ILL_ADDRMODE;
    }
    if ( addrMode == MDIS_MA_BB_INFO_PTR ) {
      *mAddr = h->dev[mSlot];
      *mSize = sizeof(CHAMELEONV2_UNIT);

    } else {
      *mAddr = ((CHAMELEONV2_UNIT*)h->dev[mSlot])->addr;
      *mSize = ((CHAMELEONV2_UNIT*)h->dev[mSlot])->size;

      DBGWRT_3((DBH, "BB - %s_GetMAddr: conventional address mode\n",BBNAME));
    }
  }

  if( *mSize == 0 ) /* e.g. Cham V0/1 devices */
    *mSize = 0x100;    /* default value fixed */

  DBGWRT_2((DBH, " mSlot=0x%04x : mem address=%08p, length=0x%x\n",
	    mSlot, *mAddr, *mSize));

  return 0;
}

/****************************** CHAMELEON_SetStat ******************************
 *
 *  Description:  Set driver status
 *
 *                Following status codes are supported:
 *
 *                Code                 Description                Values
 *                -------------------  -------------------------  ----------
 *                M_BB_DEBUG_LEVEL     board debug level          see dbg.h
 *
 *---------------------------------------------------------------------------
 *  Input......:  h				pointer to board handle structure
 *                mSlot			module slot number
 *                code			setstat code
 *                value32_or_64	setstat value or ptr to blocksetstat data
 *  Output.....:  return    0 | error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 CHAMELEON_SetStat(
			       BBIS_HANDLE     *h,
			       u_int32         mSlot,
			       int32           code,
			       INT32_OR_64     value32_or_64 )
{
  int32 value = (int32)value32_or_64; /* 32bit value */

  DBGWRT_1((DBH, "BB - %s_SetStat: mSlot=%d code=0x%04x value=0x%x\n",
	    BBNAME, mSlot, code, value));

  switch (code) {
    /* set debug level */
  case M_BB_DEBUG_LEVEL:
    h->debugLevel = value;
    break;

    /* unknown */
  default:
    return ERR_BBIS_UNK_CODE;
  }

  return 0;
}

/****************************** CHAMELEON_GetStat *****************************
 *
 *  Description:  Get driver status
 *
 *                Following status codes are supported:
 *
 *                Code                 Description                Values
 *                -------------------  -------------------------  ----------
 *                M_BB_DEBUG_LEVEL     driver debug level         see dbg.h
 *                M_MK_BLK_REV_ID      ident function table ptr   -
 *
 *---------------------------------------------------------------------------
 *  Input......:  h					pointer to board handle structure
 *                mSlot				module slot number
 *                code				getstat code
 *  Output.....:  value32_or_64P    getstat value or ptr to blockgetstat data
 *                return    0 | error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 CHAMELEON_GetStat(
			       BBIS_HANDLE     *h,
			       u_int32         mSlot,
			       int32           code,
			       INT32_OR_64     *value32_or_64P )
{
  int32 *valueP = (int32*)value32_or_64P; /* pointer to 32bit value */

  DBGWRT_1((DBH, "BB - %s_GetStat: mSlot=%d code=0x%04x\n",BBNAME,mSlot,code));

  switch (code) {
    /* get debug level */
  case M_BB_DEBUG_LEVEL:
    *valueP = h->debugLevel;
    break;

    /* ident table */
  case M_MK_BLK_REV_ID:
    *value32_or_64P = (INT32_OR_64)&h->idFuncTbl;
    break;

    /* unknown */
  default:
    return ERR_BBIS_UNK_CODE;
  }

  return 0;
}

/****************************** CHAMELEON_Unused ****************************
 *
 *  Description:  Dummy function for unused jump table entries.
 *
 *---------------------------------------------------------------------------
 *  Input......:  ---
 *  Output.....:  return  ERR_BBIS_ILL_FUNC
 *  Globals....:  ---
 ****************************************************************************/
static int32 CHAMELEON_Unused( void )		/* nodoc */
{
  return ERR_BBIS_ILL_FUNC;
}

/*********************************** Ident **********************************
 *
 *  Description:  Return ident string
 *
 *---------------------------------------------------------------------------
 *  Input......:  -
 *  Output.....:  return  pointer to ident string
 *  Globals....:  -
 ****************************************************************************/
static char* Ident( void )		/* nodoc */
{
  return( (char*) IdentString );
}

/********************************* Cleanup **********************************
 *
 *  Description:  Close all handles, free memory and return error code
 *
 *		          NOTE: The h handle is invalid after calling this
 *                      function.
 *
 *---------------------------------------------------------------------------
 *  Input......:  h			pointer to board handle structure
 *                retCode	return value
 *  Output.....:  return	retCode
 *  Globals....:  -
 ****************************************************************************/
static int32 Cleanup(
		     BBIS_HANDLE  *h,
		     int32        retCode		/* nodoc */
		     )
{
  u_int32 i;
  u_int32 error =0;
  DBGWRT_1((DBH, "BB - %s_Cleanup\n",BBNAME));

  /*------------------------------+
    |  close handles                |
    +------------------------------*/
  /* clean up desc */
  if (h->descHdl)
    DESC_Exit(&h->descHdl);

  /* remove spinlock */
  if (h->slHdl) {
    error = OSS_SpinLockRemove(h->osHdl, &h->slHdl);
    if ( error ) {
      DBGWRT_ERR((DBH, "*** BB - %s_Cleanup: OSS_SpinLockRemove() failed! "
		  "Error 0x%0x!\n", BBNAME, error ));
    }
  }

  /* cleanup debug */
  DBGEXIT((&DBH));

  /*------------------------------+
    |  free memory                  |
    +------------------------------*/
  /* release memory for devices and groups */
  for( i = 0; i < CHAMELEON_BBIS_MAX_DEVS; i++ ) {
    if( h->dev[i] ){
      OSS_MemFree(h->osHdl, h->dev[i], h->devGotSize[i]);
      h->dev[i] = NULL;
    }
  }

  /* release memory for the board handle */
  OSS_MemFree( h->osHdl, (int8*)h, h->ownMemSize);
  h = NULL;

  /*------------------------------+
    |  return error code            |
    +------------------------------*/
  return(retCode);
}

/********************************* CfgInfoSlot ******************************
 *
 *  Description:  Fulfils the BB_CfgInfo(BBIS_CFGINFO_SLOT) request
 *
 *				  The variable-argument list (argptr) contains the following
 *                parameters in the given order:
 *
 *                Input
 *                -----
 *                mSlot (u_int32) - device slot number
 *
 *                Output
 *                ------
 *                occupied (u_int32*) - occupied information
 *                 - pluggable device:
 *                   BBIS_SLOT_OCCUP_YES if slot is occupied
 *                   or BBIS_SLOT_OCCUP_NO if slot is empty
 *                 - onboard device:
 *                   BBIS_SLOT_OCCUP_ALW if device is enabled
 *                   or BBIS_SLOT_OCCUP_DIS if device is disabled
 *
 *                devId (u_int32*) - device id (4-byte hex value)
 *                  The device id should identify the type of the device
 *                  but should not contain enough information to differentiate
 *                  between two identical devices. If the device id is unknown
 *                  BBIS_SLOT_NBR_UNK must be returned.
 *                  - M-Module:
 *                    id-prom-magic-word << 16 | id-prom-module-id
 *                    Example: 0x53460024
 *                    Note: Must match to the <mmoduleid> tag of the driver's xml file.
 *                  - Chameleon Device:
 *                    device id of unit
 *                    Example: 44 (=0x0000002C) for 16Z044_DISP
 *                    Note: Must match to the <chamv2id> tag of the driver's xml file.
 *
 *                devRev (u_int32*) - device revision (4-byte hex value)
 *                  - M-Module: id-prom-layout-revision << 16 |
 *                              id-prom-product-variant
 *                              example: 0x01091400
 *                  - Chameleon Device:
 *                    chameleon module revision
 *                    Example: 0x00000001 (CAN boromir)
 *                  or BBIS_SLOT_NBR_UNK if device revision is unknown
 *
 *                slotName (char*) - slot name
 *                  The slot name should consist of the slot type and the
 *                  slot label and may contain further information about
 *                  the device instance. The slot name must not contain any
 *                  non-printing characters.
 *                  The length of the returned string, including the
 *                  terminating null character, must not exceed
 *                  BBIS_SLOT_STR_MAXSIZE.
 *                  format : "<slot type> <slot label>"
 *
 *                  Examples:
 *                  - M-Module:			"M-Module slot 0"
 *                  - Chameleon Device:	"cham-slot 3 (is instance 1)"
 *
 *                devName (char*) - device name
 *                  The device name should identify the type of the device
 *                  but should not contain enough information to differentiate
 *                  between two identical devices. Furthermore, the device
 *                  name should refer to the appropriate device driver name if
 *                  possible.
 *
 *                  The returned string must not contain any non-printing
 *                  characters or blanks. The length of the returned string,
 *                  including the terminating null character, must not exceed
 *                  BBIS_SLOT_STR_MAXSIZE.
 *
 *                  Examples:
 *                  - M-Module:		"M34", "MS9"
 *
 *                  If the device name is unknown BBIS_SLOT_STR_UNK must
 *                  be returned.
 *
 *                  Note: The returned device name must be identical to the
 *                        "hwname" value in the device drivers xml file.
 *
 *---------------------------------------------------------------------------
 *  Input......:  h			pointer to board handle structure
 *				  argptr	argument pointer
 *  Output.....:  return	error code
 *  Globals....:  -
 ****************************************************************************/
static int32 CfgInfoSlot( BBIS_HANDLE *h, va_list argptr )	/* nodoc */
{
  u_int32 mSlot     = va_arg( argptr, u_int32 );
  u_int32 *occupied = va_arg( argptr, u_int32* );
  u_int32 *devId    = va_arg( argptr, u_int32* );
  u_int32 *devRev   = va_arg( argptr, u_int32* );
  char	*slotName = va_arg( argptr, char* );
  char	*devName  = va_arg( argptr, char* );
  CHAMELEONV2_UNIT	*unitP;

  /* clear parameters to return (for error case) */
  *occupied = 0;
  *devId    = 0;
  *devRev   = 0;
  *slotName = '\0';

  /* slot out of range ? */
  if ( mSlot > CHAMELEON_BBIS_MAX_DEVS - 1 ) {
	  DBGWRT_ERR((DBH,"*** CfgInfoSlot: mSlot out of range! (mslot = 0x%08x)\n", mSlot));
	  return ERR_BBIS_ILL_PARAM; /*safe, no resources allocated till here */
  }

  /* illegal slot? */
  if( h->devId[mSlot] == CHAMELEON_NO_DEV ) {
    /*
     * no debug print here because it will be called under Windows
     * with mSlot=0x00..0xff and 0x1000..0x10ff
     */
    return ERR_BBIS_ILL_SLOT;
  }

  /* set occupied info */
  *occupied = BBIS_SLOT_OCCUP_ALW;

  if( h->devId[mSlot] == CHAMELEON_BBIS_GROUP )
    /* use first module of group */
    unitP = (CHAMELEONV2_UNIT*)(((BBIS_CHAM_GRP*)h->dev[mSlot])->dev[0]);
  else
    unitP = (CHAMELEONV2_UNIT*)h->dev[mSlot];

  *devId    = (u_int32)unitP->devId;
  *devRev   = (u_int32)unitP->revision;

  /* build slot name */
  if( unitP->group != 0 ) {
    OSS_Sprintf( h->osHdl, slotName, "cham-slot %d (is instance %d, group %d)",
		 mSlot, unitP->instance, unitP->group);
  } else {
    OSS_Sprintf( h->osHdl, slotName, "cham-slot %d (is instance %d)",
		 mSlot, unitP->instance);
  }

  /* set default for unknown chameleon device */
  /* mem */
  if( h->chamInfo.ba[unitP->bar].type == OSS_ADDRSPACE_MEM )
    *devName  = '\0'; /* indicates BBIS_SLOT_STR_UNK */
  /* io */
  else
    OSS_StrCpy( h->osHdl, "_IO", devName ); /* _IO */

  /* copy device name ( of first unit in case of group ) */
  /* known chameleon device? If: not leave '\0'. */
  if( unitP->devId != 0xffff ){
    char *retDevName = (char*) CHAM_DevIdToName((u_int16)*devId);
    /* name for devId gotten? */
    if( OSS_StrCmp( h->osHdl, retDevName, "?" ) ){
      /* mem */
      if( h->chamInfo.ba[unitP->bar].type == OSS_ADDRSPACE_MEM )
	OSS_StrCpy( h->osHdl, retDevName, devName ); /* <name> */
      /* io */
      else
	OSS_Sprintf( h->osHdl, devName, "IO_%s", retDevName ); /* IO_<name> */
    }
  }

  DBGWRT_2((DBH," devId=0x%08x, devRev=0x%08x, devName=\"%s\"\n",
	    *devId, *devRev, devName ));

  /* return on success */
  return ERR_SUCCESS;
}

#ifndef CHAM_ISA
/********************************* ParsePciPath *****************************
 *
 *  Description: Parses the specified PCI_BUS_PATH to find out PCI Bus Number
 *
 *---------------------------------------------------------------------------
 *  Input......: h   			handle
 *  Output.....: returns:	   	error code
 *				 *pciBusNbrP	main PCI bus number of D203
 *  Globals....: -
 ****************************************************************************/
static int32 ParsePciPath( BBIS_HANDLE *h, u_int32 *pciBusNbrP ) 	/* nodoc */
{
  u_int32 i;
  int32 pciBusNbr=0, pciDevNbr;
  int32 error;
  int32 vendorID, deviceID, headerType, secondBus;

  /* parse whole pci path until the chameleon device is reached */
  for(i=0; i < h->pciPathLen; i++) {

    pciDevNbr = h->pciPath[i];

#ifdef VXWORKS
    if ( ( i==0 )
#	ifdef VXW_PCI_DOMAIN_SUPPORT
	 && ( 0 != h->pciDomainNbr )
#	endif
	 ) {
#else
    if ( ( i==0 ) && ( 0 != h->pciDomainNbr )) {
#endif
      /* as we do not know the numbering order of busses on pci domains,
	 try to find the device on all busses instead of looking for the
	 first bus on the domain  */
      for (pciBusNbr=0; pciBusNbr<0xff; pciBusNbr++) {

        error = PciParseDev( h,
			     OSS_MERGE_BUS_DOMAIN(pciBusNbr, h->pciDomainNbr),
			     h->pciPath[0], &vendorID, &deviceID, &headerType,
			     &secondBus );
#ifdef VXWORKS
        if ( error == ERR_SUCCESS && vendorID != 0xffff && deviceID != 0xffff )
#else
        if ( error == ERR_SUCCESS )
#endif
	      break; /* found device */
      }

      if ( error != ERR_SUCCESS ) { /* device not found */
        DBGWRT_ERR((DBH,"*** BB - %s: first device 0x%02x in pci bus path "
		    "not found on domain %d!\n",
		    BBNAME, h->pciPath[0], h->pciDomainNbr ));
        return error;
      }
    } else {
      /* parse device only once */
      if( (error = PciParseDev( h,
                                OSS_MERGE_BUS_DOMAIN(pciBusNbr, h->pciDomainNbr ),
				pciDevNbr,
                                &vendorID, &deviceID, &headerType, &secondBus )))
	return error;
    }

    if( vendorID == 0xffff && deviceID == 0xffff ){
      DBGWRT_ERR((DBH,"*** BB - %s:ParsePciPath: Nonexistant device "
		  "domain %d bus %d dev %d\n", BBNAME, h->pciDomainNbr, pciBusNbr, pciDevNbr ));
      return ERR_BBIS_NO_CHECKLOC;
    }

#if ( !defined(VXWORKS) || defined(VXW_PCI_DOMAIN_SUPPORT) )
    /*--- device is present, is it a bridge ? ---*/
    if( (headerType & ~OSS_PCI_HEADERTYPE_MULTIFUNCTION) != OSS_PCI_HEADERTYPE_BRIDGE_TYPE ){
      DBGWRT_ERR((DBH,"*** BB - %s:ParsePciPath: Device is not a bridge!"
		  "domain %d bus %d dev %d vend=0x%x devId=0x%x\n",
		  BBNAME, h->pciDomainNbr, pciBusNbr, pciDevNbr, vendorID,
		  deviceID ));

      return ERR_BBIS_NO_CHECKLOC;
    }

    /*--- it is a bridge, determine its secondary bus number ---*/
    DBGWRT_2((DBH, " domain %d bus %d dev 0x%x: vend=0x%x devId=0x%x second bus %d\n",
    	      h->pciDomainNbr, pciBusNbr, pciDevNbr, vendorID, deviceID, secondBus ));

    /* --- continue with new bus --- */
    pciBusNbr = secondBus;
#endif
  }

  DBGWRT_1((DBH,"BB - %s: Main PCI Bus Number is %d\n", BBNAME,
	    pciBusNbr ));

  *pciBusNbrP = pciBusNbr;

  return ERR_SUCCESS;
}

/********************************* PciParseDev ******************************
 *
 *  Description: Get parameters from specified PCI device's config space
 *
 *---------------------------------------------------------------------------
 *  Input......: h          handle
 *				 pciBusNbr  pci bus number (merged with domain)
 *				 pciDevNbr  pci dev number
 *  Output.....: returns: 	error code (only fails if config access error)
 *				 *vendorIDP vendor id
 *				 *deviceIDP device id
 *				 *headerTypeP header type
 *				 *secondBusP secondary bus number (only valid for bridge)
 *  Globals....: -
 ****************************************************************************/
static int32 PciParseDev(
			 BBIS_HANDLE *h,
			 u_int32 pciBusNbr,
			 u_int32 pciDevNbr,
			 int32 *vendorIDP,
			 int32 *deviceIDP,
			 int32 *headerTypeP,
			 int32 *secondBusP)	/* nodoc */
{
  int32 error;

  u_int32 pciMainDevNbr;
  u_int32 pciDevFunc;

  pciMainDevNbr = pciDevNbr;
  pciDevFunc = 0;

  if (pciDevNbr > 0x1f)
    {
      /* separate the function number from the device number */
      pciDevFunc = pciDevNbr >> 5;
      pciMainDevNbr = (pciDevNbr & 0x0000001f);
    }

  /*--- check to see if device present ---*/
  error = OSS_PciGetConfig( h->osHdl,
			    pciBusNbr, pciMainDevNbr, pciDevFunc,
			    OSS_PCI_VENDOR_ID, vendorIDP );

  if( error == 0 )
    error = OSS_PciGetConfig( h->osHdl,
			      pciBusNbr, pciMainDevNbr, pciDevFunc,
			      OSS_PCI_DEVICE_ID, deviceIDP );

  if( error )
    return PciCfgErr(h,"PciParseDev", error,
		     pciBusNbr,pciDevNbr,OSS_PCI_DEVICE_ID);

  if( *vendorIDP == 0xffff && *deviceIDP == 0xffff )
    return ERR_SUCCESS;		/* not present */

  /*--- device is present, is it a bridge ? ---*/
  error = OSS_PciGetConfig( h->osHdl,
			    pciBusNbr, pciMainDevNbr, pciDevFunc,
			    OSS_PCI_HEADER_TYPE, headerTypeP );

  if( error )
    return PciCfgErr(h,"PciParseDev", error,
		     pciBusNbr,pciDevNbr,OSS_PCI_HEADER_TYPE);

  DBGWRT_2((DBH, " domain %d bus %d dev %d.%d: vend=0x%x devId=0x%x hdrtype %d\n",
	    OSS_DOMAIN_NBR( pciBusNbr ), OSS_BUS_NBR( pciBusNbr ), pciMainDevNbr, pciDevFunc,
	    *vendorIDP, *deviceIDP, *headerTypeP ));

  if( ( *headerTypeP & ~OSS_PCI_HEADERTYPE_MULTIFUNCTION) != OSS_PCI_HEADERTYPE_BRIDGE_TYPE )
    return ERR_SUCCESS;		/* not bridge device */


  /*--- it is a bridge, determine its secondary bus number ---*/
  error = OSS_PciGetConfig( h->osHdl,
			    pciBusNbr, pciMainDevNbr, pciDevFunc,
			    PCI_SECONDARY_BUS_NUMBER | OSS_PCI_ACCESS_8,
			    secondBusP );

  if( error )
    return PciCfgErr(h,"PciParseDev", error,
		     pciBusNbr,pciDevNbr,
		     PCI_SECONDARY_BUS_NUMBER | OSS_PCI_ACCESS_8);

  return ERR_SUCCESS;
}

/********************************* PciCfgErr ********************************
 *
 *  Description: Print Debug message
 *
 *---------------------------------------------------------------------------
 *  Input......: h              handle
 *               funcName		function name
 *               error			error code
 *				 pciBusNbr		pci bus number
 *				 pciDevNbr		pci device number
 *               reg			register
 *  Output.....: return			error code
 *  Globals....: -
 ****************************************************************************/
static int32 PciCfgErr(
		       BBIS_HANDLE *h,
		       char *funcName,
		       int32 error,
		       u_int32 pciBusNbr,
		       u_int32 pciDevNbr,
		       u_int32 reg )		/* nodoc */
{
  u_int32 pciMainDevNbr = pciDevNbr;
  u_int32 pciDevFunc = 0;

  if (pciDevNbr > 0x1f)
    {
      /* device number contains function in upper 3 bit */
      pciDevFunc = pciDevNbr >> 5;  /* devNbr e.g. 0b 0101 1110 */
      pciMainDevNbr = pciDevNbr & 0x0000001f;
    }

  DBGWRT_ERR((DBH,"*** BB - %s %s: PCI access error 0x%x "
	      "domain %d bus %d dev %d.%d reg 0x%x\n", BBNAME, funcName, error,
	      OSS_DOMAIN_NBR( pciBusNbr ), OSS_BUS_NBR( pciBusNbr ), pciMainDevNbr, pciDevFunc, reg ));
  return error;
}
#endif /* CHAM_ISA */







