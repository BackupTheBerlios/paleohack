/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
//#include "lock-externs.h"
#include "paleohackRsc.h"


static void show_dungeon() SEC_1;
static void calc_map_top(Short *unit, Short *hunit, Short *map_top) SEC_1;
Boolean where_in_dungeon_map(Short scr_x, Short scr_y,
			     Short *dun_x, Short *dun_y) SEC_1;
static void show_dungeon_tap(Short blink) SEC_1;
static Boolean convert_map_char_to_dir(Char d, Short *x, Short *y) SEC_1;
static void move_map_cursor(Short dx, Short dy) SEC_1;
/* Set these to your current position at entry
 Set them to tap-location on tap (or beep if out of bounds)
 Pop up confirmation on any tap (even out of bounds)
 Also allow user to adjust position with movement keys?
 Use these to teleport to, after displaying and confirming.  */
Short dungeon_tap_x, dungeon_tap_y;


Short map_mode_teleport = 0;

void end_turn_start_turn();
Boolean Map_Form_HandleEvent(EventPtr e)
{
  Boolean handled = false;
  FormPtr frm;
  Short map_x, map_y;
    
  switch (e->eType) {

  case frmOpenEvent:
    frm = FrmGetActiveForm();
    FrmDrawForm(frm);
    show_dungeon();
    if (map_mode_teleport) {
      dungeon_tap_x = you.ux;
      dungeon_tap_y = you.uy;
      WinDrawInvertedChars("Teleport where?", 15, 1, 0);
      { Char buf[20];
      StrPrintF(buf, "%ld", moves);
      WinDrawInvertedChars(buf, StrLen(buf),
			   160 - FntCharsWidth(buf, StrLen(buf)), 160-11);
      }
    }
    handled = true;
    break;

  case penDownEvent:
    if (map_mode_teleport) {
      if (where_in_dungeon_map(e->screenX, e->screenY, &map_x, &map_y)) {
	dungeon_tap_x = dungeon_tap_y = 0;
	move_map_cursor(map_x, map_y);
      } else {
	Short btn_num;
	show_dungeon_tap(3);
	btn_num = FrmAlert(TeleMapP); // Ok, Cancel
	show_dungeon_tap(1);
	if (btn_num == 0) {
	  extern previous_state curr_state;
	  //	  map_mode_teleport = TELE_MAP;
	  LeaveForm();
	  curr_state.cmd = '\024';
	  do_xy_command(dungeon_tap_x, dungeon_tap_y);
	  //	  end_turn_start_turn();
	} else {
	  // do nothing, allow further adjustment
	}
      }
    } else
      LeaveForm();
    handled = true;
    break;

  case keyDownEvent:
    if (map_mode_teleport) {
      if (convert_map_char_to_dir(e->data.keyDown.chr, &map_x, &map_y)) {
	move_map_cursor(map_x, map_y);
      }
    }
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
static void calc_map_top(Short *unit, Short *hunit, Short *map_top)
{
  // width of map will be DCOLS units.  height of map will be DROWS hunits.
  // that's 80 cols, 22 rows

  /* Each tile gets UNIT pixels/col and HUNIT pixels/row. 
     for 160x16,      =2                   =3              */
  *unit = ScrWidth/DCOLS;
  *hunit = *unit + 1;
  *map_top = (ScrHeight - DROWS * (*hunit)) / 2;  
  if (map_mode_teleport)
    *map_top -= 26; // 25; // magic number to make the alert popup fit
}
static void show_dungeon()
{
  RectangleType r;
  Word col, row; //, i, w, h, len;
  UChar info, fltype;
  Short unit, hunit, map_top;
  //  Char c;

  RctSetRectangle(&r, 0,0, ScrWidth, ScrHeight);
  WinDrawRectangle(&r, 6);
  
  //  unit = ScrWidth/DCOLS;
  //  hunit = unit+1;
  //  map_top = (ScrHeight-DROWS*hunit)/2;
  calc_map_top(&unit, &hunit, &map_top);

  RctSetRectangle(&r, 0, map_top, ScrWidth, DROWS*hunit);
  WinEraseRectangle(&r, 0);
  WinDrawRectangleFrame(simpleFrame, &r); /* simple,round,popup,boldRound */

  /* Draw the dungeon square by square */
  for (col = 0 ; col < DCOLS ; col++) {
    for (row = 0 ; row < DROWS ; row++) {
      RctSetRectangle(&r, col*unit, row*hunit + map_top, unit,hunit);
      info = floor_info[col][row];
      // instead of iffing on info, perhaps this should 'if' on floor_symbol
      if ( /*true ||*/ (get_cell_seen(info)) ) {
	fltype = (get_cell_type(info));
	switch(fltype) {
	case CORR:
	  //	  WinDrawRectangle(&r, 0);
	  //	  break;
	  WinDrawLine(col*unit + unit, row*hunit + map_top,
		      col*unit, row*hunit + map_top + hunit);
	  WinDrawLine(col*unit, row*hunit + map_top,
		      col*unit + unit, row*hunit + map_top + hunit);
	  break;
	case STAIRS:
	  WinDrawLine(col*unit, row*hunit + map_top,
		      col*unit + unit, row*hunit + map_top + hunit);
	  break;
	  /*
	case HWALL:
	  WinDrawLine(col*unit,        row*hunit + map_top + hunit/2,
		      col*unit + unit, row*hunit + map_top + hunit/2);
	  break;
	case VWALL:
	  WinDrawLine(col*unit + unit/2, row*hunit + map_top,
		      col*unit + unit/2, row*hunit + map_top + hunit);
	  break;
	  */
	case HWALL: case VWALL:	case SDOOR: case LDOOR:
	  WinDrawRectangle(&r, 0);
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
	case DOOR:
	  // eh, doesn't quite work
	  /*
	  WinDrawLine(col*unit + 1, row*hunit + map_top + 1,
		      col*unit + unit+1, row*hunit + map_top + 1);
	  WinDrawLine(col*unit + 1, row*hunit + map_top + 1,
		      col*unit + 1,      row*hunit + map_top + hunit+1);
	  */
	  //	  WinInvertRectangle(&r, 0);
	  WinEraseRectangle(&r, 0);
	  //	  WinDrawRectangle(&r, 0);
	  break;
	}
      }
    } /* end for row */
  } /* end for col */

  /* don't forget to put the rogue somewhere */
  WinDrawRectangle(&r, 0);
  col = you.ux;
  row = you.uy;
  RctSetRectangle(&r, col*unit, row*hunit + map_top, unit,hunit);
  WinDrawRectangle(&r, 0);
  {
    Short x, y, dy[3], h = FntCharHeight();
    Boolean know_up, know_down;
    x = col*unit;
    /*
    y = map_top/2 + map_top/3;
    WinEraseLine(x, y - 4*hunit, x, y);
    WinEraseLine(x, y, x - 2*unit, y - 2*unit);
    WinEraseLine(x, y, x + 2*unit, y - 2*unit);
    y -= map_top/2;
    WinDrawInvertedChars("@", 1, x - FntCharWidth('@')/2, y - dy);
    */
    y = map_top + DROWS*hunit;
    x = you.ux*unit - FntCharWidth('@')/2;
    if (x < 0) x = 0;
    dy[0] = 0;
    dy[1] = 0;
    dy[2] = 0;
    know_up = get_cell_seen(floor_info[xupstair][yupstair]);
    know_down = get_cell_seen(floor_info[xdnstair][ydnstair]);
    if (know_up) {
      if (you.uy > yupstair) dy[0]++;
      else dy[1]++;
    }
    if (know_down) {
      if (you.uy > ydnstair) dy[0]++;
      else dy[2]++;
    }
    // if both stairs are known we might still have some adjusting to do.
    if (dy[0] && dy[1]) dy[1]++;      // 1 1 0   -->  1 2 0
    else if (dy[0] && dy[2]) dy[2]++; // 1 0 1   -->  1 0 2
    else if (dy[1] && dy[1] == dy[2]) {          //     0 1 1 or 2 0 0
      if (yupstair > ydnstair) dy[1]++; // --> 0 2 1 or 2 1 0
      else dy[2]++;                  // or --> 0 1 2 or 2 0 1
    }
    WinDrawInvertedChars("@", 1, x, y + dy[0] * h);
    if (get_cell_seen(floor_info[xupstair][yupstair])) {
      x = xupstair*unit - FntCharWidth('<')/2;
      if (x < 0) x = 0;
      WinDrawInvertedChars("<", 1, x, y + dy[1] * h);
    }
    if (get_cell_seen(floor_info[xdnstair][ydnstair])) {
      x = xdnstair*unit - FntCharWidth('>')/2;
      if (x < 0) x = 0;
      WinDrawInvertedChars(">", 1, x, y + dy[2] * h);
    }
  }
  return;
}


void map_draw_char_init()
{
  RectangleType r;
  Short unit, hunit, map_top;

  //  unit = ScrWidth/DCOLS;
  //  hunit = unit+1;
  //  map_top = (ScrHeight-DROWS*hunit)/2;
  calc_map_top(&unit, &hunit, &map_top);

  RctSetRectangle(&r, 0,0, ScrWidth, ScrHeight);
  WinDrawRectangle(&r, 6);
  RctSetRectangle(&r, 0, map_top, ScrWidth, DROWS*hunit);
  WinEraseRectangle(&r, 0);
}

#include "display.h"
void map_draw_char(Short x, Short y, Char c)
{
  Short unit, hunit, map_top;
  Short mapx, mapy;
  //  unit = ScrWidth/DCOLS;
  //  hunit = unit+1;
  //  map_top = (ScrHeight-DROWS*hunit)/2;
  calc_map_top(&unit, &hunit, &map_top);

  mapx = x * unit;
  mapy = y * hunit + map_top;
  
  // really I should fudge around a bit first...
  mapx += unit/2;
  mapy += hunit/2;
  
#ifdef I_AM_OS_2
  FntSetFont(ledFont);
#else
  FntSetFont(SmallFont);
#endif
  mapx -= FntCharWidth(c)/2;
  mapy -= FntCharHeight()/2;
  WinDrawChars(&c, 1, mapx, mapy);
#ifdef I_AM_OS_2
  FntSetFont(stdFont);
#else
  FntSetFont(BigFont);
#endif
}


Boolean where_in_dungeon_map(Short scr_x, Short scr_y,
			     Short *dun_x, Short *dun_y)
{
  Short unit, hunit, map_top;
  //  unit = ScrWidth/DCOLS;
  //  hunit = unit+1;
  //  map_top = (ScrHeight-DROWS*hunit)/2;
  calc_map_top(&unit, &hunit, &map_top);

  scr_y -= map_top;
  *dun_x = scr_x / unit;
  *dun_y = scr_y / hunit;
  return (*dun_y >= 0 && *dun_y < DROWS && /* y may be out of bounds, */
	  *dun_x >= 0 && *dun_x < DCOLS);  /* x probably won't be */
}

/* Show where the user just tapped (so we can confirm teleport) */
static void show_dungeon_tap(Short blinks)
{
  Short x = dungeon_tap_x, y = dungeon_tap_y; // used to be args, but no need.
  Short unit, hunit, map_top, mapx, mapy;
  Short delay = SysTicksPerSecond()/10;
  Short i;
  RectangleType r;

  //  unit = ScrWidth/DCOLS;
  //  hunit = unit+1;
  //  map_top = (ScrHeight-DROWS*hunit)/2;
  calc_map_top(&unit, &hunit, &map_top);

  mapx = x * unit;
  mapy = y * hunit + map_top;
  
  // really I should fudge around a bit first...
  mapx += unit/2;
  mapy += hunit/2;
  
  // Or I should make it red if you have color.. oh well.

  RctSetRectangle(&r, mapx-1, mapy-1, unit, hunit);
  for (i = 0 ; i < blinks-1 ; i++) {
    WinInvertRectangle(&r, 0);
    SysTaskDelay(delay);
  }
  WinInvertRectangle(&r, 0);

}

// handles hjklyubnHJKLYUBN and arrow keys.  (rejects < and >)
static Boolean convert_map_char_to_dir(Char d, Short *x, Short *y)
{
  Short dx = you.dx, dy = you.dy, dz = you.dz;
  Boolean ok = convert_char_to_dir(d);
  if (ok) {
    if (you.dz == 0) {
      *x = you.dx;
      *y = you.dy;
    } else
      ok = false;
  }
  you.dx = dx; you.dy = dy; you.dz = dz;
  return ok;
}

// args will be -1, 0, or 1
static void move_map_cursor(Short dx, Short dy)
{
  dungeon_tap_x += dx;
  dungeon_tap_y += dy;
  // wrap around
  if (dungeon_tap_x < 0) dungeon_tap_x = DCOLS-1;
  else if (dungeon_tap_x >= DCOLS) dungeon_tap_x = 0;
  if (dungeon_tap_y < 0) dungeon_tap_y = DROWS-1;
  else if (dungeon_tap_y >= DROWS) dungeon_tap_y = 0;
  // blink
  show_dungeon_tap(6);
}

