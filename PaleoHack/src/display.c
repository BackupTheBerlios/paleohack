/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
#include "paleohackRsc.h"
#include "display.h"
#include "win.h"

#define ROBOTFINDSKITTEN 1 /* change in-inventory-message behavior */
//#define ROBOTFINDSKITTEN 0 /* change in-inventory-message behavior */

static void put_char_at(Short row, Short col, Char ch, Boolean bold);
void clear_visible();
static void maybe_move_visible_window(Short left_x, Short top_y);
static void ridiculous_code();
//static void wormsee(UInt tmp);
static void xor_directional();
static void print_stats_where();

static Short scrlx, scrhx, scrly, scrhy; /* corners of new area on screen */
// These four are basically consulted only by nscr,
// which currently doesn't exist, although maybe it should.

// Things for display:

/* The screen can have 15 rows, but I took 4 for stats */
#define visible_h 11
/* You can put about 19.9 m's on a memo line.. so.. we'll have 20 cols */
#define visible_w 20
#define MaxDimension visible_w
/* Height in pixels of one character ... in this case an 'M' */
#define LineHeight 11
#define visible_char_h 10
/* Width in pixels of one character ... 160 pixels / 20 col = 8 pix/col */
#define visible_char_w 8
// itsy is a smaller font
#define visible_h_itsy 18 /*17*/
#define visible_w_itsy 32
#define visible_char_h_itsy 6
#define visible_char_w_itsy 5
Short SCR_WIDTH = 160;
Short SCR_HEIGHT = 160;

Boolean itsy_on = false;
#define IsColor false /*(found_color && my_prefs.color_on)*/

// floor_symbol[][] is the equivalent of buffer[][]
// and floor_info[][] is somewhat replacing lines_dirty[].
UChar terminal[visible_h_itsy][visible_w_itsy];/*relative model of the screen*/
Short visible_x = 0, visible_y = 0;

// End of things for display.
// Things for messages:

//#define SAVED_MSGS 10 // moved to constant.h
#define SAVED_MSG_LEN 80
#define SCR_WIDTH 160


UChar MsgTopY = 0;
UChar MsgBotY = 22+1;
UChar StatsTopY = 160-22;
UChar StatsBotY = 160;
UChar DunTopY = 22+1+2;
UChar DunBotY = 160-(22+1+2); // i.e. 160 - DunTopY

Boolean pending_messages;
Char *old_messages[SAVED_MSGS];
Short last_old_message_shown;
Short command_count = 0;

/*  Char last_command = 0; */
/*  obj_t *last_command_obj = NULL; */
// these are replaced by curr_state!
extern previous_state curr_state;


// End of things for messages.

//
// Mapping from hack to this.
//
// atl        ->     print
// news0      ->     loc_symbol
// at()       ->     put_char_at
//

void print(Short x, Short y, Short ch) // Equivalent of "atl"
{
  if (OUT_OF_BOUNDS(x, y)) return;
  if (get_cell_seen(floor_info[x][y]) && floor_symbol[x][y] == ch)
    return; // so that we don't set NEW_CELL.  it already looks like that.
  floor_symbol[x][y] = ch;
  floor_info[x][y] |= NEW_CELL;  
  // I could set lines_dirty.  nah.
  on_scr(x,y);
}

void on_scr(Short x, Short y)
{
  if (x < scrlx) scrlx = x;
  if (x > scrhx) scrhx = x;
  if (y < scrly) scrly = y;
  if (y > scrhy) scrhy = y;
}


Char loc_symbol(Short x, Short y) // Equivalent of "news0"
{
  obj_t *otmp;
  trap_t *ttmp;
  UChar cell_type;
  Char c;
  Boolean blind = Blind;

  if (OUT_OF_BOUNDS(x,y)) return ' ';
  cell_type = get_cell_type(floor_info[x][y]);
  if (!get_cell_seen(floor_info[x][y]))
    c = ' ';
  else if (cell_type == POOL)
    c = POOL_SYM;
  else if (!blind && (otmp = obj_at(x,y)))
    c = otmp->olet;
  else if (!blind && gold_at(x,y))
    c = GOLD_SYM;
  else if (x == xupstair && y == yupstair)
    c = UPSTAIR_SYM;
  else if (x == xdnstair && y == ydnstair)
    c = DOWNSTAIR_SYM;
  else if ((ttmp = trap_at(x,y)) && get_trap_seen(ttmp->trap_info))
    c = TRAP_SYM;
  else switch(cell_type) {
  case HWALL: c = HWALL_SYM;  break;
  case VWALL: c = VWALL_SYM;  break;
  case CORR:  c = CORR_SYM;   break;
  case SCORR: case SDOOR:
    c = floor_symbol[x][y];	/* %% wrong after killing mimic ! */
    break;
  case LDOOR: case DOOR:
    c = DOOR_SYM;
    break;
  case ROOM:
    // the 'blind' here seems strange to me:
    if (get_cell_lit(floor_info[x][y]) || cansee(x,y) || blind) c = ROOM_SYM;
    else c = ' ';
    break;
  default:
    c = ERRCHAR;
  }
  return c;
}


Boolean fits_on_screen(Short x, Short y)
{
  Short v_h = itsy_on ? visible_h_itsy : visible_h;
  Short v_w = itsy_on ? visible_w_itsy : visible_w;
  x -= visible_x;
  y -= visible_y;
  return (x >= 0 && y >= 0 && x < v_w && y < v_h);
}


/*
 * The funtion to actually draw a character ch at a col,row position
 * (row, col are RELATIVE i.e. this is used in model-of-screen)
 * note the row+1 offset is so that we have a line for msgs.
 * The character will be drawn in either the small or normal font.
 *
 * if color is available and is on,
 * put_char_at is only called with the draw state pushed,
 * and it gets popped when finished calling put_char_at.
 */
// Equivalent of "at".
static void put_char_at(Short row, Short col, Char ch, Boolean bold)
{
  Short cheat, vcheat = DunTopY + (itsy_on ? 0 : 0);//center the map vertically
  RectangleType r;
  Short vc_w = itsy_on ? visible_char_w_itsy : visible_char_w;
  Short vc_h = itsy_on ? visible_char_h_itsy : visible_char_h;

  RctSetRectangle(&r, col * vc_w, row*vc_h+vcheat, vc_w, vc_h);

  if (!my_prefs.black_bg || IsColor)
    WinEraseRectangle(&r, 0);
  else
    WinDrawRectangle(&r, 0);
    
  // calculate pixel position of "row, col" and put char there
  cheat = vc_w - FntCharWidth(ch); // center the variable width characters

  if (cheat <= 1)   cheat = 0;
  else              cheat /= 2;

  if (ch != ' ') {

#ifdef I_AM_COLOR
    //  if (IsColor) {
    //    WinSetTextColor(get_color(ch, col+visible_x, row+visible_y));
    //  }
#endif

    if (!itsy_on && (ch== 'g' || ch== 'j' || ch== 'p' ||ch == 'q' ||ch == 'y'))
      vcheat--; // unfortunately, letters with dangling bits are a pain.
    
    if (!my_prefs.black_bg || IsColor)
      WinDrawChars(&ch, 1, col * vc_w + cheat, row * vc_h+vcheat);
    else
      WinDrawInvertedChars(&ch, 1, col * vc_w + cheat, row * vc_h+vcheat);

    if (bold)  WinInvertRectangle(&r, 0); /* 0 for square corners */
  }
  terminal[row][col] = ch;

}


// Translate a tap on the screen into absolute-dungeon coordinates.
// So that the user can ask "what's in this cell?"
void where_in_dungeon(Short scr_x, Short scr_y, Short *dun_x, Short *dun_y)
{
  Short vc_w = itsy_on ? visible_char_w_itsy : visible_char_w;
  Short vc_h = itsy_on ? visible_char_h_itsy : visible_char_h;
  scr_y -= DunTopY; // need a slight vertical shift
  *dun_x = scr_x / vc_w;
  *dun_y = scr_y / vc_h;
  *dun_x += visible_x;
  *dun_y += visible_y;
  return;
}


// x, y are dungeon coordinates (like dun_x and dun_y above)
UChar peek_at(Short x, Short y)
{
  Short row, col;
  Short v_h = itsy_on ? visible_h_itsy : visible_h;
  Short v_w = itsy_on ? visible_w_itsy : visible_w;
  if (y < visible_y || x < visible_x ||
      y >= visible_y + v_h || x >= visible_x + v_w)
    return '\0'; // the cursor position is not visible on the screen!
  row = y - visible_y;
  col = x - visible_x;
  
  return terminal[row][col];
}


/*
 * Given input SCREEN coordinates x,y, with center of screen at 0,0,
 * Translate x,y such that the onscreen location of '@' is mapped to 0,0
 */
void relativize_move(Short *x, Short *y)
{
  Short center_x, center_y, vcheat = DunTopY + (itsy_on ? 0 : 0);// same as put_char_at
  Short vc_w = itsy_on ? visible_char_w_itsy : visible_char_w;
  Short vc_h = itsy_on ? visible_char_h_itsy : visible_char_h;
  center_x = vc_w/2 + (you.ux /*char_col*/ - visible_x) * vc_w;
  center_y = vc_h/2 + (you.uy /*char_row*/ - visible_y) * vc_h + vcheat;
  *x = (*x + SCR_WIDTH/2) - center_x;
  *y = (*y + SCR_HEIGHT/2) - center_y;
}


void clear_visible()
{
  Short i, j;
  Short v_h = visible_h_itsy;
  Short v_w = visible_w_itsy;

  /* Clear the physical screen. */
  RectangleType r;
  RctSetRectangle(&r, 0, 23, 160, 114);
  qWinEraseRectangle(&r, 0);

  /* Update 'terminal' to represent the cleared screen. */
  for (i = 0 ; i < v_h; i++) {
    for (j = 0; j < v_w; j++) {
      terminal[i][j] = ' ';
    }
  }
  // do we need to set things in floor_info to "NEW"?
}
/*
 *  This will move 'terminal' either so that it's centered on the
 *  buffer-coordinate x,y, or so that the buffer-coordinate x,y is the
 *  left top position of 'terminal'.
 */
void move_visible_window(Short left_x, Short top_y, Boolean centered)
{
  Short v_h = itsy_on ? visible_h_itsy : visible_h;
  Short v_w = itsy_on ? visible_w_itsy : visible_w;

  clear_visible();
  if (centered) {
    left_x -= v_w / 2;
    top_y  -= v_h / 2;
  }
  if (left_x < 0) left_x = 0;
  if (top_y < 0) top_y = 0;
  if (left_x + v_w > DCOLS) left_x = DCOLS - v_w;
  if ( top_y + v_h > DROWS)  top_y = DROWS - v_h;
  visible_x = left_x;
  visible_y = top_y;
}
// added so that 'centered' won't redrew screen as much, I hope
static void maybe_move_visible_window(Short left_x, Short top_y)
{
  Short v_h = itsy_on ? visible_h_itsy : visible_h;
  Short v_w = itsy_on ? visible_w_itsy : visible_w;

  left_x -= v_w / 2; // alas, v_w is odd if itsy is on.
  top_y  -= v_h / 2;
  if (left_x < 0) left_x = 0;
  if (top_y < 0) top_y = 0;
  if (left_x + v_w > DCOLS) left_x = DCOLS - v_w;
  if ( top_y + v_h > DROWS)  top_y = DROWS - v_h;
  if (visible_x != left_x || visible_y != top_y) {
    clear_visible();
    visible_x = left_x;
    visible_y = top_y;
  }
}


/*
 *  This will ensure that the rogue remains "on screen",
 *  i.e. it scrolls 'terminal' so that it includes the rogue.
 * (I've increased too_small_margin so that it will scroll the screen
 * BEFORE your donut hole meets the edge of the screen.. I hope.
 * Note that this is necessary & desirable only for the x-axis, not for y.)
 */
void check_rogue_position(Boolean centered)
{
  Short v_h = itsy_on ? visible_h_itsy : visible_h;
  Short v_w = itsy_on ? visible_w_itsy : visible_w;
  //  Short char_diameter = itsy_on ? visible_char_w_itsy : visible_char_w;
  Short too_small_margin = 1;
  Short x_margin = 0;//(my_prefs.walk_search_border * 10) / char_diameter;
  if (x_margin > 7) x_margin = 7;

  if ((you.ux < visible_x) || (you.ux >= visible_x + v_w) ||
      (you.uy < visible_y) || (you.uy >= visible_y + v_h)) {
    /* I guess the rogue teleported or something */
    //    clear_visible(); // moved to move_visible_window
    move_visible_window(you.ux, you.uy, true);
    return;
  } else if (centered) {
    maybe_move_visible_window(you.ux, you.uy);
    return;
  }
  /* The rogue is able to move off the screen from here, so scroll it. */
  // I'm getting TOO MUCH redrawing.  WHY.
  if ((0 != visible_x) &&
      (you.ux <= visible_x + too_small_margin + x_margin)) {
    clear_visible();
    visible_x = max(visible_x - v_w / 2, 0);
  } else if ((DCOLS - v_w != visible_x) &&
	     (you.ux >= visible_x+(v_w-1) - (too_small_margin+x_margin))){
    clear_visible();
    visible_x = min(visible_x + v_w / 2, DCOLS - v_w);
  }
  if ((0 != visible_y) &&
      (you.uy <= visible_y + too_small_margin)) {
    clear_visible();
    visible_y = max(visible_y - v_h / 2, 0);
  } else if ((DROWS - v_h != visible_y) && 
	     (you.uy >= visible_y + (v_h-1) - too_small_margin)) {
    clear_visible();
    visible_y = min(visible_y + v_h / 2, DROWS - v_h);
  }
}


void refresh() // formerly known as docrt()
{
  Short col, line, y_abs, x_abs;
  Short v_h = itsy_on ? visible_h_itsy : visible_h;
  Short v_w = itsy_on ? visible_w_itsy : visible_w;
  UChar info;

  if (FrmGetActiveFormID() != MainForm) return; // XXX

  if (you.uswallow) {
    swallowed();
    return;
  }
  ridiculous_code();

#ifdef I_AM_OS_2
  if (itsy_on) FntSetFont(ledFont);
#else
  if (itsy_on) FntSetFont(SmallFont);
#endif

  /* line is RELATIVE, line+visible_y is ABSOLUTE.
     col is RELATIVE, col+visible_x is ABSOLUTE. */
  for (line = 0 ; line < v_h ; line++) {
    y_abs = line + visible_y;
    for (col = 0; col < v_w; col++) {
      x_abs = col + visible_x;
      info = floor_info[x_abs][y_abs];
      if (info & (SEEN_CELL | NEW_CELL)) // SEEN_CELL in case we clear_visibled
	if (floor_symbol[x_abs][y_abs] != terminal[line][col]) {
	  put_char_at(line, col, floor_symbol[x_abs][y_abs], false);
      }
    }
  }
  scrlx = DCOLS;
  scrly = DROWS;
  scrhx = scrhy = 0;

#ifdef I_AM_OS_2
  if (itsy_on) FntSetFont(stdFont);
#else
  if (itsy_on) FntSetFont(BigFont);
#endif
}

// Not good to basically have two independent "refresh" procedures... :-b
// hmmm in the original, pline called nscr!
void nscr()
{
  Short x_abs, y_abs, col, line;
  Short v_h = itsy_on ? visible_h_itsy : visible_h;
  Short v_w = itsy_on ? visible_w_itsy : visible_w;

  if (FrmGetActiveFormID() != MainForm) return; // XXX
  // XXX flags.nscrinh is used only in goto_level()  ..will need it though.
  if (you.uswallow || you.ux == FAR /*|| flags.nscrinh*/) return;
#ifdef I_AM_OS_2
  if (itsy_on) FntSetFont(ledFont);
#else
  if (itsy_on) FntSetFont(SmallFont);
#endif
  pru();
  for (y_abs = scrly ; y_abs <= scrhy ; y_abs++) {
    line = y_abs - visible_y;
    if (line < 0 || line >= v_h) continue;
    for (x_abs = scrlx ; x_abs <= scrhx ; x_abs++) {
      col = x_abs - visible_x;
      if (col < 0 || col >= v_w) continue;
      if (get_cell_new(floor_info[x_abs][y_abs])) {
	//	room = &levl[x_abs][y_abs]
	floor_info[x_abs][y_abs] &= ~NEW_CELL; // room->new = 0;
	put_char_at(line, col, floor_symbol[x_abs][y_abs], false);
      }
    }
  }
  scrlx = DCOLS;
  scrly = DROWS;
  scrhx = scrhy = 0;
#ifdef I_AM_OS_2
  if (itsy_on) FntSetFont(stdFont);
#else
  if (itsy_on) FntSetFont(BigFont);
#endif
}



// When something outside display.c wants to call 'at' aka put_char_at
// to draw something briefly, this is probably what it wants...
// Draw 1 cell on screen (here row,col are NOT screen-relative.)
// For making animatedness. If c = 0, it will set cell to 'original' value
void animate_char(Short y, Short x, Char c, Boolean bold)
{
  Short row, col, ch;
  Short v_h = itsy_on ? visible_h_itsy : visible_h;
  Short v_w = itsy_on ? visible_w_itsy : visible_w;

  if (FrmGetActiveFormID() != MainForm) return; // XXX

  if (y < visible_y || x < visible_x ||
      y >= visible_y + v_h || x >= visible_x + v_w)
    return; // the cursor position is not visible on the screen!

  //  if (c != 0 && FrmGetActiveFormID() == MainForm)
  //    lines_dirty[y] = screen_dirty = true;// worst case, refresh will clean up

  row = y - visible_y;
  col = x - visible_x;
  
  ch = terminal[row][col];
  if (c == 0) c = ch;

#ifdef I_AM_OS_2
  if (itsy_on) FntSetFont(ledFont);
#else
  if (itsy_on) FntSetFont(SmallFont);
#endif
  /*
#ifdef I_AM_COLOR
  if (IsColor) {
    start_color();
  }
#endif //I_AM_COLOR
  */
  put_char_at(row, col, c, bold);
  /*
#ifdef I_AM_COLOR
  if (IsColor) {
    stop_color();
  }
#endif //I_AM_COLOR
  */
#ifdef I_AM_OS_2
  if (itsy_on) FntSetFont(stdFont);
#else
  if (itsy_on) FntSetFont(BigFont);
#endif
  terminal[row][col] = ch;
}


// Extremely funky animation routines
// This is for an "unidentified flying object"
typedef struct anim_s {
  Char let;
  coord prev;
} anim_state;
anim_state my_ufo;
static void tmp_at_erase(Int8 px, Int8 py) SEC_5;
void tmp_at_init(Char c) // Replaces the old "tmp_at(-1, c)"
{
  my_ufo.let = c;
  my_ufo.prev.x = -1;
}
void tmp_at_newsymbol(Char c) // Replaces the old "tmp_at(-2, c)"
{
  my_ufo.let = c;
}
void tmp_at_cleanup() // Replaces the old "tmp_at(-1,-1)"
{
  tmp_at_erase(my_ufo.prev.x, my_ufo.prev.y);
  my_ufo.let = '\0';
  my_ufo.prev.x = -1;
}
static void tmp_at_erase(Int8 px, Int8 py)
{
  if (px >= 0 && cansee(px, py)) {
    //    delay_output(); // ??
    //    prl(px, py);	/* in case there was a monster */
    //    animate_char(py, px, floor_symbol[px][py], false); // XXXXXXXXXXXXX
    animate_char(py, px, 0, false); // XXXXXXXXXXXXX
  }
}
void tmp_at(Int8 x, Int8 y)
{
  tmp_at_erase(my_ufo.prev.x, my_ufo.prev.y);
  if (cansee(x, y)) {
    //    WinDrawChars(&my_ufo.let, 1, 10, 10); // XXX debug
    animate_char(y, x, my_ufo.let, false);
  }
  SysTaskDelay(SysTicksPerSecond()/16); // probably a nonoptimal location/delay
  my_ufo.prev.x = x;
  my_ufo.prev.y = y;
}

// This is for a "beam" (LIFO erasure)
typedef struct animation_s {
  Char let;
  Short cnt;
  coord tc[DCOLS];		/* but watch reflecting beams! */
} animation_state;
animation_state my_anim;

// The routine Tmp_at is like tmp_at, except the erasure is LIFO.
void Tmp_at_init(Char c) // Replaces the old "Tmp_at(-1, c)"
{
  my_anim.let = c;
  my_anim.cnt = 0;
}
void Tmp_at_newsymbol(Char c) // Replaces the old "Tmp_at(-2, c)"
{
  my_anim.let = c;
}
void Tmp_at_cleanup() // Replaces the old "Tmp_at(-1, 0)" or -1,-1...
{
  Short xx,yy;
  SysTaskDelay(SysTicksPerSecond()/16);
  while (my_anim.cnt--) {
    xx = my_anim.tc[my_anim.cnt].x;
    yy = my_anim.tc[my_anim.cnt].y;
    SysTaskDelay(SysTicksPerSecond()/32);
    animate_char(yy, xx, 0, false); // 'erase' the animated character
  }
}
void Tmp_at(Int8 x, Int8 y)
{
  if (cansee(x,y)) {
    //    if (my_anim.cnt) delay_output();
    animate_char(y, x, my_anim.let, false);
    SysTaskDelay(SysTicksPerSecond()/32);
    my_anim.tc[my_anim.cnt].x = x;
    my_anim.tc[my_anim.cnt].y = y;
    //    if (++my_anim.cnt >= MaxDimension) panic("Tmp_at overflow?");
    if (++my_anim.cnt >= DCOLS) my_anim.cnt = DCOLS-1;
    // levl[x][y].new = 0;	/* prevent pline-nscr erasing --- */
    floor_info[x][y] &= ~NEW_CELL;  /* prevent pline-nscr erasing --- */
  }
}



////////////////////////////
//// These are wacky things from hack/pri.c.  names unchanged unless noted.
////////////////////////////
Char swallowed_str[] = { '/', EMDASH, '\\', '|', '@', '|', '\\', EMDASH, '/' };
void swallowed()
{
  // needs testing .... also, find out what this really looks like in unix.
  Short line, col, dy, dx, i = 0;
  if (FrmGetActiveFormID() != MainForm) return; // XXX
  clear_visible();
#ifdef I_AM_OS_2
  if (itsy_on) FntSetFont(ledFont);
#else
  if (itsy_on) FntSetFont(SmallFont);
#endif
  for (line = you.uy - visible_y, dy = -1 ; dy <= 1 ; dy++)
    for (col = you.ux - visible_x, dx = -1 ; dx <= 1 ; dx++)
      put_char_at(line+dy, col+dx, swallowed_str[i++], false);
  put_char_at(line, col, you.usym, false);
#ifdef I_AM_OS_2
  if (itsy_on) FntSetFont(stdFont);
#else
  if (itsy_on) FntSetFont(BigFont);
#endif
}

void prme()
{
  if (!Invisible) print(you.ux, you.uy, you.usym);  
}

static void ridiculous_code()
{
  /* "Some ridiculous code to get display of @ and monsters (almost) right" */
  monst_t *mtmp;
  if (!Invisible) {
    you.udisx = you.ux;
    you.udisy = you.uy;
    floor_symbol[you.ux][you.uy] = you.usym;
    floor_info[you.ux][you.uy] |= SEEN_CELL;
    you.udispl = true;
  } else
    you.udispl = false;

  seemons();	/* reset old positions */
  for (mtmp = fmon ; mtmp ; mtmp = mtmp->nmon)
    mtmp->bitflags &= ~M_IS_DISPLAYED; // === mtmp->mdispl = 0;
  seemons();	/* force new positions to be shown */
  /* "This nonsense should disappear soon ---------------------------------" */
  // Yeah, right.
}

Boolean vism_at(Short x, Short y)
{
  monst_t *mtmp;

  return((x == you.ux && y == you.uy && !Invisible)
	 ? true :
	 (mtmp = mon_at(x,y))
	 ? ((Blind && Telepat) || canseemon(mtmp)) :
	 false);
}





void pru()
{
  if (you.udispl &&
      (Invisible || you.udisx != you.ux || you.udisy != you.uy)) {
    /* if (! levl[u.udisx][u.udisy].new) */ // this line was commented out.
    if (!vism_at(you.udisx, you.udisy))
    newsym(you.udisx, you.udisy);
  }
  if (Invisible) {
    you.udispl = false;
    prl(you.ux, you.uy);
  } else if (!you.udispl || you.udisx != you.ux || you.udisy != you.uy) {
    print(you.ux, you.uy, you.usym);
    you.udispl = true;
    you.udisx = you.ux;
    you.udisy = you.uy;
  }
  floor_info[you.ux][you.uy] |= SEEN_CELL;
}


/* print a (single-cell?) position that is visible for @ */
#ifndef NOWORM
extern wseg_t *m_atseg; // invent.c
#endif NOWORM
void prl(Short x, Short y)
{
  monst_t *mtmp;
  obj_t *otmp;
  UChar cell_type;

  if (OUT_OF_BOUNDS(x, y)) return; // formerly '!isok'

  if (x == you.ux && y == you.uy && (!Invisible)) {
    pru();
    return;
  }
  cell_type = get_cell_type(floor_info[x][y]);
  if ((!cell_type) ||
      (IS_ROCK(cell_type) &&
       get_cell_type(floor_info[you.ux][you.uy]) == CORR))
    return;
  if ((mtmp = mon_at(x,y)) &&
      !(mtmp->bitflags & M_IS_HIDER) &&
      (!(mtmp->bitflags & M_IS_INVISIBLE) || See_invisible)) {
#ifndef NOWORM
    if (m_atseg)
      pwseg(m_atseg);
    else
#endif NOWORM
      pmon(mtmp);
  } else if ((otmp = obj_at(x,y)) && cell_type != POOL)
    print(x,y,otmp->olet);
  else if (mtmp && (!(mtmp->bitflags & M_IS_INVISIBLE) || See_invisible)) {
    /* must be a hiding monster, but not hiding right now */
    /* assume for the moment that long worms do not hide */
    pmon(mtmp);
  }
  else if (gold_at(x,y) && cell_type != POOL)
    print(x, y, GOLD_SYM);
  else if (!get_cell_seen(floor_info[x][y]) || floor_symbol[x][y] == ' ') {
    floor_info[x][y] |= (NEW_CELL | SEEN_CELL);
    newsym(x,y);
    on_scr(x,y);
  }
  floor_info[x][y] |= SEEN_CELL;
}



void newsym(Short x, Short y)
{
  print(x, y, loc_symbol(x,y));
}

/* used with wand of digging (or pick-axe): fill scrsym and force display */
/* also when a POOL evaporates */
void mnewsym(Short x, Short y)
{
  Char newscrsym;

  if (!vism_at(x,y)) {
    newscrsym = loc_symbol(x,y);
    if (floor_symbol[x][y] != newscrsym) {
      floor_symbol[x][y] = newscrsym;
      floor_info[x][y] &= ~SEEN_CELL; // make cell not-yet-seen
    }
  }
}

// turns off a "lighted" room square.
void nosee(Short x, Short y)
{
  if (OUT_OF_BOUNDS(x,y)) return;
  if (floor_symbol[x][y] == '.' && !get_cell_lit(floor_info[x][y]) && !Blind) {
    // print(x, y, ' '); // should be same as these three lines:
    floor_symbol[x][y] = ' ';
    floor_info[x][y] |= NEW_CELL;  
    on_scr(x,y);
  }
}


void prl1(Short x, Short y)
{
  if (you.dx) {
    if (you.dy) {
      prl(x-(2*you.dx), y);
      prl(x-you.dx, y);
      prl(x, y);
      prl(x, y-you.dy);
      prl(x, y-(2*you.dy));
    } else {
      prl(x, y-1);
      prl(x, y);
      prl(x, y+1);
    }
  } else {
    prl(x-1, y);
    prl(x, y);
    prl(x+1, y);
  }
}

// 
void nose1(Short x, Short y)
{
  if (you.dx) {
    if (you.dy) {
      nosee(x, you.uy);
      nosee(x, you.uy-you.dy);
      nosee(x, y);
      nosee(you.ux-you.dx, y);
      nosee(you.ux, y);
    } else {
      nosee(x, y-1);
      nosee(x, y);
      nosee(x, y+1);
    }
  } else {
    nosee(x-1, y);
    nosee(x, y);
    nosee(x+1, y);
  }
}

void unpobj(obj_t *obj)
{
  // This was commented out:
  /* 	if (obj->odispl){
	if (!vism_at(obj->odx, obj->ody))
	newsym(obj->odx, obj->ody);
	obj->odispl = 0;
	}
  */
  if (!vism_at(obj->ox,obj->oy))
    newsym(obj->ox,obj->oy);
}


void seeobjs() // WHY is this called seeobjs?  It should be decay_corpses!
{
  obj_t *obj, *obj2;
  for (obj = fobj ; obj ; obj = obj2) {
    obj2 = obj->nobj;
    if (obj->olet == FOOD_SYM && obj->otype >= CORPSE
	&& obj->age + 250 < moves)
      delobj(obj);
  }
  for (obj = invent ; obj ; obj = obj2) {
    obj2 = obj->nobj;
    if (obj->olet == FOOD_SYM && obj->otype >= CORPSE
	&& obj->age + 250 < moves)
      useup(obj);
  }
}


//static void wormsee(UInt tmp) { return; } // fake...

void seemons()
{
  monst_t *mtmp;
  for (mtmp = fmon ; mtmp ; mtmp = mtmp->nmon) {
    if (mtmp->data->mlet == ';') {
      // decide whether monster is invisible
      if (you.ustuck != mtmp && 
	  get_cell_type(floor_info[mtmp->mx][mtmp->my]) == POOL)
	mtmp->bitflags |= M_IS_INVISIBLE;
      else mtmp->bitflags &= ~M_IS_INVISIBLE;
    }
    pmon(mtmp);
#ifndef NOWORM
    if (mtmp->wormno) wormsee(mtmp->wormno);
#endif NOWORM
  }
}

void pmon(monst_t *mon)
{
  Short show = (Blind && Telepat) || canseemon(mon);
  if (mon->bitflags & M_IS_DISPLAYED) {
    if (mon->mdx != mon->mx || mon->mdy != mon->my || !show)
      unpmon(mon);
  }
  if (show && !(mon->bitflags & M_IS_DISPLAYED)) {
    Char c;
    if (!mon->mappearance ||
	you.uprops[PROP(RIN_PROTECTION_FROM_SHAPE_CHANGERS)].p_flags)
      c = mon->data->mlet;
    else
      c = mon->mappearance;
    print(mon->mx,mon->my, c);
    mon->bitflags |= M_IS_DISPLAYED;
    mon->mdx = mon->mx;
    mon->mdy = mon->my;
  }
}

void unpmon(struct monst *mon)
{
  if (mon->bitflags & M_IS_DISPLAYED) {
    newsym(mon->mdx, mon->mdy);
    mon->bitflags &= ~M_IS_DISPLAYED;
  }
}




/*
 * Switch between the small font and the regular font.
 * (Do NOT allow the small font if the SDK that I compiled with
 * is inappropriate for your operating system version.)
 */
Boolean toggle_itsy()
{
  DWord version;
  FtrGet(sysFtrCreator, sysFtrNumROMVersion, &version);
#ifdef I_AM_OS_2
  if (version < 0x03000000l) {
    itsy_on = !itsy_on;
    return true;
  } else itsy_on = false;
#else
  if (version >= 0x03000000l) {
    itsy_on = !itsy_on;
    return true;
  } else itsy_on = false;
#endif
  return false;
}



Boolean draw_directional_p;
Boolean undraw_directional_p;
void draw_directional()
{
  //  XOR some lines onto the screen
  // centered around 80,80
  switch(curr_state.cmd) {
  case 'C':
    alert_message("Using camera...");
    break;
  case 'P':
    alert_message("Digging...");
    break;
  case 'z':
    alert_message("Zapping wand...");
    break;
  case 't':
    alert_message("Throwing...");
    break;
  case '^':
    alert_message("Identify trap...");
    break;
    // ...
  }
  xor_directional();
  draw_directional_p = false;
  undraw_directional_p = true;
  flags.botl = BOTL_NONE;
}
void undraw_directional()
{
  xor_directional(); // .. hopefully nothing has changed!
  //  flags.botl = BOTL_ALL;
  undraw_directional_p = false;
}
static void xor_directional()
{
  WinInvertLine(0,  40, 60, 70);
  WinInvertLine(100,90,160,120);
  WinInvertLine(0,  120,   60, 90);
  WinInvertLine(100, 70,  160, 40);
  WinInvertLine(40,  0, 70, 60);
  WinInvertLine(90,100,120,159);
  WinInvertLine(120,  0,90, 60);
  WinInvertLine(70, 100,40,159);
}


///////////////////////////////////////////////////////////////////////
//   End of display, start of messages and stats.
///////////////////////////////////////////////////////////////////////



void level_message(Char *buf)
{
  RectangleType r;
  Short y;
  RctSetRectangle(&r, 0, SCR_HEIGHT/2 - visible_char_h,
		  SCR_WIDTH, 2*visible_char_h);
  qWinEraseRectangle(&r, 0); /* 0 for square corners */
  y = SCR_HEIGHT/2 - visible_char_h;
  qWinDrawLine(0, y, SCR_WIDTH, y);
  y = SCR_HEIGHT/2 + visible_char_h;
  qWinDrawLine(0, y, SCR_WIDTH, y);
  // Perhaps this should be centered.
  qWinDrawChars(buf, StrLen(buf), 1, (SCR_HEIGHT - visible_char_h)/2);
  message(buf); // for good measure
}

// This is a buffer for everyone who wants to do a StrPrintF into message.
// So that you don't need to allocate one in everyone's stack.
//Char ScratchBuffer[BIGBUF]; // (definitely needs to be way more than 80)
Char ScratchBuffer[BIGBUF]; // (definitely needs to be way more than 80)

void message(const Char *buf)
{
  Word curfrm = FrmGetActiveFormID();
  Short i, new_msgs_ctr = 1;
  Char *tmp;
  if (curfrm == MainForm ||
      curfrm == SenseForm ||
      curfrm == Chargen1Form ||
      curfrm == Chargen2Form ||
      curfrm == InvForm ||
      curfrm == InvMsgForm ||
      curfrm == InvActionForm) {    // Log a message for later display
    if (!buf) return;
    tmp = old_messages[0];
    for (i = 0 ; i < SAVED_MSGS-1 ; i++)
      old_messages[i] = old_messages[i+1];
    StrNCopy(tmp, buf, SAVED_MSG_LEN-2); /* leave room for \n.. */
    tmp[SAVED_MSG_LEN-2] = '\0'; /* making sure it's terminated.. */
    if (FntCharsWidth(tmp, StrLen(tmp)) <= SCR_WIDTH)
      StrCat(tmp, "\n");
    else {
      // hmmm may need to split it into two messages, IF it is longer 
      // than TWO screen widths when word-wrapped.
      Short wrap_len = FntWordWrap(tmp, SCR_WIDTH);//How much fits on 1st line.
      if (FntCharsWidth(tmp+wrap_len, StrLen(tmp+wrap_len)) > SCR_WIDTH) {
	new_msgs_ctr++;
	// This message would require three (or more) lines.  (it's a fortune.)
	// Split it after the second line.
	wrap_len += FntWordWrap(tmp+wrap_len, SCR_WIDTH);
	// Truncate here.  Copy the remainder from buf to a new 'tmp'.
	if (wrap_len >= SAVED_MSG_LEN-2) wrap_len = SAVED_MSG_LEN-2;
	tmp[wrap_len] = '\n';
	tmp[wrap_len-1] = '!'; // XXX DEBUGGING
	tmp[wrap_len+1] = '\0';
	old_messages[SAVED_MSGS-1] = tmp;	
	tmp = old_messages[0];
	for (i = 0 ; i < SAVED_MSGS-1 ; i++)
	  old_messages[i] = old_messages[i+1];
	StrNCopy(tmp, buf+wrap_len, SAVED_MSG_LEN-2); /* leave room for \n.. */
	tmp[SAVED_MSG_LEN-2] = '\0'; /* making sure it's terminated.. */
	// of course probably they're reversed now or something.  sigh.
      }
      StrCat(tmp, "\n");
    }
    old_messages[SAVED_MSGS-1] = tmp;
    // this part is for "--more--" ability:
    if ((curfrm != InvForm && curfrm != InvActionForm) || ROBOTFINDSKITTEN)
      // don't re-show inv messages on exit!  also...
      if (command_count <= 0 || (0 != StrCompare(old_messages[SAVED_MSGS-1],
						 old_messages[SAVED_MSGS-2])))
	// don't --more-- a count_message if it's the same as the previous one
	last_old_message_shown -= new_msgs_ctr;
    if (last_old_message_shown < 0) // "none of these msgs have been shown"
      last_old_message_shown = -1;
    // Remember that we have something to display
    pending_messages = true;
    command_count = 0;  // (lame hack!)
  } // (Else you're in a form that DOESN'T LOG MESSAGES.)

  if (curfrm == InvForm || curfrm == InvActionForm) {
    // These messages should also be printed straightaway.
    // XXX or maybe I should pop up a message window, like robotfindskitten?
    if (!ROBOTFINDSKITTEN) {
      RectangleType r;
      RctSetRectangle(&r,0,128,156,11);
      WinEraseRectangle(&r, 0);
      if (buf)
	WinDrawChars(buf, StrLen(buf), 1, 128);
    }
    // Make sure there's at least a moment to read it, if we have
    // several messages to print ("You wear x" "Oops, cursed!" "The foo hits")
    // SysTaskDelay(SysTicksPerSecond());
  }
}
// a mainform message that we require to be displayed in "real time"
void alert_message(Char *buf)
{
  RectangleType r;
  if (FrmGetActiveFormID() != MainForm || !buf) return;
  //  RctSetRectangle(&r,0,134,160,160-134); /* left,top, width and height */
  //RctSetRectangle(&r,0,StatsTopY,160,MsgBotY-StatsTopY);/* x,y, W, and H */
  RctSetRectangle(&r, 0, MsgTopY, 160, 11+4); /* x,y, W, and H */
  // XXX Why is that "11+4" ???
  RctSetRectangle(&r, 0, MsgTopY, 160, 11+11); /* x,y, W, and H */
  qWinEraseRectangle(&r, 0); /* 0 for square corners */
  //  if (buf)
  //    WinDrawChars(buf, StrLen(buf), 1, 134);
  qWinDrawChars(buf, StrLen(buf), 1, MsgTopY);
}

// Clear the message area.  Return true if there are more messages.
Boolean message_clear(Boolean really)
{
  RectangleType r;
  //  RctSetRectangle(&r,0,134,160,160-134); /* left,top, width and height */
  RctSetRectangle(&r,0,MsgTopY,160,MsgBotY-MsgTopY); /* x,y, W, and H */
  if (really)
    qWinEraseRectangle(&r, 0);
  //  WinDrawLine(0,133,160,133);
  qWinDrawLine(0,MsgBotY,160,MsgBotY);
  //print_stats(STATS_QUO);
  if (last_old_message_shown < SAVED_MSGS-1)  return true;
  else return false;
}

/*
 * Blow away the message queue
 * (Use this when you die)
 */
void preempt_messages()
{
  pending_messages = false;
  last_old_message_shown = SAVED_MSGS - 1;  
}

// should be working properly now (don't call show_messages after calling it!)
void show_all_messages()
{
  while (last_old_message_shown < SAVED_MSGS-1) {
    show_messages();
    if (last_old_message_shown < SAVED_MSGS-1) {
      show_a_more(SCR_WIDTH, 2*LineHeight, true); // invert the more
      SysTaskDelay((3*SysTicksPerSecond())/2); // XXXX
    }
  }
}

// Print up to 2 lines of messages.  make sure you word-wrap.
// 1. print first message
// 2. if necessary, word wrap
// 3. else if second message exists + does not need word wrap, print it too.
// 4. else if second message does not exist, print teensy stats.
// 5. print "--more--" if any more messages are in the queue
void show_messages()
{
  Short lines_avail = 2, len_line, len_str, lines_used = 0;
  Short str_width = SCR_WIDTH;
  Char *msg;
  Word curfrm = FrmGetActiveFormID();

  RectangleType r;
  //RctSetRectangle(&r,0,134,160,160-134); /* left,top, width and height */
  RctSetRectangle(&r,0,MsgTopY,160,MsgBotY-MsgTopY); /* x,y, W, and H */

  if (curfrm != MainForm && curfrm != SenseForm) { // xxxx adding SenseForm.
    if (ROBOTFINDSKITTEN && (curfrm == InvForm || curfrm == InvActionForm)
	&& !you.dead) 
      FrmPopupForm(InvMsgForm);
    return; // XXX otherwise inventory form title is lost
  }

  // if there's nothing to print, just print stats.
  if (last_old_message_shown >= SAVED_MSGS-1) {
    //    print_stats_real(need_print_stats, true);
    message_clear(true); //print_stats(0);
  } else {
    qWinEraseRectangle(&r, 0);
    //    WinDrawLine(0,133,160,133);
    qWinDrawLine(0,MsgBotY,160,MsgBotY);
    while (lines_avail > 0 && last_old_message_shown < SAVED_MSGS-1) {
      msg = old_messages[last_old_message_shown+1];
      len_str = StrLen(msg);
      if (lines_avail <= 1 && FntCharsWidth(msg, len_str-1) > SCR_WIDTH)
	break; // 1 line avail., message is 2 lines.  Do not print partial msg.
      while (lines_avail > 0) {
	len_str = StrLen(msg);
	len_line = FntWordWrap(msg, SCR_WIDTH);
	if (len_line >= len_str) { // we have finished the string!
	  //	  WinDrawChars(msg, len_str-1, 0, 134 + 11*lines_used);
	  qWinDrawChars(msg, len_str-1, 0, MsgTopY + 11*lines_used);
	  str_width = FntCharsWidth(msg, len_str-1); // remember for print..hp!
	  lines_avail--; lines_used++;
	  break;
	}
	//	WinDrawChars(msg, len_line, 0, 134 + 11*lines_used);
	qWinDrawChars(msg, len_line, 0, MsgTopY + 11*lines_used);
	str_width = FntCharsWidth(msg, len_line); // remember for print_s._hp!
	lines_avail--; lines_used++;
	msg += len_line;
      }    // we finished the string, or we ran out of space.
      last_old_message_shown++;
    } // finished all messages or ran out of space.
    if ((last_old_message_shown < SAVED_MSGS-1)/* || (last_command == 'I')*/)
      //show_a_more(lines_used); // prompt for the Any Key with "--more--"
      show_a_more(SCR_WIDTH, 2*LineHeight, false);//prompt for the Any Key with "--more--"
    else {
      if (lines_used > 1)	// see if there was room for hit points
	; //print_stats_hp(str_width); // this will decide whether to print them
    }
    if (lines_used < 2) {
      ; // print_stats(0);
      // this should turn on CURSTAT_ITSY in current_print_stats.
    } else
      ; // current_print_stats = CURSTAT_MSG;
  }
  pending_messages = false;
  //  need_print_stats = STATS_NONE; // print_stats_real does this
}
// This is what prints the MORE in lower right corner of screen.
// A couple non-display.c routines also would like to use it..
void show_a_more(Short w, Short y0, Boolean invert)
{
  DWord version;
  Boolean large = false;
  Short x, y;
  FtrGet(sysFtrCreator, sysFtrNumROMVersion, &version);
#ifdef I_AM_OS_2
  if (version < 0x03000000l) FntSetFont(ledFont);
  else large = true;
#else
  if (version >= 0x03000000l) FntSetFont(129);
  else large = true;
#endif
  //    WinDrawChars("--more--", 8, 120, 134 + 11*lines_used);
  //WinDrawChars("MORE", 8, (large ? 130 : 140), 134 + 11*lines_used - 3);
  x = w - (large ? 30 : 20);
  //  y = MsgTopY + 11*lines_used - 5;
  y = MsgTopY + y0 - 5;
  if (invert)
    qWinDrawInvertedChars("MORE", 4, x, y);
  else
    qWinDrawChars("MORE", 4, x, y);
#ifdef I_AM_OS_2
  FntSetFont(stdFont);
#else
  FntSetFont(stdFont);
#endif
}

// call alloc_message_log initially when all stuff is being allocated..
// it has to be dynamic so we can play games with pointers in log_message.
void alloc_message_log()
{
  Short i;
  for (i = 0 ; i < SAVED_MSGS ; i++) {
    old_messages[i] = (Char *) md_malloc(sizeof(Char) * SAVED_MSG_LEN);
    old_messages[i][0] = '\0';
  }
  // the log is all null strings.. so (we pretend) all of it has been shown
  last_old_message_shown = SAVED_MSGS - 1;
}
void clear_message_log()
{
  Short i;
  for (i = 0 ; i < SAVED_MSGS ; i++)
    old_messages[i][0] = '\0';
  // the log is all null strings.. so (we pretend) all of it has been shown
  last_old_message_shown = SAVED_MSGS - 1;
}
























UChar stats_x[13] = {0,0,0,0,0,0,0,0,0,0,0,0,0};
#define NUM_HUNGER_STR 7
Char hunger_str[NUM_HUNGER_STR][9] = {
  "satiated", " ", "hungry", "weak", "fainting", "fainted", "starved"
};
static void print_stats_where()
{
  /*
  stats_x[0] = FntCharsWidth("L", 1);
  stats_x[1] = FntCharsWidth("99 ", 2+1);
  stats_x[2] = FntCharsWidth("t", 1); // FntCharsWidth("$", 1);
  stats_x[3] = FntCharsWidth("1234567890 ", 10+1);
  stats_x[4] = FntCharsWidth("Hp", 2);
  stats_x[5] = FntCharsWidth("999(999) ", 8+1);
  // then 'hunger' - that is the last thing on the first line.
  stats_x[6] = FntCharsWidth("Ac ", 3);
  stats_x[7] = FntCharsWidth("-20 ", 3+1);
  stats_x[8] = FntCharsWidth("Str ", 4);
  stats_x[9] = FntCharsWidth("18/00 ", 5+1);
  stats_x[10] = FntCharsWidth("Exp ", 4);
  // then %2d/%10d - that is the last thing on the second line.
  */
  Short i;
  stats_x[0] = FntCharsWidth("L ", 2);
  stats_x[1] = FntCharsWidth("99 ", 2+1);
  stats_x[2] = FntCharsWidth("Hp ", 3);
  stats_x[3] = FntCharsWidth("999(999) ", 8+1);
  stats_x[4] = FntCharsWidth("Ac ", 3);
  stats_x[5] = FntCharsWidth("-20 ", 3+1);
  stats_x[6] = FntCharsWidth("Str ", 4);
  stats_x[7] = FntCharsWidth("18/00 ", 5+1);
  // End of the first line.
  //
  stats_x[8] = FntCharsWidth("Exp ", 4);
  stats_x[9] = FntCharsWidth("14/409610 ", 2+1+6+1);
  // Max level is 14, this takes 40961 points to achieve.  You can still
  //    get more experience but you don't get to a higher level.
  //    Maybe six digits will be enough for experience points?
  stats_x[10] = 0; // figure out the longest hunger-string
  for (i = 0 ; i < NUM_HUNGER_STR ; i++) {
    Short len = FntCharsWidth(hunger_str[i], StrLen(hunger_str[i]));
    if (len > stats_x[10]) stats_x[10] = len;
  }
  stats_x[10] += FntCharWidth(' ');
  //  stats_x[11] = FntCharsWidth("t1234567890", 10+1);
  stats_x[11] = FntCharsWidth("1234567890", 10);
}


typedef struct {
  Short dlevel;
  Long gold;
  Short hpcur;
  Short hpmax;
  UChar hunger;
  Short ac;
  Short str;
  Short plevel;
  Long exp;
} test_stats;
test_stats oldstats = {
  0, 0L, 0, 0, 0, 0, 0, 0
};

#define LineHeight 11
void print_stats(UInt which_stats)
{
  Char buf[40];
  RectangleType r;
  Short x=0, y = StatsTopY, k = 0;

  // Level, Gold, Hp, Ac, Str, Exp, [hunger], [moves].
  // dlevel, u.ugold, u.uhp, u.uhpmax, u.uac, u.ustr, u.ulevel, u.uexp, ...
  // see hack103/pri.c

  qWinDrawLine(0,y-1,SCR_WIDTH,y-1);
  if (stats_x[0] == 0) print_stats_where();

  if (flags.botl == BOTL_ALL) {
    // I should really only do these if we need to redraw all stats.
    RctSetRectangle(&r, 0, y, SCR_WIDTH, LineHeight);
    qWinEraseRectangle(&r, 0);
    x = 0; 
    k = 0;
    qWinDrawChars("L", 1, x, y);
    x += stats_x[k++]; // 0
    x += stats_x[k++]; // 1
    qWinDrawChars("Hp", 2, x, y);
    x += stats_x[k++]; // 2
    x += stats_x[k++]; // 3
    qWinDrawChars("Ac", 2, x, y);
    x += stats_x[k++]; // 4
    x += stats_x[k++]; // 5
    qWinDrawChars("Str", 3, x, y);
    k += 2; // skip 6 and 7
    x = 0;
    y += LineHeight;
    RctSetRectangle(&r, 0, y, SCR_WIDTH, LineHeight);
    qWinEraseRectangle(&r, 0);
    qWinDrawChars("Exp", 3, x, y);
    x += stats_x[k++]; // 8
    x += stats_x[k++]; // 9
    // (skip 10 and 11)
  }
  x = 0;
  y = StatsTopY;
  k = 0;

  x += stats_x[k++]; // 0
  if (flags.botl & BOTL_DLEVEL) {
    RctSetRectangle(&r, x,y, stats_x[k], LineHeight); qWinEraseRectangle(&r,0);
    StrPrintF(buf, "%d", dlevel); // dungeon level:  dlevel.  Max of 40.
    qWinDrawChars(buf, StrLen(buf), x, y);
  }
  x += stats_x[k++]; // 1
  x += stats_x[k++]; // 2
  if (flags.botl & BOTL_HP) {
    RctSetRectangle(&r, x,y, stats_x[k], LineHeight); qWinEraseRectangle(&r,0);
    StrPrintF(buf, "%d(%d)", you.uhp, you.uhpmax); // hp. guessing max is 999??
    qWinDrawChars(buf, StrLen(buf), x, y);
  }
  x += stats_x[k++]; // 3
  x += stats_x[k++]; // 4
  if (flags.botl & BOTL_AC) {
    RctSetRectangle(&r, x,y, stats_x[k], LineHeight); qWinEraseRectangle(&r,0);
    StrPrintF(buf, "%d", you.uac); // Don't know if this is limited..
    qWinDrawChars(buf, StrLen(buf), x, y);
  }
  x += stats_x[k++]; // 5
  x += stats_x[k++]; // 6
  if (flags.botl & BOTL_STR) {
    RctSetRectangle(&r, x,y, stats_x[k], LineHeight); qWinEraseRectangle(&r,0);
    if (you.ustr <= 18)
      StrPrintF(buf, "%d", you.ustr); // 18, 18/(01-99), 18/00 ...
    else
      StrPrintF(buf, "%d%s%d", 18, 
		((you.ustr < 18+10) || (you.ustr > 18+99)) ? "/0" : "/",
		(you.ustr > 18+99) ? 0 : you.ustr-18 );
    qWinDrawChars(buf, StrLen(buf), x, y);
  }
  k++; // skip 7
  x = 0;
  y += LineHeight;
  x += stats_x[k++]; // 8
  if (flags.botl & BOTL_EXP) {
    RctSetRectangle(&r, x,y, stats_x[k], LineHeight); qWinEraseRectangle(&r,0);
    StrPrintF(buf, "%d/%ld", you.ulevel, you.uexp);//don't know if uexp limited
    qWinDrawChars(buf, StrLen(buf), x, y);
  }
  x += stats_x[k++]; // 9
  if (flags.botl & BOTL_HUNGER) {
    RctSetRectangle(&r, x,y, stats_x[k], LineHeight); qWinEraseRectangle(&r,0);
    StrPrintF(buf, "%s", hunger_str[you.uhs]);
    qWinDrawChars(buf, StrLen(buf), x, y);
  }
  x += stats_x[k++]; // 10
  if (true) {
    RctSetRectangle(&r, x,y, stats_x[k], LineHeight); qWinEraseRectangle(&r,0);
    if (moves >= 1000000000L) {
      StrPrintF(buf, "%ld", moves); //StrPrintF(buf, "t%ld", moves);
      qWinDrawChars(buf, StrLen(buf), x, y);
      x += stats_x[k++]; // 11
    } else {
      StrPrintF(buf, "%ld", moves); //StrPrintF(buf, "t %ld", moves);
      x += stats_x[k++]; // 11
      qWinDrawChars(buf, StrLen(buf), x - FntCharsWidth(buf, StrLen(buf)), y);
    }
  }

  /*
  oldstats.dlevel = dlevel;
  oldstats.gold = you.ugold;
  oldstats.hpcur = you.uhp;
  oldstats.hpmax = you.uhpmax;
  oldstats.ac = you.uac;
  oldstats.str = you.ustr;
  oldstats.plevel = you.ulevel;
  oldstats.exp = you.uexp;
  */
}
