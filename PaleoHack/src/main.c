/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
//#include <Graffiti.h> /* just for GrfGetState */
//#include <CharAttr.h> /* for IsDigit */ // why is this not working for 3.1??
#ifndef I_AM_COLOR
#include <System/Globals.h> // for font foo
#else /* I_AM_COLOR */
// nothing needed, apparently
#endif /* I_AM_COLOR */

#include "paleohackRsc.h"
//#include "lock-externs.h"

DmOpenRef phDB = NULL;
DmOpenRef phSaveDB = NULL;
DmOpenRef phBonesDB = NULL;
DmOpenRef phScoreDB = NULL;

Boolean Version_GE_OS35 = false; // (is OS version >= 3.5)
Boolean death = false;
extern Short multi; // living in movesee.c right now..


void move_visible_window(Short left_x, Short top_y, Boolean center);//display.c


extern Boolean came_from_generation;

static Boolean convert_to_dir(Short x, Short y, Short ignore_center);
static Boolean convert_char_to_dir(Char d);
static Boolean do_command(Char com_val);
static Boolean do_inv_command(Char com_val);
static void do_multi_move();
static Boolean do_dir_command();
static Boolean do_xy_command(Short x, Short y);
static Boolean buttonsHandleEvent(EventPtr e);
static Boolean OpenDatabase(void);
static Boolean ApplicationHandleEvent(EventPtr e);
static void readPrefs();
static Word StartApplication(void);
static void StopApplication(void);
static void EventLoop(void);

HackPreferenceType my_prefs = {
  6, // run_walk_border
  2, // walk_center_border
  { 0, HWB_W, HWB_E, 0, HWB_N, HWB_S, HWB_MAP, HWB_SEARCH },
  true, //  false, // hardware buttons XXX just for testing.
  "",
  true,  // is male
  true,  // big font
  false, // relative move (off)
  true,  // sound
  true,  // run
  //  true,  // auto pickup
  false, // invert (not) on
  false, // color (not) on
  false  // (don't) turn off animations
};


extern Short msglog_mode;
previous_state prev_state;
previous_state curr_state;
static void reset_state(previous_state *ps)
{
  ps->mode = MODE_DEFAULT;
  ps->cmd = -1;
  //  ps->dir_cmd = DIR_NONE;
  //  ps->dir = 5;
  ps->item = NULL;
  ps->mon = NULL;
  ps->count = 0;
  ps->spell = -1;
}
void bump_state()
{
  previous_state *ps = &curr_state;
  ps->mode = MODE_AGAIN;
  if (curr_state.cmd != -1)
    prev_state = curr_state;
  // reset_state:
  ps->mode = MODE_DEFAULT;
  ps->cmd = -1;
  //  ps->dir_cmd = DIR_NONE;
  //  ps->dir = 5;
  ps->item = NULL;
  ps->mon = NULL;
  ps->count = 0;
  ps->spell = -1;
}


//#define abs(a)                                  (((a) >= 0) ? (a) : (-(a)))
// Return false if the direction is "no move"
static Boolean convert_to_dir(Short x, Short y, Short ignore_center)
{
  you.dx = you.dy = 0;
  you.dz = 0; // ???
  if (x*x + y*y < ignore_center*ignore_center) return false;

  if ((x > 0) && (12*(abs(x)) > 5*(abs(y)))) you.dx = 1;  // ne, e, se
  if ((x < 0) && (12*(abs(x)) > 5*(abs(y)))) you.dx = -1; // nw, w, sw
  if ((y > 0) && (12*(abs(y)) > 5*(abs(x)))) you.dy = 1;  // se, s, sw
  if ((y < 0) && (12*(abs(y)) > 5*(abs(x)))) you.dy = -1; // ne, n, nw
  return (you.dx || you.dy);
}
static const Char map_arrows[4] = { 'h','l','k','j' };
static void convert_arrow_keys(Char *comval)
{
  // map the arrow keys for keyboards (e.g. stowaway, gotype)
  Char c = *comval;
  if (c >= leftArrowChr && c <= downArrowChr) {
    Boolean capslock_on, numlock_on, autoshifted;
    Short tmp_shift;
    c = map_arrows[c - leftArrowChr]; // convert to one of "hlkj"!
    GrfGetState(&capslock_on, &numlock_on, &tmp_shift, &autoshifted);
    if ((tmp_shift == grfTempShiftUpper) || capslock_on)
      c -= 'a'-'A';
    *comval = c;
  }
}
static Boolean convert_char_to_dir(Char d)
{
  you.dx = you.dy = 0;
  you.dz = 0; // ???
  convert_arrow_keys(&d);
  if (d < 'a' && d >= 'A') d += 'a'-'A';
  switch(d) {
  case 'h': you.dx--; return true;
  case 'j': you.dy++; return true;
  case 'k': you.dy--; return true;
  case 'l': you.dx++; return true;
  case 'y': you.dx--; you.dy--; return true;
  case 'u': you.dx++; you.dy--; return true;
  case 'b': you.dx--; you.dy++; return true;
  case 'n': you.dx++; you.dy++; return true;
  case '<': you.dz--; return true;
  case '>': you.dz++; return true;
  }
  return false;
}

Boolean took_time;
static void wail()
{
  static Long wailmsg = 0;
  if (moves - wailmsg <= 50) return;
  wailmsg = moves;
  message( (you.uhp == 1) ?
	   "You hear the wailing of the Banshee..." :
	   "You hear the howling of the CwnAnnwn..." );
}
static void regen()
{
  if (you.ulevel > 9) {
    if (Regeneration || !(moves % 3)) {
      flags.botl |= BOTL_HP;
      you.uhp += rnd((Int) you.ulevel-9);
      if (you.uhp > you.uhpmax) you.uhp = you.uhpmax;
    }
  } else if (Regeneration || (!(moves % (22-you.ulevel*2)))) {
    flags.botl |= BOTL_HP;
    you.uhp++;
  }
}
Char *nomovemsg = NULL;
void (*afternmv)();
void tick()
{
  if (you.dead) return;
  // This should make one turn pass.  Right now it is a dummy.
  set_track(); // Hey!  In init, after we do moonphase, also need initrack()!
  if ((moves % 2 == 0) || (!(Fast & ~INTRINSIC) && (!Fast || rund(3)))) {
    movemon();
    if (!rund(70))  makemon(NULL, 0, 0);
  } // else you get an 'extra' turn, you lucky thing.
  if (Glib)  slippery_fingers();
  timeout(); // tick any effects that are on timers.
  moves++;
  //  if (flags.time) flags.botl = 1; // Just to print "%ld" moves in statusbar
  if (you.uhp < 1 && !you.dead) {
    message("You die...");
    done("died"); // XXX 
    return;
  }
  if (you.uhp*10 < you.uhpmax)  wail();
  if (you.uhp < you.uhpmax)  regen();
  if (Teleportation && !rund(85))  tele();
  if (Searching && multi >= 0)  do_search();
  gethungry();
  invault();
  amulet();
  /////////////////////////////////////////////////////
  // Above are the things that happen iff "flags.move".
  /////////////////////////////////////////////////////
  /*
  if (multi < 0) {
    if (!++multi) {      // XXXX I'm not ready for afternmv yet... commented:
      message(nomovemsg ? nomovemsg : "You can move again.");
      nomovemsg = NULL;
      if (afternmv) (*afternmv)(); // unfaint(), Meatdone(), stealarm().
      afternmv = NULL;
    }
  }
  */
  find_ac();
  if (!flags.mv || Blind) {
    seeobjs(); // really should be called decay_corpses
    seemons();
    nscr();// XXXXX isn't this redundant?  since I'm calling refresh in tock...
    // on the other hand, after adding nscr, the punishment ball draws properly
  }
  //  if (flags.botl || flags.botlx) bot(); // bot==print_stats...
  flags.move = 1;
  // Here is the point where we would check for monsters to stop 'occupation'.
  // Followed by the point where we would do repeated things: "if (multi > 0)".
  // "rhack" is the "(read in and) dispatch a command"
}
void tock()
{
  if (you.dead) return;
  show_messages();
  //  if (flags.botl) {
  print_stats(0);
  flags.botl = BOTL_NONE;
  //  }
  refresh();
}

// Caller needs to avoid taking a turn after we return..........
void spin_multi(Char *msg)
{
  if (multi < 0) {
    do {
      tick();
      if (you.dead) { multi = 0; return; }
    } while (++multi < 0);
    if (!msg)
      message("You can move again.");
    else if (msg[0])
      message(msg);
    took_time = false; // xxxx
  }
}

extern Boolean draw_directional_p;
extern Boolean undraw_directional_p;

void end_turn_start_turn()
{
  if (you.dead) return;
  if (undraw_directional_p) undraw_directional();

  if (multi < 0) spin_multi(NULL); // XXX
  if (took_time) {
    bump_state();
    flags.nopick = 0;
    tick(); // "end_of_turn(); start_of_turn();"
  }
  tock();

  //  if (you.dead) {    LeaveForm();    FrmGotoForm(DeathForm);  }
  if (curr_state.mode == MODE_DEFAULT && message_clear(false))
    curr_state.mode = MODE_MORE;

  if (draw_directional_p) draw_directional();
}

//void test_findname();
void draw_tombstone();
Boolean Main_Form_HandleEvent(EventPtr e)
{
  Boolean handled = false, valid = false;
  Short tmp_x, tmp_y;
  Char c;
  FormPtr frm;

  switch (e->eType) {

  case frmOpenEvent:
    reset_state(&prev_state);
    reset_state(&curr_state);
    frm = FrmGetActiveForm();
    FrmDrawForm(frm);
    //    clear_visible();
    if (came_from_generation) {
      init_player();
    } else {
      message("restoring...");
      greet_player();
    }
    came_from_generation = false;
    moon_player();
    init_track();
    check_rogue_position(true); // center screen at rogue
    refresh();
    print_stats(0);
    show_messages();
    /*
      {
      Char buf[10];
      Short s = sizeof(monst_t);
      StrPrintF(buf, "ptr %d", s);
      WinDrawChars(buf, StrLen(buf), 30, 30);
      }
    */
    return true;
    // end frmOpenEvent
  default: break;
  }
  // end FIRST switch e->eType

  // Other modes will have different handlers
  switch(curr_state.mode) {
  case MODE_MORE:
    // Go into this mode when message_clear(false) returns true.
    // right now, we can only enter it from end_turn_start_turn..
    switch (e->eType) {
    case keyDownEvent: case penDownEvent:
      show_messages();
      if (!message_clear(false))
	curr_state.mode = MODE_DEFAULT;
      return true;
    default:
      return handled;
    }
    break; // end MODE_MORE

  case MODE_GETCELL:
    switch (e->eType) {
    case penDownEvent:
      where_in_dungeon(e->screenX, e->screenY, &tmp_x, &tmp_y);//cnvert to cell
      valid = do_xy_command(tmp_x, tmp_y);
      break;
    default:
      return handled;
    }
    if (valid) {
      monst_t *mtmp = curr_state.mon;
      bump_state();  // Can't: we need the curr_state.mon ...
      curr_state.mon = mtmp;
    } else reset_state(&curr_state);
    handled = true;
    break; // end MODE_GETCELL

  case MODE_DIRECTIONAL:
    // Can be entered only from do_command: iff one of "f^cSod?-" was entered.
    switch (e->eType) {
    case keyDownEvent:
      c = e->data.keyDown.chr;
      if ((valid = convert_char_to_dir(c)))
      // I should set fields of curr_state to represent you.dx, you.dy...
	valid = do_dir_command();
      break;
    case penDownEvent:
      tmp_x = e->screenX - 80;
      tmp_y = e->screenY - 80;
      // I guess I better let do_dir_command decide whether a
      // noop direction gets a free_turn_flag=true or what.
      convert_to_dir(tmp_x, tmp_y, 10);
      // I should set fields of curr_state to represent you.dx, you.dy...
      valid = do_dir_command();
      break;
    default:
      return handled;
    }
    if (valid) bump_state();
    else reset_state(&curr_state);
    handled = true;
    break; // end MODE_DIRECTIONAL

  case MODE_DEFAULT:
    // Mode gets set back to this every time we end a time-taking command.
    switch (e->eType) {
    case menuEvent:
      MenuEraseStatus(NULL);
      switch(e->data.menu.itemID) {
      case menu_mainAbout: FrmPopupForm(AboutForm);   return true;
      case menu_mainMap:   FrmPopupForm(MapForm);     return true;
      case menu_mainMsgs:
	msglog_mode = SHOW_MSGLOG;
	FrmPopupForm(MsgLogForm);
	return true;
      case menu_mainHelp:
	if (0 == FrmAlert(LongShortP))
	  msglog_mode = SHOW_LHELP;
	else
	  msglog_mode = SHOW_SHELP;
	FrmPopupForm(MsgLogForm);
	return true;
      case menu_cmd_i:     FrmPopupForm(InvForm);     return true;
      case menu_mainQuit:
	// need:
	// use a FrmAlertForm to confirm: "Really quit?".  then call,
	done("quit");
	// and then goto the TOPTEN form.
	//	//	FrmGotoForm(Chargen1Form);
	return true;
      case menu_mainFont:
	toggle_itsy();
	move_visible_window(you.ux, you.uy, true);
	refresh();
	return true;
      case menu_mainWiz1:
	{
	  extern Short engrave_or_what;
	  engrave_or_what = GET_WISH;
	  FrmPopupForm(EngraveForm);
	  flags.beginner = false; // xxx for testing
	  return true;
	}
      case menu_mainRedraw:
	// xxx
	savelev(dlevel, true);
	getlev(dlevel, true);
	move_visible_window(you.ux, you.uy, true);
	refresh();
	return true;
      case menu_mainTest:
	//FrmCustomAlert(PickUpThisP, "fnord", NULL, NULL);
	//      message("toggling ask-before-pickup");
	//      flags.askpick = !flags.askpick;
	message("making all Known");
	{
	  Short i;
	  for (i = 0 ; i < MAX_OC_NK ; i++)
	    oc_name_known[i] = 0xff;
	}
	return true;
	/*
	test_findname();
	show_messages();
	return true;
	*/
      default:
	{
	  Short cmd;
	  // MAGIC & MORE MAGIC..
	  cmd = e->data.menu.itemID - MAGIC_MENU_NUMBER;
	  if (cmd > 0 && cmd < 128)
	    handled = do_command((Char) cmd);
	}
	curr_state.count = /*command_count = */0;
	break;
      }
      handled = true;
      break;
      // end menuEvent

    case keyDownEvent:
      if (message_clear(false)) {
	show_messages();
	//    command_count = 0; // I do need this - it just doesn't exist yet
	return true;
      }
      c = e->data.keyDown.chr;
      handled = do_command(c); // XXXX This needs fixed!
      //    if (handled && !free_turn_flag) command_count = 0; // XXX
      //    else { ... } // Just swipe all that from kMoria, I wrote it anyway.
      if (!handled)
	if (do_inv_command(c))
	  // will pop up a form!
	  return true;
      break;

    case penDownEvent:
      if (message_clear(false)) {
	show_messages();
	// command_count = 0; // I do need this - it just doesn't exist yet
	return true;
      }
      tmp_x = e->screenX - 80;
      tmp_y = e->screenY - 80;
      if (!convert_to_dir(tmp_x, tmp_y, 10)) {
	took_time = false;
	handled = true;
	break;
      }
      {
	Short tmp_dist;
	tmp_dist = tmp_x * tmp_x + tmp_y * tmp_y;
	//      if (tmp_dist < my_prefs.run_walk_border * 10
	//	  * my_prefs.run_walk_border * 10) {
	if (tmp_dist < 6 * 10 * 6 * 10 && !flags.run) {
	  took_time = do_move();
	  check_rogue_position(false);
	} else 
	  do_multi_move();
      }
      // also might want a 'command_count = 0;'
      handled = true;
      break;
      // end penDownEvent
    default:
      return handled;
    } // end switch(e->eType)
  } // end my_mode switch

  // for anyone who has not returned,
  // call something to move the monsters.
  end_turn_start_turn();
  return handled;
}

/*****************************************************************************/
static Boolean do_command(Char com_val)
{
  Boolean handled = true;
  //  Short dir;
  took_time = true; // by default
  // map the deeply irritating arrow keys for stowaway keyboard.
  // I particularly distrust and despise the shift thing.
  convert_arrow_keys(&com_val);
  convert_char_to_dir(com_val);
  switch(com_val) {
  case 'm':
    flags.run = 0;
    flags.nopick = 1;
    took_time = false;
    break;
  case 'M':
    flags.run = 1;
    flags.nopick = 1;
    took_time = false;
    break;
  case 's':
    do_search(); // need to make this take a count_command..
    break;
  case 'h': case 'j': case 'k': case 'l':
  case 'y': case 'u': case 'b': case 'n':
    // Hey.  If m: flags.run = 1 and flags.nopick = 1;
    // also, don't forget about possibility of command_count
    if (flags.run) do_multi_move();
    else {
      took_time = do_move(); // XXX sometimes do_move crashes.. reading from unalocated chunk of memory... basically, when you fall through a trap door before it draws the next level.
      check_rogue_position(false);
    }
    break;
  case 'H': case 'J': case 'K': case 'L':
  case 'Y': case 'U': case 'B': case 'N':
    do_multi_move();
    break;
  case 'p':
    took_time = dopay(); // pay shopkeeper; takes no time if none is present.
    break;
  case '$':
    took_time = doprgold(); // count your money.  can take multiple turns..
    break;
  case 'T':
    if (getobj_init("[", "take off", ACT_AOFF))
      FrmPopupForm(InvActionForm);
    else {
      message("Not wearing any armor.");
      took_time = false;
    }
    break;
  case ')':
    message("list currently wielded weapon...");
    took_time = false;
    break;
  case '[':
    message("list currently worn armor...");
    took_time = false;
    break;
  case '=':
    message("list currently put-on rings...");
    took_time = false;
    break;
  case ':':
    took_time = do_look(); // list the things you are standing on.  needs work.
    break;
  case '/':
    // "whatsit" command.. puts main event handler into MODE_GETCELL,
    // you need to write a graffiti character next.
    // (I think I should allow screen taps too.)
    // and you get a message saying what that class of objs/mons is.
    took_time = false;
    break;
  case '\\':
    // "list known items" command
    msglog_mode = SHOW_DISCOVERED;
    FrmPopupForm(MsgLogForm);
    took_time = false;
    break;
  case '.':
    message("resting");
    break;
  case ',':
    // oh man, I know people are going to ask for button/key bindings here..
    // (it pops up a FrmCustomAlert form.)
    took_time = do_pickup();
    break;
  case '<':
    took_time = do_up();
    break;
  case '>':
    took_time = do_down();
    break;
  case 'C':
    took_time = false;
    curr_state.cmd = 'C';
    curr_state.item = NULL;
    curr_state.mode = MODE_GETCELL;
    message("Select a monster to name...");
    break; // name an individual monster.
    // allows you to move cursor to a monster to give it a name!
    // gar... not sure how to do that... will be like "tap to recall"
    // in moria: basically I need a MODE_NAMEMON for main event handler.
  case '\024': // ^T "teleport".  octal 24, decimal 20.
  case 'Z': /* I'm not sure that ^T will work, even with a keyboard.
	       ^T is ok as a menu item but keypress doesn't work in POSE.
	       So I may have to add Z as an alternative for graffitios.  */
    took_time = dotele();
    return true;
  default:
    handled = false;
    took_time = false;
    break;
  }
  return handled;
}

static Boolean do_inv_command(Char com_val)
{
  Boolean tmp, handled = true;
  took_time = false; // by default, we're popping up a form.
  // if we return true, the caller will also return immediately.
  //
  // we pop up a (maybe filtered/scrolled) inventory form
  // with one verb button (\n) and one cancel buttom (ESC)
  //
  // some of these are no-op,
  // if there is nothing reasonable in yor inventory for that verb,
  // or if you choose to cancel it or whatever,
  // 
  // if we need to take a turn HERE, instead of
  // in an inventory-form event handler, call it explicitly here.
  switch(com_val) {
    // Take a long hard look at getobj, for the filtering.
  case '?':
    if (0 == FrmAlert(LongShortP))
      msglog_mode = SHOW_LHELP;
    else
      msglog_mode = SHOW_SHELP;
    FrmPopupForm(MsgLogForm);
    break;
  case 'a':
    if (getobj_init("(", "use or apply", ACT_APPLY))
      FrmPopupForm(InvActionForm);
    break;
  case 'c':
    // it SHOULD first ask you whether you want to name an individual object
    // (yes = 'call' or no = 'name'?)
    if (0 == FrmAlert(NameCallP))
      tmp = getobj_init("#", "name", ACT_NAME);
    else
      tmp = getobj_init("?!=/", "call", ACT_CALL);
    if (tmp) FrmPopupForm(InvActionForm);
    break;
  case 'd':
    if (getobj_init("0$#", "drop", ACT_DROP))
      FrmPopupForm(InvActionForm);
    break;
  case 'D':
    {
      extern Boolean drop_not_identify;
      drop_not_identify = true;
      FrmPopupForm(ObjTypeForm);
    }
    break;
    // drop by category... takes a single turn?
    // ok, basically have a variation on the 'drop' form:
    // one button to drop, one button to exit (like cancel), but
    // the drop button does NOT make you LeaveForm nor take a turn,
    // instead the cancel button DOES take a turn IFF you dropped anything.
    // if you drop nothing, it will not take a turn.
    // - will people miss the ability to filter by category?  maybe..
    //   (would need pushbuttons to select categories, plus an ALL button).
  case 'e':
    took_time = eat_off_floor();
    if (took_time)
      end_turn_start_turn();
    else if (getobj_init("%", "eat", ACT_EAT))
      FrmPopupForm(InvActionForm);
    break;
  case 'q':
    if (getobj_init("!", "drink", ACT_QUAFF))
      FrmPopupForm(InvActionForm);
    break;
  case '!':
    // later, will need ("!", "dip into") also.
    if (getobj_init("#", "dip", ACT_DIP))
      FrmPopupForm(InvActionForm);
    break;
  case 'r': 
    if (getobj_init("?", "read", ACT_READ))
      FrmPopupForm(InvActionForm);
    break;
  case 't':
    if (getobj_init("#", "throw", ACT_THROW))
      FrmPopupForm(InvActionForm);
    break;
  case 'w':
    if (getobj_init("#-)", "wield", ACT_WIELD))
      FrmPopupForm(InvActionForm);
    break;
  case 'E':
    // This will need to pop up the get-a-string Engrave form afterwards,
    // if the action is not cancelled.
    if (getobj_init("#-)", "write with", ACT_ENGRAVE))
      FrmPopupForm(InvActionForm);
    break;
  case 'z':
    if (getobj_init("/", "zap", ACT_ZAP))
      FrmPopupForm(InvActionForm);
    break;
  case 'P':
    if (getobj_init("=", "wear", ACT_RWEAR))
      FrmPopupForm(InvActionForm);
    break;
  case 'R': break; // remove ring - this should simply ask "right or left"
    // (*I* think it should also tell you which ring is on which..)
  case 'W':
    if (getobj_init("[", "wear", ACT_AWEAR))
      FrmPopupForm(InvActionForm);
    break;
  case 'i':
    FrmPopupForm(InvForm);
    return true;
  case '\015': // ^M "map".  octal 15, decimal 13.
    FrmPopupForm(MapForm);
    return true;
  case '^': // identify an adjacent trap in some direction,
    curr_state.cmd = '^';
    curr_state.item = NULL;
    curr_state.mode = MODE_DIRECTIONAL;
    draw_directional();
    // do not 'tick'.
    // also, call "do_id_trap" when you're done.
    break;
  case '@':  // XXXX DEBUGGING TAKE THIS OUT.
    {
      extern Short engrave_or_what;
      engrave_or_what = GET_WISH;
      FrmPopupForm(EngraveForm);
      flags.beginner = false; // xxx for testing
      return true;
    }
  default:
    handled = false;
    took_time = false;
    break;
  }
  return handled;
}


static void do_multi_move()
{
  // If capital letter: flags.run = 1;
  // If f+direction: flags.run = 2;
  // If F+direction: flags.run = 3;
  // If m: flags.run = 1 and flags.nopick = 1;
  // Anyway, we need a nice little do_move() loop somewhere.
  // It is implicit in the dungeon loop in hack but we don't have that loop.
  // sigh.
  // 'multi' is a flag that says "what we're doing will take multiple turns".
  // it's used for stuff besides moving, too.  maybe should be command_count!
  multi = MAX_RUN;
  if (!flags.run) flags.run = 3;//1;
  do {
    took_time = do_move(); // might call nomul.
    check_rogue_position(false);
    // NOTE: lookaround ALSO changes direction when the corridor turns!
    lookaround(); // might call nomul.
    // Also, call whatever makes time pass!!!!
    if (took_time) tick();
    // hm, watch out, could an attack change multi to negative?
    // is that what flags.mv is for or something else.
  } while (--multi > 0 /*&& flags.mv*/);
  flags.mv = flags.run = multi = 0;
  took_time = false; // cause we already called tick
}


// to do continuations of directional commmands.
// return true if the command was valid/meaningful.
static Boolean do_dir_command()
{
  if (undraw_directional_p) undraw_directional();

  if (curr_state.cmd == -1) return false;
  switch(curr_state.cmd) {
    // If you want to do up/down, you will have to use graffiti, currently.
  case 'C': // Camera
    took_time = false;
    if (you.dx || you.dy || you.dz) {
      use_camera(curr_state.item);
      took_time = true;
    } else {
      multi = 0; // flags.move = 0 also?
      return false;
    }
    break;
  case 'P': // Pickaxe
    took_time = false;
    if (you.dx || you.dy || you.dz) {
      took_time = use_pick_axe(curr_state.item);
    } else {
      return false;
    }
    break;
  case '^': // identify trap, adjacent in given direction.  If any.
    took_time = false;// id trap should never take time!
    do_id_trap();
    // Can you id a trap you're standing on?  Yes if you hit '>'.
    break;
  case 't': // Throw
    took_time = false;
    if (you.dx || you.dy || you.dz)
      took_time = do_throw(curr_state.item);
    else
      return false;
    break;
  case 'z': // Zap
    took_time = false;
    if (you.dx || you.dy || you.dz)
      took_time = do_zap_helper(curr_state.item);
    else
      return false;
    break;
  default:
    break;
  }
  return true;

}

// to do continuations of get-a-cell commmands.
// return true if the command was valid/meaningful.
extern Char plname[PL_NSIZ];
extern Short engrave_or_what;
static Boolean do_xy_command(Short x, Short y)
{
  monst_t *mtmp;
  if (curr_state.cmd == -1) return false;
  if (OUT_OF_BOUNDS(x, y)) return false;
  switch(curr_state.cmd) {
  case 'C': // 'call monster'
    took_time = true; // yes, screwing up takes a turn.  bummer eh.
    mtmp = mon_at(x, y);
    if (!mtmp) {
      if (x == you.ux && y == you.uy) {
	StrPrintF(ScratchBuffer,
		  "This ugly monster is called %s and cannot be renamed.",
		  plname);
	message(ScratchBuffer);
      } else                            message("There is no monster there.");
    }
    else if (mtmp->bitflags & M_IS_MIMIC)  message("I see no monster there.");
    else if (!cansee(x, y))          message("I cannot see a monster there.");
    else {
      took_time = false;
      engrave_or_what = ACT_CHRISTEN;
      curr_state.mon = mtmp;
      FrmPopupForm(EngraveForm);
    }
    break;
  case '\024': // ^T "teleport".  octal 24, decimal 20.
    took_time = true;
    tele_finish(x, y, Teleport_control);
    break;
  case '/': // 'whatsit'
    break;
  default:
    break;
  }
  return true;
}

/*****************************************************************************/

const Char hwb_commands[HWB_last+2] = "\0kjlh\015stiag!#";//last2: font, shift.
static Boolean buttonsHandleEvent(EventPtr e)
{
  Boolean handled = false, valid = false, capslock_on, numlock_on, autoshifted;
  Word curfrm;
  Short btn, dispatch_type = 0, tmp_shift;
  Char command;

  // block app-switching in msglogform
  if ( (e->data.keyDown.chr == launchChr ||
	e->data.keyDown.chr == hardPowerChr) &&
       FrmGetActiveFormID() == MsgLogForm)
    return true;

  if ( ((e->data.keyDown.chr < hard1Chr) || (e->data.keyDown.chr > hard4Chr))
       && (e->data.keyDown.chr != calcChr)
       && (e->data.keyDown.chr != findChr)
       && (e->data.keyDown.chr != pageUpChr)
       && (e->data.keyDown.chr != pageDownChr))
    return false; // it's NOT a hardware button.. probably graffiti.

  curfrm = FrmGetActiveFormID();

  switch (e->data.keyDown.chr) {
    // this just maps it from wacky-button-char to '0...7'.
  case hard1Chr:     btn = HW_hard1Chr   ;  break;      // datebook
  case hard2Chr:     btn = HW_hard2Chr   ;  break;      // address
  case hard3Chr:     btn = HW_hard3Chr   ;  break;      // todo
  case hard4Chr:     btn = HW_hard4Chr   ;  break;      // memos
  case pageUpChr:    btn = HW_pageUpChr  ;  break;
  case pageDownChr:  btn = HW_pageDownChr;  break;
  case calcChr:      btn = HW_calcChr    ;  break;
  case findChr:      btn = HW_findChr    ;  break;
  default:
    return false;
  }

  // [ here I should do some per-form button handling, possibly ]

  dispatch_type = my_prefs.hardware[btn]; // XXX
  // tapping the 'goto map' button a second time will EXIT the map.. rah.
  if ((curfrm == MapForm) && (dispatch_type == HWB_MAP)) {
    LeaveForm();
    return true;
  }

  if (curfrm != MainForm) {
    // IF the key is bound, return TRUE to MASK it, unless it is up/down.
    return ( (dispatch_type != HWB_NOOP ||
	      curfrm == MsgLogForm) &&
	     e->data.keyDown.chr != pageUpChr &&
	     e->data.keyDown.chr != pageDownChr );
  }

  if (dispatch_type < HWB_first || dispatch_type > HWB_last) return false;
  command = hwb_commands[dispatch_type];
  if (dispatch_type >= 1 && dispatch_type <= 4) { // "kjlh"
    // This part is just like map_roguedir...
    GrfGetState(&capslock_on, &numlock_on, &tmp_shift, &autoshifted);
    if ((tmp_shift == grfTempShiftUpper) || capslock_on)
      command -= 'a' - 'A';
    // ... except we need to "turn off" the temp shift afterwards (if any).
    GrfSetState(capslock_on, numlock_on, false);
  }

  switch(curr_state.mode) {
  case MODE_MORE:
    show_messages();
    if (!message_clear(false))
      curr_state.mode = MODE_DEFAULT;
    return true;
  case MODE_GETCELL:
    return true; // do nothing
  case MODE_DIRECTIONAL:
    // Act like keyDownEvent in Main_Form_HandleEvent
    if ((valid = convert_char_to_dir(command)))
      valid = do_dir_command();
    if (valid) bump_state();
    else reset_state(&curr_state);
    handled = true;
    break;
  case MODE_DEFAULT:
    if (command == HWB_FONT_CMD) {
      // just like "case menu_mainFont" in main form event handler
      toggle_itsy();
      move_visible_window(you.ux, you.uy, true);
      refresh();
      took_time = false; // or maybe I should 'return true'.
      handled = true;
    } else if (command == HWB_SHIFT_CMD) {
      // act like we toggled the CapsLock
      GrfGetState(&capslock_on, &numlock_on, &tmp_shift, &autoshifted);
      GrfSetState(!capslock_on, numlock_on, false);
      return true;
    } else {
      // Act like keyDownEvent in Main_Form_HandleEvent
      if (message_clear(false)) {
	show_messages();
	//    command_count = 0; // I do need this - it just doesn't exist yet
	return true;
      }
      handled = do_command(command);
      if (!handled)
	if (do_inv_command(command)) return true;
    }
    break;
  }

  end_turn_start_turn();
  return handled;
}


/*****************************************************************************
 *                                                                           *
 *                      OpenDatabase                                         *
 *                                                                           *
 *****************************************************************************/
static Boolean OpenDatabase(void)
{
  // The '0' arguments are all 'card number'.
  // Note I may have several databases of the same type, so
  //  DmOpenDatabaseByTypeCreator is not going to work.

  LocalID dbID;
  Boolean created = false;
  // We cannot run if we cannot find the phDB, so return error.
  // hm, shouldn't it be read-only?
  if (0 == (dbID = DmFindDatabase(0, phDBName))) return 1;
  if (0 == (phDB = DmOpenDatabase(0, dbID, dmModeReadWrite))) return 1;

  // We can run if we cannot find the phSaveDB.  Just create it.
  if (0 == (dbID = DmFindDatabase(0, phSaveDBName))) {
    if (DmCreateDatabase(0, phSaveDBName, phAppID, phSaveDBType, false))
      return 1;
    created = true;
    if (0 == (dbID = DmFindDatabase(0, phSaveDBName))) return 1;
  }
  if (0 == (phSaveDB = DmOpenDatabase(0, dbID, dmModeReadWrite))) return 1;

  // We can run if we cannot find the phBonesDB.  Just create it.
  if (0 == (dbID = DmFindDatabase(0, phBonesDBName))) {
    if (DmCreateDatabase(0, phBonesDBName, phAppID, phBonesDBType, false))
      return 1;
    created = true;
    if (0 == (dbID = DmFindDatabase(0, phBonesDBName))) return 1;
  }
  if (0 == (phBonesDB = DmOpenDatabase(0, dbID, dmModeReadWrite))) return 1;

  // We can run if we cannot find the phScoreDB.  Just create it.
  if (0 == (dbID = DmFindDatabase(0, phScoreDBName))) {
    if (DmCreateDatabase(0, phScoreDBName, phAppID, phScoreDBType, false))
      return 1;
    created = true;
    if (0 == (dbID = DmFindDatabase(0, phScoreDBName))) return 1;
  }
  if (0 == (phScoreDB = DmOpenDatabase(0, dbID, dmModeReadWrite))) return 1;

  return 0;
}

/*****************************************************************************
 *                      ApplicationHandleEvent                               *
 *****************************************************************************/
static Boolean ApplicationHandleEvent(EventPtr e)
{
    FormPtr frm;
    Word    formId;
    Boolean handled = false;

    if (e->eType == frmLoadEvent) {
	formId = e->data.frmLoad.formID;
	frm = FrmInitForm(formId);
	FrmSetActiveForm(frm);

	switch(formId) {
	case MainForm:
	    FrmSetEventHandler(frm, Main_Form_HandleEvent);
	    break;
	case SnowCrashForm:
	    FrmSetEventHandler(frm, SnowCrash_Form_HandleEvent);
	    break;
	case AboutForm:
	    FrmSetEventHandler(frm, About_Form_HandleEvent);
	    break;
	case Chargen1Form:
	    FrmSetEventHandler(frm, Chargen1_Form_HandleEvent);
	    break;
	case Chargen2Form:
	    FrmSetEventHandler(frm, Chargen2_Form_HandleEvent);
	    break;
	case MapForm:
	    FrmSetEventHandler(frm, Map_Form_HandleEvent);
	    break;
	case MsgLogForm:
	    FrmSetEventHandler(frm, MsgLog_Form_HandleEvent);
	    break;
	case SenseForm:
	    FrmSetEventHandler(frm, Sense_Form_HandleEvent);
	    break;
	case InvForm:
	    FrmSetEventHandler(frm, Inv_Form_HandleEvent);
	    break;
	case InvActionForm:
	    FrmSetEventHandler(frm, InvAction_Form_HandleEvent);
	    break;
	case ObjTypeForm:
	    FrmSetEventHandler(frm, ObjType_Form_HandleEvent);
	    break;
	case EngraveForm:
	    FrmSetEventHandler(frm, Engrave_Form_HandleEvent);
	    break;
	case TombstoneForm:
	    FrmSetEventHandler(frm, Tombstone_Form_HandleEvent);
	    break;
	}
	handled = true;
    }

    return handled;
}

/*****************************************************************************
 *                      Preferences                                          *
 *****************************************************************************/

static void readPrefs()
{
  Word prefsSize;
  SWord prefsVersion;

  prefsSize = sizeof(HackPreferenceType);
  prefsVersion = PrefGetAppPreferences(phAppID, phAppPrefID, &my_prefs,
				       &prefsSize, true);
  if (!my_prefs.big_font)
    toggle_itsy();

  if (prefsVersion < phAppPrefVersion) {
    // hmm... remember to zero out the new space ...
    writePrefs();
  }
}
void writePrefs()
{
  Word prefsSize;

  prefsSize = sizeof(HackPreferenceType);
  PrefSetAppPreferences(phAppID, phAppPrefID, phAppPrefVersion,
			&my_prefs, prefsSize, true);
}




/*****************************************************************************
 *                      StartApplication                                     *
 *****************************************************************************/
Boolean evil = false;
Short snowcrash = 0;
Boolean IsVGA = false; // like IsColor.  tells you if you're on a Handera 330.
#ifdef I_AM_OS_2
FontPtr oldFontSix = 0;
#else
FontID SmallFont, MyFont;
FontID BigFont;
FontType *fontPtr; // was in StartApplication
#endif
static void init_fonts()
{
  DWord version;
#ifdef I_AM_OS_2
  void *font128 = 0;
#else
  VoidHand fontHandle;
  FontType *fontPtr;
#endif

  Version_GE_OS35 = false;

  // Deal with the font.
  FtrGet(sysFtrCreator, sysFtrNumROMVersion, &version);
#ifdef I_AM_OS_2
  // evil font kludges!
  custom_font = MemHandleLock(DmGetResource('NFNT', ItsyFont));
  if (version < 0x03000000L) {
    // Note: POSE will warn you that this line is a bad idea:
    oldFontSix = ((UIGlobalsPtr)GUIGlobalsP)->uiFontTable[6];
    ((UIGlobalsPtr)GUIGlobalsP)->uiFontTable[6] = custom_font;
  } else {
    snowcrash = 1;
    evil = true;
  }
#else
  if (version >= 0x03503000L) {
    Version_GE_OS35 = true;
  }
  if (version >= 0x03000000L) {
    UInt32 version2;
    UInt32 defaultFont;
    // Figure out whether we are on a Handera.
    IsVGA = _TRGVGAFeaturePresent(&version2);
    // Get a pointer to what is the "default" font.
    FtrGet(sysFtrCreator, sysFtrDefaultFont, &defaultFont);
    // Load up our custom font. "user defined fonts start from 129"
    fontHandle = DmGetResource('NFNT',ItsyFont);
    fontPtr = MemHandleLock(fontHandle);
    FntDefineFont(USERFONT, fontPtr);
    MyFont = USERFONT;
    // the custom font is uninstalled automatically when we leave the app;
    // however, fontHandle must remain locked until then.
    if (IsVGA) {
      if (VgaIsVgaFont(defaultFont)) { // vga fonts are 50% LARGER
	SmallFont = VgaVgaToBaseFont(defaultFont);
	BigFont = defaultFont;
      } else {
	SmallFont = defaultFont;
	BigFont = VgaBaseToVgaFont(defaultFont);
      }
      //FntSetFont(SmallFont);// So the forms OTHER than Main have non-VGA font
    } else {
      BigFont = defaultFont;
      SmallFont = USERFONT;
    }
  } else {
    snowcrash = 2;
    evil = true;
  }
#endif
}
static void init_handera()
{
#ifndef I_AM_OS_2
  if (IsVGA) {// If Handera, go to hi res mode
    //    VgaSetScreenMode(screenMode1To1, rotateModeNone);
    //    init_display_size(); // xxx not impl yet
  }
#endif
}
static void uninit_fonts()
{
  //  VoidHand h;
  DWord version;
#ifdef I_AM_OS_2
  //void *font128 = 0;
#else
  VoidHand fontHandle;
#endif

  FtrGet(sysFtrCreator, sysFtrNumROMVersion, &version);
#ifdef I_AM_OS_2
  if (version < 0x03000000L) {
    if (oldFontSix) {
      ((UIGlobalsPtr)GUIGlobalsP)->uiFontTable[6] = oldFontSix;
      oldFontSix = 0;
      //      if ((custom_font = ((UIGlobalsPtr)GUIGlobalsP)->uiFontTable[6]))
      //	MemHandleUnlock(font128);
    }
  }
#else
  if (version >= 0x03000000L) {
    fontHandle = DmGetResource('NFNT',ItsyFont);
    MemHandleUnlock(fontHandle);
  }  
#endif
}

static Word StartApplication(void)
{
  Boolean found_my_character = false;
  Word error = 0;

  init_fonts();

  // Return error if we don't have phDB.  Open or create phSaveDB.
  error = OpenDatabase();
  if (error && !evil) {
    snowcrash = 3;
    evil = true;
  }

  if (evil) {
    FrmGotoForm(SnowCrashForm);
    return 0;
  }

  init_handera();

  // Lock the various database records
  lock_const_recs();
  lock_volatile_recs();
  /*  data_record_lock(true);  */
  alloc_message_log();

  // Read user preferences.
  readPrefs();
  
/*
  TO DO:
  Call lock.c function to check for existence of a You Crashed record
  If it exists, DO NOT call load_char.. call antisave_char and proceed.
  Else, call function to create a You Crashed record and call load_char
  (outcome for both branches is: You Crashed record now exists.)
*/

  found_my_character = dorecover(); 

  /*
  found_my_character = load_char();

  game_init(); // so, hm, currently that only initializes the store inv?

  came_from_generation = !found_my_character;
  if (recover_crash()) // 'true' if we need to recover from a crash
    if (found_my_character) {
      if (0 == FrmAlert(CrashP)) {
	// "Just Quit"
	you.dead = true;
	StrCopy(died_from, "software bug");
	FrmGotoForm(DeathForm);
	return 0;
      } else {
	// "Recover Me"
	came_from_generation = true; // Generate the level all over again! ha!
	dun_level = 0; // Send you to the town!  Think this will work?
	// DANGER DANGER
	// There may still be inconsistent counters, e.g. inventory, equipment!
	// But you may consider this resuscitation worth the risk.
      }
    } // else there was no save file, no need for recovery

  if (found_my_character)
    FrmGotoForm(MainForm);
  else {
    FrmGotoForm(CharRaceForm);
    FrmPopupForm(IntroForm); // let's try this puppy out
  }
  */
  if (found_my_character)
    FrmGotoForm(MainForm); // XXXXXX test test test test
  else
    FrmGotoForm(Chargen1Form);
  
  return 0;
}


/*****************************************************************************
 *                      StopApplication                                      *
 *****************************************************************************/
#define free_me(a)  h = MemPtrRecoverHandle((a)); if (h) MemHandleFree(h);
static void StopApplication(void)
{

  if (evil) return;

  uninit_fonts();

  // try to save!  woohoo.  but I should only do this if you're not dead...
  /*
  if (!death) save_char(); // returns false on failure; ignored for now.
  else        antisave_char(); // delete old save file, if any

  didnt_crash();
  data_record_lock(false);
  */

  writePrefs(); // perhaps I should do it earlier..

  dosave(); // always save, we'll check for "already dead" when we load it.
  unlock_const_recs();
  unlock_volatile_recs();
  // possibly I should free some things too.
  // "everything" that is saved gets freed by the save routines.
  // (or, it should be, anyway.)

  FrmSaveAllForms();
  FrmCloseAllForms();

  if (phDB!=NULL) {
    DmCloseDatabase(phDB);
    phDB = NULL;
  }
  if (phSaveDB!=NULL) {
    DmCloseDatabase(phSaveDB);
    phSaveDB = NULL;
  }
  // free stuff

}


/*****************************************************************************
 *                      EventLoop                                            *
 *****************************************************************************/

/* The main event loop */


static void EventLoop(void)
{
    Word err;
    EventType e;
     
    do {
	EvtGetEvent(&e, evtWaitForever);
	// first see if it's a hardware button thing!!!
	// don't ask me what the poweredOnKeyMask is, though; cargo cult.
	// Do special hardware button things only if:
	// it's a hardware button event, [you're alive,] and in the main form.
	if ( (e.eType != keyDownEvent)
	     || !my_prefs.use_hardware
	     || (e.data.keyDown.modifiers & poweredOnKeyMask)
	     // || (FrmGetActiveFormID() != MainForm)
	     || !buttonsHandleEvent(&e) )
	  // now proceed with usual handling
	  if (! SysHandleEvent (&e))
	    if (! MenuHandleEvent (NULL, &e, &err))
	      if (! ApplicationHandleEvent (&e))
		FrmDispatchEvent (&e);
    } while (e.eType != appStopEvent);
}


/* Main entry point; it is unlikely you will need to change this except to
   handle other launch command codes */
/*****************************************************************************
 *                      PilotMain                                            *
 *****************************************************************************/
DWord PilotMain(Word cmd, Ptr cmdPBP, Word launchFlags)
{
    Word err;

    if (cmd == sysAppLaunchCmdNormalLaunch) {

      err = StartApplication();
      if (err) return err;

      EventLoop();
      StopApplication();

    } else {
      return sysErrParamErr;
    }

    return 0;
}
