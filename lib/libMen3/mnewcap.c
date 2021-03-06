/* mnewcap.c - code to change menu captions on the fly */

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


/* @(#)mnewcap.c	1.8 26/4/92 (UKC) */
char Men3_mnewcap_c_rcsid[] = "$Id$";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif


#include <local/wn.h>
#include "menu3.h"
#include "menu_priv.h"

static int mchange MPROTO((MENU * menu, int oldrv, int newrv,
			   const char *str, int fontno));
static int mdelete_entry MPROTO((MENU **p_menu, int rv, short *p_xshift,
				 short *p_yshift, short flags));
static int mmaketoggle MPROTO((MENU *menu, int rv, int *p_value, int value));

/*  Change the menu caption return value and font number of the menu
 *  buttons with return value oldrv. If str is NULL the caption is left
 *  unchanged, if newrv is zero the return value is unchanged and if
 *  fontno is zero the font number is left unchanged.
 *  The number of matched return values is returned.
 */
int
Mchange(md,oldrv,newrv,str,fontno)
int md, oldrv, newrv;
const char *str;
int fontno;
{
	int found;

	if ((md < 0) || (md >= MAXMEN)) {
		menerr = MBADMD;
		return(-1);
	}
	if (_menu_[md].om_root == NULL) {
		menerr = MNOTMEN;
		return(-1);
	}
	found = mchange(_menu_[md].om_root,oldrv,newrv,str,fontno);
	if (_menu_[md].om_last != NULL && found > 0)
		mshow(_menu_[md].om_root,0);
	return(found);
}

static int
mchange(menu,oldrv,newrv,str,fontno)
MENU * menu;
int oldrv, newrv;
const char *str;
int fontno;
{
	int found = 0;
	char *newcap;

	if (menu == NULL)
		return(0);
	if ((oldrv == menu->me_rv) && (menu->me_cap != NULL)) {
		if (str != NULL) {
			if (menu->me_flags & ME_FREC) {
				/* BUG: me_cap is "const char *", so
				 * we have to cast it.
				 */
				free((char *)menu->me_cap);
			}
			newcap = malloc(strlen(str) + 1);
			strcpy(newcap, str);
			menu->me_cap = newcap;
			menu->me_flags |= ME_FREC;
			menu->me_flags |= ME_REDRAW;
		}
		if (newrv != 0)
			menu->me_rv = newrv;
		if (fontno != 0) {
			menu->me_flags &= ~ME_FONT;
			menu->me_flags |= (fontno - 1) & ME_FONT;
			menu->me_flags |= ME_REDRAW;
		}
		found++;
	}
	found += mchange(menu->me_topleft,oldrv,newrv,str,fontno);
	found += mchange(menu->me_botrite,oldrv,newrv,str,fontno);
	return(found);
}

static int
mdelete_entry(p_menu,rv, p_xshift, p_yshift, flags)
MENU** p_menu;
int rv;
short* p_xshift;
short* p_yshift;
short  flags;
{
	MENU* menu = *p_menu;
	int found = 0;
	short xshift;
	short yshift;
	short* p2_xshift = p_xshift;
	short* p2_yshift = p_yshift;

	if (menu == NULL)
		return(0);

	/* First see if this is the searched for menu
	**
	** The check for me_cap is because ups's menu_data.c has
	** many menu objects with mv_rv !=0 but no captions.
	** These are probably generated by med3 when one creates
	** menu items then subdivides them, and should be ignored
	** when searching for a given item.
	*/
	if (   (rv == menu->me_rv) 
	    && (menu->me_cap != NULL) )
	{
		/*  Yes it is.
		**     Clear pointer to the menu.
		**     Unless it is a popup, adjust the shifts to
		**      account for the space the menu took up.
		*/
		*p_menu = NULL;
		if (flags & ME_VER)
		    *p_yshift += menu->me_yend- menu->me_ystart;
		else if (flags & ME_HOR)
		    *p_xshift += menu->me_xend- menu->me_xstart;
		mdelete(menu);
		return 1;
	}


	/*  Shift the start points to account for any menus already found */
	menu->me_xstart -= *p_xshift;
	menu->me_ystart -= *p_yshift;

	/* for a popup, use a fresh set of shifts */
	if (menu->me_flags & ME_POPUP)
	{
	    p2_xshift = &xshift;
	    p2_yshift = &yshift;
	    xshift = yshift = 0;
	}


	/*  See if there is a hit in the top left submenu */
	if (mdelete_entry(&menu->me_topleft, rv, p2_xshift,p2_yshift, menu->me_flags))
	{
	     found = 1;
	}
	/* Now adjust division point */
	if ( menu->me_flags & ME_VER)
	    menu->me_pos -= *p_yshift;
	else if ( menu->me_flags & ME_HOR)
	    menu->me_pos -= *p_xshift;

	/*  See if there is a hit in the bottom right submenu */
	if (mdelete_entry(&menu->me_botrite, rv, p2_xshift,p2_yshift, menu->me_flags))
	{
	     found = 1;
	}

	/* Now adjust the end points */
	menu->me_xend -= *p_xshift;
	menu->me_yend -= *p_yshift;

	/* If neither the menu nor the submenu is a popup, and there
	** is only one submenu left, copy it to the parent menu */
	if (found
	    && !(menu->me_flags & ME_POPUP)
	    && !(flags & ME_POPUP))
	{
	    if ( menu->me_topleft == NULL)
	    {
		 *p_menu = menu->me_botrite;
		 menu->me_botrite->me_parent = menu->me_parent;
		 menu->me_botrite = NULL;
		 mdelete(menu);
	    } else
	    if ( menu->me_botrite == NULL)
	    {
		 *p_menu = menu->me_topleft;
		 menu->me_topleft->me_parent = menu->me_parent;
		 menu->me_topleft = NULL;
		 mdelete(menu);
	    }
	}

	return(found);
}

int
Mdelete_entry(md,rv)
int md, rv;
{
	int found;
	short xshift = 0;
	short yshift = 0;

	if ((md < 0) || (md >= MAXMEN)) {
		menerr = MBADMD;
		return(-1);
	}
	if (_menu_[md].om_root == NULL) {
		menerr = MNOTMEN;
		return(-1);
	}
	found = mdelete_entry(&_menu_[md].om_root,rv, &xshift, &yshift, 0);
	return(found);
}

static int
mmaketoggle(menu, rv, p_value, value)
MENU* menu;
int rv;
int* p_value;
int  value;
{
	if (menu == NULL)
		return(0);

	if (   (rv == menu->me_rv)
	    && (menu->me_topleft == NULL)
	    && (menu->me_botrite == NULL))
	{
		menu->me_toggle = p_value;
		menu->me_select_val = value;
		return 1;
	} else
	    return   mmaketoggle(menu->me_topleft,rv,p_value,value)
	          || mmaketoggle(menu->me_botrite,rv,p_value,value);
}

/*
**   Make a menu option act as a toggle item.
**    The item will be displayed with a filled or empty box next to
**    it to indicate whether the option is currently on or off.
**
**    parameters:
**       md  the integer identifying the menu containing the option.
**
**       rv  the me_rv value for the menu option that becomes a toggle option.
**
**       p_value  pointer to an int that represents the current state of the
**                option.
*/
int
Mmaketoggle(md, rv, p_value)
int md;
int rv;
int* p_value;
{
	if ((md < 0) || (md >= MAXMEN)) {
		menerr = MBADMD;
		return(-1);
	}
	if (_menu_[md].om_root == NULL) {
		menerr = MNOTMEN;
		return(-1);
	}
	return  mmaketoggle(_menu_[md].om_root,rv, p_value, -1);
}


/*
**   Make menu options act as an exclusive group of options
**    The items will be displayed with a filled or empty diamond next to
**    them to indicate whether the option is currently selected.
**
**    parameters:
**       md  the integer identifying the menu containing the option.
**
**       p_value  pointer to an int that represents the current state of the
**                option.
**
**       rv1      the first me_rv value for a menu option that becomes a group option.
**       val1     the value >= 0 to indicate rv1 is active
**       rv2, val2, ...
**       end with an rv of zero
**
*/
#ifdef __STDC__
int
Mmakegroup(int md, int* p_value, ...)
{
    int rv, value;
    int ret = 0;
    va_list ap;
    va_start(ap,p_value);
#else
int
Mmakegroup(va_alist)
{
    MENU* menu;
    va_list ap;
    int md, rv, *p_value, value;
    int ret = 0;
    va_start(ap);
    md = va_arg(ap,int);
    p_value = va_arg(ap,int*);
#endif

    if ((md < 0) || (md >= MAXMEN)) {
		menerr = MBADMD;
		return(-1);
	}
    if (_menu_[md].om_root == NULL) {
		menerr = MNOTMEN;
		return(-1);
	}
    rv = va_arg(ap,int);
    while (rv && ret >= 0)
    {
	value = va_arg(ap,int);
	ret = mmaketoggle(_menu_[md].om_root,rv, p_value, value);
        rv = va_arg(ap,int);
    }
    return ret;
}

