#include <stdio.h>
#include <local/menu3.h>

MENU nolib_men = {
	ME_FREC+ME_FREE,
	0,	/* me_pos */
	"Load library",
	'l',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	469,	/* me_xend */
	123,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
