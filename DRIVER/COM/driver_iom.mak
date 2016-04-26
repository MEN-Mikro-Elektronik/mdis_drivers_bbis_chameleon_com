#***************************  M a k e f i l e  *******************************
#
#         Author: ub
#          $Date: 2005/02/16 15:34:43 $
#      $Revision: 1.1 $
#
#    Description: Makefile definitions for the CHAMELEON BBIS driver
#                 (IO mapped version)
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: driver_iom.mak,v $
#   Revision 1.1  2005/02/16 15:34:43  ub
#   Initial Revision
#
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2004 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=chameleon_iom

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/desc$(LIB_SUFFIX)  \
         $(LIB_PREFIX)$(MEN_LIB_DIR)/oss$(LIB_SUFFIX)   \
         $(LIB_PREFIX)$(MEN_LIB_DIR)/chameleon_iom$(LIB_SUFFIX) \
         $(LIB_PREFIX)$(MEN_LIB_DIR)/dbg$(LIB_SUFFIX)

MAK_SWITCH=$(SW_PREFIX)MAC_IO_MAPPED        \
           $(SW_PREFIX)CHAM_VARIANT=CHAM_IOM

MAK_INCL=$(MEN_INC_DIR)/bb_chameleon.h  \
         $(MEN_INC_DIR)/bb_defs.h   \
         $(MEN_INC_DIR)/bb_entry.h  \
         $(MEN_INC_DIR)/dbg.h       \
         $(MEN_INC_DIR)/desc.h      \
         $(MEN_INC_DIR)/mdis_api.h  \
         $(MEN_INC_DIR)/mdis_com.h  \
         $(MEN_INC_DIR)/mdis_err.h  \
         $(MEN_INC_DIR)/men_typs.h  \
         $(MEN_INC_DIR)/oss.h

MAK_INP1=bb_chameleon$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)