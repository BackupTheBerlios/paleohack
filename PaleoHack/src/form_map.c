/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
//#include "lock-externs.h"
#include "paleohackRsc.h"


static void show_dungeon() SEC_1;

Boolean Map_Form_HandleEvent(EventPtr e)
{
  Boolean handled = false;
  FormPtr frm;
    
  switch (e->eType) {

  case frmOpenEvent:
    frm = FrmGetActiveForm();
    FrmDrawForm(frm);
    show_dungeon();
    handled = true;
    break;

  case penDownEvent:
    LeaveForm();
    handled = true;
    break;

  default:
    break;
  }
  return handled;
}



/**********************************************************************
                       SHOW_DUNGEON
 The "real" dungeon is 80 wide x 22 high.  each square is 2 x 3 pixels.
 **********************************************************************/
#define ScrWidth 160
#define ScrHeight 160
static void show_dungeon()
{
  RectangleType r;
  Word col, row; //, i, w, h, len;
  UChar info, fltype;
  Short unit, hunit, map_top;
  //  Char c;

  RctSetRectangle(&r, 0,0, ScrWidth, ScrHeight);
  WinDrawRectangle(&r, 6);
  unit = ScrWidth/DCOLS;
  hunit = unit+1;
  // width of map will be DCOLS units.  height of map will be DROWS hunits.
  // that's 80 cols, 22 rows

  /* Each tile gets UNIT pixels/col and HUNIT pixels/row. 
     for 160x16,      =2                   =3              */
  map_top = (ScrHeight-DROWS*hunit)/2;

  RctSetRectangle(&r, 0, map_top, ScrWidth, DROWS*hunit);
  WinEraseRectangle(&r, 0);
  WinDrawRectangleFrame(simpleFrame, &r); /* simple,round,popup,boldRound */

  /* Draw the dungeon square by square */
  for (col = 0 ; col < DCOLS ; col++) {
    for (row = 0 ; row < DROWS ; row++) {
      RctSetRectangle(&r, col*unit, row*hunit + map_top, unit,hunit);
      info = floor_info[col][row];
      // instead of iffing on info, perhaps this should 'if' on floor_symbol
      if ( true || (get_cell_seen(info)) ) {
	fltype = (get_cell_type(info));
	switch(fltype) {
	case STAIRS: case HWALL: case VWALL:
	  WinDrawLine(col*unit, row*hunit + map_top,
		      col*unit + unit, row*hunit + map_top + hunit);
	  break;
	case ROOM:
	  WinEraseRectangle(&r, 0);
	  break;
	  /*
	case HWALL:
	  WinDrawLine(col*unit + 1, row*hunit + map_top + 1,
		      col*unit + unit+1, row*hunit + map_top + 1);
	  break;
	case VWALL:
	  WinDrawLine(col*unit + 1, row*hunit + map_top + 1,
		      col*unit + 1, row*hunit + map_top + hunit+1);
	  break;
	  */
	 case CORR:
	  WinDrawRectangle(&r, 0);
	  break;
	case DOOR:
	  // eh, doesn't quite work
	  /*
	  WinDrawLine(col*unit + 1, row*hunit + map_top + 1,
		      col*unit + unit+1, row*hunit + map_top + 1);
	  WinDrawLine(col*unit + 1, row*hunit + map_top + 1,
		      col*unit + 1,      row*hunit + map_top + hunit+1);
	  */
	  //	  WinInvertRectangle(&r, 0);
	  break;
	}
      }
    } /* end for row */
  } /* end for col */

  /* don't forget to put the rogue somewhere */
  /*
  col = sotu->roguep->col * unit - 1;
  if (col < 0) col = 0;
  row = sotu->roguep->row * hunit + map_top - 1;
  RctSetRectangle(&r, col, row, 
		      unit+1, hunit+1);
  WinDrawRectangle(&r, 0);
  */
  return;
}
