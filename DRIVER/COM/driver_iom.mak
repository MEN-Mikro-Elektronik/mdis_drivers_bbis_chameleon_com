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
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


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
