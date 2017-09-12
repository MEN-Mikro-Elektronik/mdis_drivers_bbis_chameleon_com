/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: io_access.c
 *      Project: CHAMELEON board handler
 *
 *       Author: dieter.pfeuffer@men.de
 *        $Date: 2011/04/01 10:50:24 $
 *    $Revision: 1.2 $
 *
 *  Description: io mapped access
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: io_access.c,v $
 * Revision 1.2  2011/04/01 10:50:24  CRuff
 * R: definition of MAC_IO_MAPPED caused error during linux build because
 *    MAC_MEM_MAPPED is defined by linux build system
 * M: undef MAC_MEM_MAPPED if necessary before defining MAC_IO_MAPPED
 *
 * Revision 1.1  2011/01/19 11:02:46  dpfeuffer
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 # Copyright (c) 2011 MEN Mikro Elektronik GmbH. All rights reserved.
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

#include <MEN/men_typs.h>

#ifdef MAC_MEM_MAPPED
#undef MAC_MEM_MAPPED
#endif
#define MAC_IO_MAPPED
#include <MEN/maccess.h>

#include <MEN/oss.h>
#include <MEN/dbg.h>
#include <MEN/desc.h>
#include <MEN/bb_defs.h>
#include <MEN/bb_entry.h>
#include <MEN/bb_chameleon.h>

/**************************** __BB_CHAMELEON_IoReadD32 **********************
 *
 *  Description:  Io access read 
 *
 *---------------------------------------------------------------------------
 *  Input......:  ma		MACCESS pointer
 *                offs		offset
 *  Output.....:  return    read value
 *  Globals....:  ---
 ****************************************************************************/
extern u_int32 __BB_CHAMELEON_IoReadD32( MACCESS ma, u_int32 offs )
{
	return MREAD_D32( ma, offs );
}

/**************************** __BB_CHAMELEON_IoWriteD32 *********************
 *
 *  Description:  Io access write 
 *
 *---------------------------------------------------------------------------
 *  Input......:  ma		MACCESS pointer
 *                offs		offset
 *                val       value to write 
 *  Output.....:  ---
 *  Globals....:  ---
 ****************************************************************************/
extern void __BB_CHAMELEON_IoWriteD32( MACCESS ma, u_int32 offs, u_int32 val )
{
	MWRITE_D32( ma, offs, val );
}
