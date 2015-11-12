/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: bb_chameleon.c
 *      Project: CHAMELEON board handler
 *
 *       Author: kp
 *        $Date: 2007/03/01 16:07:11 $
 *    $Revision: 1.2 $
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
 *	   GROUP_4/GROUP_ID      = U_INT32  0x0001 	# group Nr. as in cham table
 *	   GROUP_4/DEVICE_IDV2_0 = U_INT32  0x3500 	# IDE,     group 1, instance 0
 *	   GROUP_4/DEVICE_IDV2_1 = U_INT32  0x4400 	# IDETGT , group 1, instance 0
 *	   GROUP_4/DEVICE_IDV2_2 = U_INT32  0x4600 	# IDEDISK, group 1, instance 0
 *	   GROUP_5/GROUP_ID      = U_INT32  0x0002 	# group Nr. as in cham table
 *	   GROUP_5/DEVICE_IDV2_0 = U_INT32  0x2c00 	# DISP, group 2, instance 0
 *	   GROUP_5/DEVICE_IDV2_1 = U_INT32  0x2b00 	# SDRAM, group 2, instance 0
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
 *	  AUTOENUM_EXCLUDINGV2 = BINARY 0x13,0x19,0x34,0x2C # excluded device IDs
 *    or (if AUTOENUM_EXCLUDINGV2 is not found)
 *    AUTOENUM_EXCLUDING   = BINARY 0x0a,0x07,0x20,0x25 # excluded module codes
 *
 *  FPGA modules
 *  nbr  | devId | group | name         | BBIS slot number
 *  -----+-------+-------+--------------+-----------------
 *  0    |  0x13 |   0   | Z035_SYSTEM  | (excluded)
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
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: sc15_test_bb_chameleon.c,v $
 * Revision 1.2  2007/03/01 16:07:11  SYao
 * Remove the need to define IO1 und IO2 macro
 *
 * Revision 1.1  2007/02/13 17:54:40  SYao
 * Initial Revision
 *
 * Revision 1.16  2006/12/11 17:01:22  ufranke
 * added
 *  + fkt13 is now CHAMELEON_SetIrqHandle()
 *  + GIRQ unit support for BB irqEnable()
 *
 * Revision 1.15  2006/08/16 20:25:36  ts
 * Bugfix: retrieve Unit IRQ correctly for Nongroup-devices
 *
 * Revision 1.14  2006/08/02 15:54:42  DPfeuffer
 * supports unknown chameleon devices for autoenum
 *
 * Revision 1.13  2006/08/01 12:04:00  DPfeuffer
 * - CHAMELEON_Init(): get AUTOENUM_EXCLUDINGV2 desc-key fixed
 * - CHAMELEON_BrdInit(): fixed: InitPci() was called without OSS handle
 * - CfgInfoSlot(): slot name includes now the chameleon device instance
 * - some casts for Windows compiler added
 *
 * Revision 1.12  2006/07/20 15:30:11  ufranke
 * cosmetics
 *
 * Revision 1.11  2006/05/31 14:54:19  cs
 * fixed: memory leak (OSS_MemFree was called with size = 0 for every device)
 *
 * Revision 1.10  2006/05/30 10:56:44  cs
 * bugfix for excluding members of groups (limits where not checked properly)
 *
 * Revision 1.9  2006/03/07 12:48:29  cs
 * changed:
 *     - use Chameleon V2 lib
 *     - support up to 32 devices and 16 groups (32 devices each)
 *     - GetMAddr now returns:
 *         - actual size of addr space for CHAM V2 chameleon tables
 *         - addr space size 0x100 for CHAM V0/V1 chameleon tables
 *         - for groups: index of MACCESS possible
 *
 * Revision 1.8  2005/08/03 11:22:40  dpfeuffer
 * cast added
 *
 * Revision 1.7  2005/07/27 12:54:37  dpfeuffer
 * CfgInfoSlot(): ChameleonHwName() replaced with ChameleonModName()
 *
 * Revision 1.6  2005/05/11 14:03:39  ufranke
 * temporarily workaround for removed ChameleonHwName()
 *
 * Revision 1.5  2005/02/16 15:27:54  ub
 * added ability to use PCI_BUS_PATH/PCI_BUS_SLOT from descriptor
 *
 * Revision 1.4  2004/11/22 09:52:16  ub
 * Added: IO mapped access
 *
 * Revision 1.3  2004/06/08 16:02:37  ub
 * added capability to use irq level from PCI config space
 *
 * Revision 1.2  2004/05/24 10:25:23  dpfeuffer
 * 1) automatic enumeration implemented
 * 2) BrdInfo(BBIS_BRDINFO_BRDNAME) and CfgInfo(BBIS_CFGINFO_SLOT) added
 * 3) CfgInfo(BBIS_CFGINFO_IRQ): mode no longer BBIS_IRQ_EXCEPTION
 *
 * Revision 1.1  2003/02/03 10:44:36  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2003..2006 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

static const char RCSid[]="$Id: sc15_test_bb_chameleon.c,v 1.2 2007/03/01 16:07:11 SYao Exp $";

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
#include <MEN/maccess.h>   	

/*-----------------------------------------+
|  DEFINES                                 |
+-----------------------------------------*/
#define CHAMELEON_USE_PCITABLE
/* debug settings */
#define DBG_MYLEVEL		h->debugLevel
#define DBH             h->debugHdl
#define CHAMELEON_BBIS_DEBUG


#define BBNAME				"CHAMELEON"
#define CHAMELEON_BBIS_MAX_DEVS	32			/* max number of devices supported */
#define CHAMELEON_BBIS_MAX_GRPS	15			/* max number of groups supported */
#define CHAMELEON_NO_DEV		0xfffd		/* flags devId[x] invalid */
#define CHAMELEON_BBIS_GROUP	0xfffe		/* flags devId[x] is a group */
#define MAX_EXCL_MODCODES		0xff		/* number of max excluded module codes */
#define MAX_PCI_PATH			16		    /* max number of bridges to devices */
#define PCI_SECONDARY_BUS_NUMBER	0x19	/* PCI bridge config */

#define BBCHAM_GIRQ_SPACE_SIZE		0x20		/* 32 byte register + reserved */
#define BBCHAM_GIRQ_IRQ_REQ			0x00		/* interrupt request register */
#define BBCHAM_GIRQ_IRQ_EN			0x08		/* interrupt enable register  */

#define EU_IO1	(0x1e)
/*
#define EU_CTRL_IO1
#ifdef	EU_CTRL_IO1
	#define CAN_NUM	(3)	
	#define CAN_BAR_ADDR (0x90000000UL)
#endif
#ifdef EU_CTRL_IO2
	#define CAN_NUM	(4)	
	#define CAN_BAR_ADDR (0x90010000UL)
#endif
#ifdef EU_CTRL_IO1
	#ifdef EU_CTRL_IO2
		#error "define EU_CTRL_IO1 or EU_CTRL_IO2, but not both"
	#endif
#endif

#ifndef EU_CTRL_IO1
	#ifndef EU_CTRL_IO2
		#error "define EU_CTRL_IO1 or EU_CTRL_IO2"
	#endif
#endif
*/



/*-----------------------------------------+
|  TYPEDEFS                                |
+-----------------------------------------*/
typedef struct {
	u_int32 grpId;									/* group ID from Table */
	u_int16	devId[CHAMELEON_BBIS_MAX_DEVS];         /* from DEVICE_IDV2_n */
	int16   inst[CHAMELEON_BBIS_MAX_DEVS];			/* instance (V2) else -1 */
	u_int32 idx[CHAMELEON_BBIS_MAX_DEVS];			/* index (V0/1) */
    CHAMELEONV2_UNIT unit[CHAMELEON_BBIS_MAX_DEVS]; /* info of each module */
	int32 	devCount;							  /* num of devices in group */
}BBIS_CHAM_GRP;
typedef struct {
	MDIS_IDENT_FUNCT_TBL idFuncTbl;	/* id function table		*/
	CHAM_FUNCTBL       chamFuncTbl;	/* chameleon V2 function table */
    u_int32     ownMemSize;			/* own memory size			*/
    OSS_HANDLE* osHdl;				/* os specific handle		*/
    DESC_HANDLE *descHdl;			/* descriptor handle pointer*/
    u_int32     debugLevel;			/* debug level for BBIS     */
	DBG_HANDLE  *debugHdl;			/* debug handle				*/

	u_int32		pciBusNbr;			/* PCI bus number of FPGA	*/
	u_int32		pciDevNbr;			/* PCI device number of FPGA	*/
	u_int8		pciPath[MAX_PCI_PATH]; /* PCI path from desc		*/
	u_int32		pciPathLen;			/* number of bytes in pciPath	*/

	u_int16		devId[CHAMELEON_BBIS_MAX_DEVS]; /* copy of DEVICE_IDV2_n */
	int16   	inst[CHAMELEON_BBIS_MAX_DEVS];	/* instance (V2) else -1 */
	u_int32 	idx[CHAMELEON_BBIS_MAX_DEVS];	/* index of cham device */
	void*		dev[CHAMELEON_BBIS_MAX_DEVS];	/* info of module */
	u_int32 	devGotSize[CHAMELEON_BBIS_MAX_DEVS];/* mem allocated for me */
	int32		devCount;						/* num of slots occupied */

	char 			*girqPhysAddr;	/* GIRQ unit physical address */
	char 			*girqVirtAddr;	/* GIRQ unit virtual address */
	OSS_IRQ_HANDLE	*irqHdl;		/* irq handle */

	u_int32		autoEnum;			/* <>0: auomatic enumeration */
    u_int8      exclModCodes[MAX_EXCL_MODCODES]; /* excluded module codes */
	u_int32		exclModCodesNbr;		/* number of excluded module codes */
} BBIS_HANDLE;

/* include files which need BBIS_HANDLE */
#include <MEN/bb_entry.h>	/* bbis jumptable		*/
#include <MEN/bb_chameleon.h>		/* PCI bbis header file	*/


/*-----------------------------------------+
|  GLOBALS                                 |
+-----------------------------------------*/

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
static int32 CHAMELEON_SetStat(BBIS_HANDLE*, u_int32, int32, int32);
static int32 CHAMELEON_GetStat(BBIS_HANDLE*, u_int32, int32, int32*);
/* unused */
static int32 CHAMELEON_Unused(void);
/* miscellaneous */
static char* Ident( void );
static int32 Cleanup(BBIS_HANDLE *h, int32 retCode);
static int32 CfgInfoSlot( BBIS_HANDLE *h, va_list argptr );

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

static int32 CHAMELEON_SetIrqHandle( BBIS_HANDLE *h, OSS_IRQ_HANDLE *irqHdl );

/**************************** CHAMELEON_GetEntry ***********************************
 *
 *  Description:  Initialize drivers jump table.
 *
 *---------------------------------------------------------------------------
 *  Input......:  bbisP     pointer to the not initialized structure
 *  Output.....:  *bbisP    initialized structure
 *  Globals....:  ---
 ****************************************************************************/
#ifdef _ONE_NAMESPACE_PER_DRIVER_
	extern void BBIS_GetEntry( BBIS_ENTRY *bbisP )
#else
	extern void CHAMELEON_GetEntry( BBIS_ENTRY *bbisP )
#endif
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
    bbisP->setIrqHandle =   CHAMELEON_SetIrqHandle;
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
 *				  PCI_BUS_NUMBER
 *				  PCI_DEVICE_NUMBER
 *                DEVICE_ID_n  (n=0..15)   -                0...31
 *                GROUP_n/DEVICE_IDV2_n  (n=0..15)   -                0...31
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
	u_int32     mechSlot, gotsize, i, g;
    int32       status;
    u_int32		value;
	BBIS_CHAM_GRP *devGrp = NULL;

    /*-------------------------------+
    | initialize the board structure |
    +-------------------------------*/
    /* get memory for the board structure */
    *hP = h = (BBIS_HANDLE*) (OSS_MemGet( osHdl, sizeof(BBIS_HANDLE), &gotsize ));
    if ( h == NULL )
        return ERR_OSS_MEM_ALLOC;

	/* cleanup the turkey */
	OSS_MemFill( h->osHdl, sizeof(BBIS_HANDLE), (char*)h, 0x00 );

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


    /*---- get PCI bus/device number ----*/

    /* PCI_BUS_NUMBER - required if PCI_BUS_PATH not given  */
    status = DESC_GetUInt32( h->descHdl, 0, &h->pciBusNbr,
							 "PCI_BUS_NUMBER");

	if( status == ERR_DESC_KEY_NOTFOUND ){

		/* PCI_BUS_PATH - required if PCI_DEVICE_NUMBER not given */
		h->pciPathLen = MAX_PCI_PATH;

		status = DESC_GetBinary( h->descHdl, (u_int8*)"", 0,
								 h->pciPath, &h->pciPathLen,
								 "PCI_BUS_PATH");

		if( status ){
			DBGWRT_ERR((DBH, "*** BB - %s_Init: Found neither Desc Key "
                        "PCI_BUS_PATH nor PCI_BUS_NUMBER !\n",	BBNAME));
			return( Cleanup(h,status) );
		}

		/*--------------------------------------------------------+
		|  parse the PCI_PATH to determine bus number of devices  |
		+--------------------------------------------------------*/
#ifdef DBG
		DBGWRT_2((DBH, " PCI_PATH="));
		for(i=0; i<h->pciPathLen; i++){
			DBGWRT_2((DBH, "0x%x", h->pciPath[i]));
		}
		DBGWRT_2((DBH, "\n"));
#endif
		if( (status = ParsePciPath( h, &h->pciBusNbr )) )
			return( Cleanup(h,status));

	} /* if( status == ERR_DESC_KEY_NOTFOUND ) */
	else {
		if( status == ERR_SUCCESS) {
			DBGWRT_1((DBH,"BB - %s: Using main PCI Bus Number from desc %d\n",
					  BBNAME, h->pciBusNbr ));
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

	if(status==ERR_DESC_KEY_NOTFOUND){

		/* PCI_BUS_SLOT - required if PCI_DEVICE_NUMBER not given */
    	status = DESC_GetUInt32( h->descHdl, 0, &mechSlot, "PCI_BUS_SLOT");

		if( status==ERR_DESC_KEY_NOTFOUND ){
			DBGWRT_ERR((DBH, "*** BB - %s_Init: Found neither Desc Key "
                        "PCI_BUS_SLOT nor PCI_DEVICE_NUMBER !\n", BBNAME));
		}

		if( status )
        	return( Cleanup(h,status) );

	    /* convert PCI slot into PCI device id */
    	if( (status = OSS_PciSlotToPciDevice( osHdl, h->pciBusNbr, mechSlot,
											   (int32*)&h->pciDevNbr)) )
			return( Cleanup(h,status) );
    }


	/* get AUTOENUM (optional) */
    status = DESC_GetUInt32( h->descHdl, 0, &h->autoEnum, "AUTOENUM");
    if( status && (status!=ERR_DESC_KEY_NOTFOUND) )
		return( Cleanup(h,status) );
	
	h->devCount = 0;
	/* no device found yet */
	for( i=0; i<CHAMELEON_BBIS_MAX_DEVS; i++ ) {
		h->devId[i]      = CHAMELEON_NO_DEV;
		h->dev[i]        = NULL;
		h->devGotSize[i] = 0;
	}

	/* manual enumeration? */

	/* get DEVICE_ID(V2)_n, group 0*/
	for( i=0; i < CHAMELEON_BBIS_MAX_DEVS; i++ ){

		if( (status = DESC_GetUInt32( h->descHdl, 0, &value,
									  "DEVICE_IDV2_%d", i)) == ERR_SUCCESS )
		{
			h->devId[i] = (u_int16)(value & 0xffffff00) >> 8;
			h->inst[i]  = (int16)value & 0xff;
			h->idx[i]   = 0;
		} else if( (status = DESC_GetUInt32( h->descHdl, 0, &value,
									  "DEVICE_ID_%d", i)) == ERR_SUCCESS )
		{
			u_int16 modId	= (u_int16)(value & 0xffffff00) >> 8;
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
	for( g=1; g <= CHAMELEON_BBIS_MAX_GRPS &&
			  h->devCount < CHAMELEON_BBIS_MAX_DEVS; g++ )
	{
		status = DESC_GetUInt32( h->descHdl, 0, &value,
								 "GROUP_%d/GROUP_ID", g);

		if( status == ERR_SUCCESS )
		{
			/* group exists? get memory for group */
			u_int32 gotSize = 0;
			devGrp = (BBIS_CHAM_GRP *)OSS_MemGet( h->osHdl,
								 sizeof( devGrp ),
								 &gotSize);
			if( !devGrp ) {
				DBGWRT_ERR((DBH, "*** %s_Init: no ressources\n",
							BBNAME));
				return(  Cleanup(h,ERR_BBIS_DESC_PARAM) );
			}
			h->dev[g]    = devGrp;
			h->devGotSize[g] = gotSize;

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
				devGrp->devId[i] = (u_int16)(value & 0xffffff00) >> 8;
				devGrp->inst[i]  = (int16)value & 0xff;
				devGrp->idx[i]   = 0;
				devGrp->devCount++;
				DBGWRT_2(( DBH, " GROUP_%d/DEVICE_IDV2_%d = 0x%x\n",
						   g, i, value ));
			}
		}
	}
	/*--- check if any device specified ---*/
	if( h->devCount == 0 ){
		DBGWRT_ERR((DBH, "*** %s_Init: No devices in descriptor!\n",
					BBNAME));
		return( Cleanup(h,ERR_BBIS_DESC_PARAM) );
	}

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
	CHAMELEONV2_UNIT chamUnit;		/* unit info of current module */
	u_int32 can_offset[3];
	int	CAN_NUM;
	u_int32 CAN_BAR_ADDR;
	int32 error=0, i;
	u_int32 gotSize = 0;
	if (h->pciDevNbr == EU_IO1) /*EU_IO1*/
	{
		CAN_NUM = 3;
		can_offset[0] = 0x200;
		can_offset[1] = 0x300;
		can_offset[2] = 0x400;
		CAN_BAR_ADDR = 0x90000000;
	}
	else 
	{
		CAN_NUM = 4;
		can_offset[0] = 0x300;
		can_offset[1] = 0x400;
		can_offset[2] = 0x500;
		can_offset[3] = 0x600;
		CAN_BAR_ADDR = 0x90010000;
	}
	
		
	
	DBGWRT_1((DBH, "BB - %s_BrdInit\n",BBNAME));
		
	chamUnit.devId = CHAMELEON_16Z029_CAN;		/*16z029 can bus controller*/
	chamUnit.variant = 0;		/*basic implementation*/
	chamUnit.revision = 0;		/*basic version*/

	/* location */
	chamUnit.busId = 0;			/*main bus*/
	chamUnit.instance = 0; 		/*instance number of unit (0..n) */
	chamUnit.group = 0; 		/*no group */

	/* resources */
	chamUnit.interrupt = 0; 				/*no girq, this feld has no effect*/
	chamUnit.bar = 0;					/*!< unit's address space is in BARx (0..7) */	
	chamUnit.size = 0x100;				/*!< unit's address space size */	
	chamUnit.reserved = 0; 			/*!< reserved */

	
	for( i=0; i < CAN_NUM; i++ )
	{
		DBGWRT_2((DBH," filling unit structure  for %dth CAN module\n", i));
		
		h->dev[i] = OSS_MemGet( h->osHdl,
								sizeof( CHAMELEONV2_UNIT ),
								&gotSize);
		if( !h->dev[i] ) 
		{
			DBGWRT_ERR((DBH, "*** %s_BrdInit: no ressources\n", BBNAME));
			error = ERR_OSS_MEM_ALLOC;
			goto ABORT;
		}
		
		h->devGotSize[i] = gotSize;
		
		chamUnit.offset = can_offset[i];	/*!< unit's address space offset to BARx */
		chamUnit.addr = (void*) CAN_BAR_ADDR + can_offset[i];		/*!< computed CPU address (BARx addr + offset) */	
		/*fill the unit structure here*/
		OSS_MemCopy( h->osHdl, sizeof( chamUnit ),
					 (char*)&chamUnit,
					 (char*)h->dev[i] );
	}
	/*no 16Z052 GIRQ Controller*/
	
	/* it's allright */
	return error;

 ABORT:
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
	DBGWRT_1((DBH, "BB - %s_BrdExit\n",BBNAME));

	if( h->girqVirtAddr )
	{
		error = OSS_UnMapVirtAddr( h->osHdl, (void*)&h->girqVirtAddr, BBCHAM_GIRQ_SPACE_SIZE, OSS_ADDRSPACE_MEM );
		if( error )
		{
			DBGWRT_ERR((DBH,"*** %s_Init: OSS_UnMapVirtAddr() girqVirtAddr %08x failed\n", BBNAME, h->girqVirtAddr ) );
        	goto CLEANUP;
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
			u_int32 *status = va_arg( argptr, u_int32* );

			/* no optional BBIS function do anything */
            *status = FALSE;
            break;
			funcCode++; /*dummy*/
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

			*busType = OSS_BUSTYPE_PCI;
            break;
        }

        /* device bus type */
        case BBIS_BRDINFO_DEVBUSTYPE:
        {
            u_int32 mSlot       = va_arg( argptr, u_int32 );
            u_int32 *devBusType = va_arg( argptr, u_int32* );

			*devBusType = OSS_BUSTYPE_NONE;
            break;
            mSlot++; /*dummy*/
        }

        /* interrupt capability */
        case BBIS_BRDINFO_INTERRUPTS:
        {
            u_int32 mSlot = va_arg( argptr, u_int32 );
            u_int32 *irqP = va_arg( argptr, u_int32* );

            *irqP = BBIS_IRQ_DEVIRQ;
            break;
            mSlot++; /*dummy*/
        }

        /* address space type */
        case BBIS_BRDINFO_ADDRSPACE:
        {
            u_int32 mSlot      = va_arg( argptr, u_int32 );
            u_int32 *addrSpace = va_arg( argptr, u_int32* );

			mSlot = mSlot; /* dummy access to avoid compiler warning */

#ifdef MAC_IO_MAPPED
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
 *                BBIS_CFGINFO_IRQ          interrupt parameters
 *                BBIS_CFGINFO_EXP          exception interrupt parameters
 *                BBIS_CFGINFO_SLOT			slot information
 *
 *                The BBIS_CFGINFO_BUSNBR code returns the number of the
 *                bus on which the specified device resides
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

			if ( (mSlot > CHAMELEON_BBIS_MAX_DEVS) ||
				 (h->devId[mSlot] == CHAMELEON_NO_DEV ))
				status = ERR_BBIS_ILL_SLOT;
			else
				*busNbr = h->pciBusNbr;

            break;
        }

	        /* interrupt information */
		case BBIS_CFGINFO_IRQ:
		{
			u_int32 mSlot   = va_arg( argptr, u_int32 );
			u_int32 *vector = va_arg( argptr, u_int32* );
			u_int32 *level  = va_arg( argptr, u_int32* );
			u_int32 *mode   = va_arg( argptr, u_int32* );
			
			if ( (mSlot > CHAMELEON_BBIS_MAX_DEVS) ||
				 (h->devId[mSlot] == CHAMELEON_NO_DEV )){
				status = ERR_BBIS_ILL_SLOT;
			}
			else {
				printf("cfginfor_irq through pci\n");
                *mode = BBIS_IRQ_SHARED;

                /*
                 * Take irq level to use from PCI config space instead from
                 * table inside FPGA. Useful e.g. for EM05.
                 */
                OSS_PciGetConfig( h->osHdl, h->pciBusNbr, h->pciDevNbr, 0,
                                  OSS_PCI_INTERRUPT_LINE, (int32*)level );
                if(h->pciDevNbr == EU_IO1)                
                	*level=23; /*sc15b*/
                else
					*level=22; /*sc15c*/
				
				/*printf("BUSNR %d pciDevNr %d\n", h->pciBusNbr, h->pciDevNbr);*/

				OSS_IrqLevelToVector( h->osHdl, OSS_BUSTYPE_PCI,
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

/*************************** CHAMELEON_SetIrqHandle **************************
 *
 *  Description:  Set the irq handle for BBIS.
 *
 *---------------------------------------------------------------------------
 *  Input......:  h			pointer to board handle structure
 *                irqHdl    irq handle
 *  Output.....:  return    0 | ERR_BBIS_ILL_IRQPARAM
 *  Globals....:  ---
 ****************************************************************************/
static int32 CHAMELEON_SetIrqHandle( BBIS_HANDLE *h, OSS_IRQ_HANDLE *irqHdl )
{
	int32 error = ERR_BBIS_ILL_IRQPARAM;
	
	if( irqHdl )
	{
		h->irqHdl = irqHdl;
		error = 0;
	}
	else
	{
		DBGWRT_ERR((DBH, "*** BB - %sSetIrqHandle: irqHdl is NULL\n", BBNAME ));
	}
	
	return( error );
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
	int slotShift;

	if( h->girqVirtAddr )
	{
		OSS_IRQ_STATE oldState;
		int			  offs = 0;
		
		if( h->devId[slot] == CHAMELEON_BBIS_GROUP )				
			slotShift = ((BBIS_CHAM_GRP*)h->dev[slot])->unit[0].interrupt;
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
		
		/* sanity check */
		if( h->irqHdl == NULL )
		{
			error = ERR_BBIS_ILL_IRQPARAM;
			DBGWRT_ERR((DBH, "*** BB - %s%s: SetIrqHandle must be called before\n", BBNAME,functionName ));
			goto CLEANUP;
		}
		
		/* lock critical section - disable context change i.e. to VxWorks intEnable() in the same FPGA */
		oldState = OSS_IrqMaskR(  h->osHdl, h->irqHdl );

		/* set/reset slot corresponding irq enable bit */
		irqen = MREAD_D32( h->girqVirtAddr, BBCHAM_GIRQ_IRQ_EN + offs );

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
		
		MWRITE_D32( h->girqVirtAddr, BBCHAM_GIRQ_IRQ_EN + offs, irqen );
		
		/* unlock critical section */
		OSS_IrqRestore( h->osHdl, h->irqHdl, oldState );

	    DBGWRT_1((DBH, "BB - %s%s: slot=%d enable=%d GIRQ @%08x is %08x slotShift %d\n", BBNAME,functionName,
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

	if ( (mSlot > CHAMELEON_BBIS_MAX_DEVS) ||
		 (h->devId[mSlot] == CHAMELEON_NO_DEV ))
		return ERR_BBIS_ILL_SLOT;

	if( h->devId[mSlot] == CHAMELEON_BBIS_GROUP ) {
		if( addrMode != MDIS_MA_CHAMELEON ) {
			DBGWRT_ERR((DBH,"*** %s_GetMAddr: ill addr mode for group!\n",
					BBNAME));
			return ERR_BBIS_ILL_ADDRMODE;
		}
		if( dataMode > MDIS_MD_CHAM_7 ) {
			DBGWRT_ERR((DBH,"*** %s_GetMAddr: ill data mode for group!\n",
					BBNAME));
			return ERR_BBIS_ILL_DATAMODE;
		}
		*mAddr = ((BBIS_CHAM_GRP *)h->dev[mSlot])->unit[dataMode].addr;
		*mSize = ((BBIS_CHAM_GRP *)h->dev[mSlot])->unit[dataMode].size;

	} else {
		*mAddr = ((CHAMELEONV2_UNIT*)h->dev[mSlot])->addr;
		*mSize = ((CHAMELEONV2_UNIT*)h->dev[mSlot])->size;
	}

	if( *mSize == 0 ) /* e.g. Cham V0/1 devices */
		*mSize = 0x100;    /* default value fixed */

	DBGWRT_2((DBH, " mSlot=0x%04x : mem address=0x%x, length=0x%x\n",
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
 *  Input......:  h			pointer to board handle structure
 *                mSlot     module slot number
 *                code      setstat code
 *                value     setstat value or ptr to blocksetstat data
 *  Output.....:  return    0 | error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 CHAMELEON_SetStat(
    BBIS_HANDLE     *h,
    u_int32         mSlot,
    int32           code,
    int32           value )
{
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
 *  Input......:  h			pointer to board handle structure
 *                mSlot     module slot number
 *                code      getstat code
 *  Output.....:  valueP    getstat value or ptr to blockgetstat data
 *                return    0 | error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 CHAMELEON_GetStat(
    BBIS_HANDLE     *h,
    u_int32         mSlot,
    int32           code,
    int32           *valueP )
{
    DBGWRT_1((DBH, "BB - %s_GetStat: mSlot=%d code=0x%04x\n",BBNAME,mSlot,code));

    switch (code) {
        /* get debug level */
        case M_BB_DEBUG_LEVEL:
            *valueP = h->debugLevel;
            break;

        /* ident table */
        case M_MK_BLK_REV_ID:
           *valueP = (int32)&h->idFuncTbl;
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
	return (
		"CHAMELEON - "
		"  Base Board Handler: $Id: sc15_test_bb_chameleon.c,v 1.2 2007/03/01 16:07:11 SYao Exp $" );
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
    DBGWRT_1((DBH, "BB - %s_Cleanup\n",BBNAME));

    /*------------------------------+
    |  close handles                |
    +------------------------------*/
	/* clean up desc */
	if (h->descHdl)
		DESC_Exit(&h->descHdl);

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
 *                    Note: The returned device id must be identical to the
 *                          "autoid" value in the device drivers xml file.
 *                  - Chameleon Device:
 *                    chameleon module code (see chameleon.h)
 *                    Example: 0x00000008 (CAN boromir)
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

	/* clear parameters to return */
	*occupied = 0;
	*devId    = 0;
	*devRev   = 0;
	*slotName = '\0';
	*devName  = '\0';

	/* illegal slot? */
	if( (mSlot > CHAMELEON_BBIS_MAX_DEVS) ||
		(h->devId[mSlot] == CHAMELEON_NO_DEV) ){
		DBGWRT_ERR((DBH,"*** %s_CfgInfoSlot: wrong module slot number=0x%x\n",
					BBNAME,mSlot));
		return ERR_BBIS_ILL_SLOT;
	}

	/* set occupied info */
	*occupied = BBIS_SLOT_OCCUP_ALW;

	if( h->devId[mSlot] == CHAMELEON_BBIS_GROUP )
		/* use first module of group */
		unitP = &((BBIS_CHAM_GRP*)h->dev[mSlot])->unit[0];
	else
		unitP = (CHAMELEONV2_UNIT*)h->dev[mSlot];

	*devId    = (u_int32)unitP->devId;
	*devRev   = (u_int32)unitP->revision;

	/* build slot name */
	OSS_Sprintf( h->osHdl, slotName, "cham-slot %d (is instance %d)",
		mSlot, unitP->instance);

    /* copy device name ( of first unit in case of group ) */
	/* known chameleon device? */
	if( h->devId[mSlot] != 0xffff ){
		OSS_StrCpy( h->osHdl, (char*)CHAM_DevIdToName((u_int16)*devId), devName );
	}

	DBGWRT_2((DBH," devId=0x%08x, devRev=0x%08x, devName=%s\n",
                  *devId, *devRev, devName ));

	/* return on success */
	return ERR_SUCCESS;
}

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

	for(i=0; i<h->pciPathLen; i++){

		pciDevNbr = h->pciPath[i];

		/*--- parse device ---*/
		if( (error = PciParseDev( h, pciBusNbr, pciDevNbr,
								  &vendorID, &deviceID, &headerType,
								  &secondBus )))
			return error;

		if( vendorID == 0xffff && deviceID == 0xffff ){
			DBGWRT_ERR((DBH,"*** BB - %s:ParsePciPath: Nonexistant device "
						"bus %d dev %d\n", BBNAME, pciBusNbr, pciDevNbr ));
			return ERR_BBIS_NO_CHECKLOC;
		}

		/*--- device is present, is it a bridge ? ---*/
		if( headerType != 1 ){
			DBGWRT_ERR((DBH,"*** BB - %s:ParsePciPath: Device is not a bridge!"
						"bus %d dev %d vend=0x%x devId=0x%x\n",
						BBNAME, pciBusNbr, pciDevNbr, vendorID,
						deviceID ));

			return ERR_BBIS_NO_CHECKLOC;
		}

		/*--- it is a bridge, determine its secondary bus number ---*/
		DBGWRT_2((DBH, " bus %d dev %d: vend=0x%x devId=0x%x second bus %d\n",
				  pciBusNbr, pciDevNbr, vendorID, deviceID, secondBus ));

		/*--- continue with new bus ---*/
		pciBusNbr = secondBus;
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
 *				 pciBusNbr	pci bus number
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
	int32 *secondBusP)		/* nodoc */
{
	int32 error;

	/*--- check to see if device present ---*/
	error = OSS_PciGetConfig( h->osHdl, pciBusNbr, pciDevNbr, 0,
							  OSS_PCI_VENDOR_ID, vendorIDP );

	if( error == 0 )
		error = OSS_PciGetConfig( h->osHdl, pciBusNbr, pciDevNbr, 0,
									  OSS_PCI_DEVICE_ID, deviceIDP );

	if( error )
		return PciCfgErr(h,"PciParseDev", error,
						 pciBusNbr,pciDevNbr,OSS_PCI_DEVICE_ID);

	if( *vendorIDP == 0xffff && *deviceIDP == 0xffff )
		return ERR_SUCCESS;		/* not present */

	/*--- device is present, is it a bridge ? ---*/
	error = OSS_PciGetConfig( h->osHdl, pciBusNbr, pciDevNbr, 0,
							  OSS_PCI_HEADER_TYPE, headerTypeP );

	if( error )
		return PciCfgErr(h,"PciParseDev", error,
						 pciBusNbr,pciDevNbr,OSS_PCI_HEADER_TYPE);

	DBGWRT_2((DBH, " bus %d dev %d: vend=0x%x devId=0x%x hdrtype %d\n",
			  pciBusNbr, pciDevNbr, *vendorIDP, *deviceIDP, *headerTypeP ));

	if( *headerTypeP != 1 )
		return ERR_SUCCESS;		/* not bridge device */


	/*--- it is a bridge, determine its secondary bus number ---*/
	error = OSS_PciGetConfig( h->osHdl, pciBusNbr, pciDevNbr, 0,
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
	DBGWRT_ERR((DBH,"*** BB - %s %s: PCI access error 0x%x "
				"bus %d dev %d reg 0x%x\n", BBNAME, funcName, error,
				pciBusNbr, pciDevNbr, reg ));
	return error;
}




