/* mclose.c - Mclose code */

/*  Copyright 1991 John Bovey, University of Kent at Canterbury.
 *  All Rights Reserved.
 *
 *  This file is part of MED3.
 *
 *  MED3 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  MED3 is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with UPS; if not, write to the Free Software Foundation, Inc., 59 Temple
 *  Place, Suite 330, Boston, MA  02111-1307  USA
 */


/* @(#)mclose.c	1.7 26/4/92 (UKC) */
char Men3_mclose_c_rcsid[] = "$Id$";

#include <stdio.h>
#include <stdlib.h>

#include <local/wn.h>
#include "menu3.h"
#include "menu_priv.h"

/*  Remove the menu from the list of open menus and free the menu descriptor
 *  for re-use. If the menu is currently displayed it will be removed.
 */
int
Mclose(md)
int md;
{
	if ((md < 0) || (md >= MAXMEN)) {
		menerr = MBADMD;
		return(-1);
	}
	if (_menu_[md].om_root == NULL) {
		menerr = MNOTMEN;
		return(-1);
	}
	if (_menu_[md].om_last != NULL)
		if (Mremove(md) < 0)
			return(-1);

	mdelete(_menu_[md].om_root);
	_menu_[md].om_root = NULL;
	return(0);
}

/*  Free the memory occupied by the menu.
 */
void
mdelete(menu)
MENU * menu;
{
	if (menu == NULL)
		return;
	mdelete(menu->me_topleft);
	mdelete(menu->me_botrite);
	if ((menu->me_cap != NULL) && (menu->me_flags & ME_FREC)) {
		/*  BUG: we cast "const char *" to "char *" here.
		 *  me_cap is of type const char *, because menus are
		 *  statically initialised from strings.  
		 */
		free((char *)menu->me_cap);
	}
	if (menu->me_flags & ME_FREN)
		free((char *)menu);
}
