/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
//#include "lock-externs.h"
#include "paleohackRsc.h"

extern Short msglog_mode; // just for show msgs

// flotsam from kMoria:
//Boolean just_throwing = false; // for entering this form via 't' (throw)
extern previous_state curr_state;

Boolean dropped_something;

Short inventory_item;
Char **inventory_display;
Int8 *inv_display_ix;
Short inv_display_ctr; /* how many of inventory_display are used; value may be
			  from i to 2*i, inclusive, where i is the number
			  of items in your inventory. */
static obj_t * get_nth_item(Short n) SEC_2;
static void refresh_inv(Word lst_i) SEC_2;

static Boolean handle_invbtn_frob(FormPtr frm, Word lst_i, Boolean b) SEC_2;
static Boolean perform_action(FormPtr frm, Word lst_i, obj_t *otmp,
			      Short iverb, Boolean *worked) SEC_5;
static Boolean handle_invbtn_drop(Word lst_i) SEC_2;
static Boolean handle_invbtn_throw() SEC_2;
static Boolean handle_invbtn_cancel() SEC_2;
static void handle_inv_dip() SEC_2;
static void handle_inv_name() SEC_2;
static void handle_inv_call() SEC_2;
static void do_inv_wield(Word lst_i) SEC_2;
static void handle_inv_engrave() SEC_2;
static Boolean handle_invbtn_extra() SEC_2;
static Boolean handle_invact_none() SEC_5;

static void show_inven(FormPtr frm, ListPtr lst, Boolean skip_some) SEC_2;
static void free_inventory_select(FormPtr frm) SEC_2;
static Short do_what(obj_t *otmp) SEC_2;
static void dwimify_buttons(FormPtr frm, Short item) SEC_2;
static Boolean match_and_fire(FormPtr frm, Word lst_i, Char c, Short num_btns) SEC_4;
static Boolean already_in_use(obj_t *otmp, Char *word) SEC_4;
static Boolean skip_this_item(obj_t *otmp) SEC_4;

typedef struct getobj_info_s {
  UChar allowcnt; // 0, 1, 2.
  Boolean allowgold;
  Boolean allowall;
  Boolean allownone;
  UChar action;
  Char let[10];
  Char word[14];
} getobj_info_t;
getobj_info_t getobj_info;

//Boolean show_extra_inv_btns;
void end_turn_start_turn(); // in main.c
Boolean Inv_Form_HandleEvent(EventPtr e)
{
  Boolean handled = false;
  FormPtr frm;
  ListPtr lst;
  CharPtr label;
  RectangleType r;
  Char c;
  Short s;

  switch (e->eType) {

  case frmOpenEvent:
    frm = FrmGetActiveForm();
    /* ...init form... */
    inventory_item = -1;
    inventory_display = NULL;
    dropped_something = false;
    lst = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, list_if));
    show_inven(frm, lst, false);
    //    show_extra_inv_btns = false;
    //    if (just_throwing)
    //      select_best_missile(frm);
    FrmDrawForm(frm);
    //    show_weight();
    // /*    show_invbtns = SHOW_DEFAULT; */
    dwimify_buttons(frm, inventory_item);
    handled = true;
    break;
    
  case lstSelectEvent:
    frm = FrmGetActiveForm();
    lst = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, list_if));
    s = e->data.lstSelect.selection;// - 1;
    if (s >= 0 && s < inv_display_ctr)
      inventory_item = inv_display_ix[s];
    else
      inventory_item = -1;
    dwimify_buttons(frm, inventory_item);
    // also clear the message space
    //    if (old_inven_weight == inven_weight) {
    RctSetRectangle(&r, 0, 128, 156, 11);
    WinEraseRectangle(&r, 0);
    //  } else
    //      show_weight(0);
    handled = true;
    break;

  case keyDownEvent:
    // hardware button -- or else graffiti.
    frm = FrmGetActiveForm();
    lst = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, list_if));
    switch(e->data.keyDown.chr) {
    case pageUpChr:
      if (LstScrollList(lst, winUp, 10)) {//  10 items visible at a time
	LstSetSelection(lst, -1);
	inventory_item = -1;
	dwimify_buttons(frm, inventory_item);
      }
      handled = true;
      break;
    case pageDownChr:
      if (LstScrollList(lst, winDown, 10)) {
	LstSetSelection(lst, -1);
	inventory_item = -1;
	dwimify_buttons(frm, inventory_item);
      }
      handled = true;
      break;
    case ' ':
      if (inventory_item == -1)
	inventory_item = 0;
      else {
	s = LstGetSelection(lst);
	if (s >= inv_display_ctr - 1) inventory_item = 0;
	else {
	  if (inv_display_ix[s] == inv_display_ix[s+1])
	    inventory_item += 2;
	  else
	    inventory_item++;
	}
      }
      LstSetSelection(lst, inventory_item);
      dwimify_buttons(frm, inventory_item);
      handled = true;
      break;
    default:
      c = e->data.keyDown.chr;
      if ((LstGetSelection(lst) != -1) && match_and_fire(frm, list_if, c, 4)) {
	// if we already selected in lst, and c is the first letter of a
	// button, we fire the button.
	// Why doesn't this work for 'Done'?
	handled = true;
	break;
      }
      if (c < 'A' || c > 'z' || (c > 'Z' && c < 'a')) {
	Word objid = -1;
	switch(c) {
	case '\n': objid = btn_if_frob; break;
	case 27: objid = btn_if_cancel; break;
	default: objid = -1; break;
	}
	if (objid != -1) {
	  hit_button_if_usable(frm, objid);
	  handled = true;
	}
	break;
      }
      // Find the line starting with this letter. in order; some have 2 lines
      // (or will later when I add the multi-line display option)
      // and in hack, letter-indices CAN be skipped (grrr) e.g. 'f' w/o 'e'
      // so we must search from 0 to 2*s (or, heck, to the end of the list.)
      // I will assert that all continuation-lines start with a SPACE ' '.

      //      if (c >= 'a') s = e->data.keyDown.chr - 'a';
      //      else s = (e->data.keyDown.chr - 'A') + 26; // lowercase first..
      s = 0; // we can't skip to c-'a' because there may be letter-index gaps.

      inventory_item = -1; // keep track of whether we found a match.
      do {
	label = LstGetSelectionText(lst, s);
	if (label && (label[0] == c)) {
	  LstSetSelection(lst, s); // index in List,
	  inventory_item = inv_display_ix[s]; // index in inventory.
	  break;
	}
      } while (++s < inv_display_ctr);
      if (inventory_item == -1) LstSetSelection(lst, -1);
      dwimify_buttons(frm, inventory_item);
      handled = true;	
      break;
    }
    break;


  case ctlSelectEvent:
    frm = FrmGetActiveForm();
    switch(e->data.ctlSelect.controlID) {
    case btn_if_frob:
      //      lst = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, list_if));
      handled = handle_invbtn_frob(frm, list_if, true);
      break;
    case btn_if_drop:
      handled = handle_invbtn_drop(list_if);
      break;
    case btn_if_throw:
      handled = true;
      handle_invbtn_throw();
      break;
    case btn_if_cancel:
      handled = handle_invbtn_cancel();
      break;
    case btn_if_extra:
      handled = handle_invbtn_extra();
      break;
    }

  case menuEvent:
    MenuEraseStatus(NULL);
    switch(e->data.menu.itemID) {
    case menu_invDip:
      handle_inv_dip();
      break;
    case menu_invName:
      handle_inv_name();
      break;
    case menu_invCall: // this is only for scrolls, potions, rings, wands.
      handle_inv_call();
      break;
    case menu_invWield:
      //      frm = FrmGetActiveForm();
      //      lst = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, list_if));
      do_inv_wield(list_if);
      break;
    case menu_invEngrave:
      handle_inv_engrave();
      break;
    case menu_invMsgs:
      msglog_mode = SHOW_MSGLOG;
      FrmPopupForm(MsgLogForm);
      break;
    }
    handled = true;
    break;
    // end menuEvent

  default:
    break;
  }
  return handled;
}


// may return NULL.
static obj_t * get_nth_item(Short n)
{
  Short i;
  obj_t *otmp = invent;
  if (n < 0) return NULL;
  for (i = 0 ; (i < n) && otmp ; i++)
    otmp = otmp->nobj;
  return otmp;
}
// after you drop or wield, call this to recalculate all the strings
static void refresh_inv(Word lst_i)
{
  ListPtr lst;
  FormPtr frm;
  if (you.dead) return; // in case of cockatrice-wielding.
  frm = FrmGetActiveForm();
  lst = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, lst_i));
  free_inventory_select(frm);
  show_inven(frm, lst, false);
  LstDrawList(lst);
  if (lst_i == list_if)
    dwimify_buttons(frm, inventory_item);
}

static Boolean handle_invbtn_frob(FormPtr frm, Word lst_i, Boolean dwim)
{
  Short iverb;
  obj_t *otmp;
  Boolean handled = true, worked = false;
  //  ListPtr lst;
  //  FormPtr frm = FrmGetActiveForm();
  //  lst = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, lst_i));
  if (inventory_item != -1) {
    otmp = get_nth_item(inventory_item);
    if (!otmp) return handled; // BUG if this happens.
    iverb = (dwim ? do_what(otmp) : getobj_info.action);
    if (perform_action(frm, lst_i, otmp, iverb, &worked))
      return handled;
    if (worked) {
      tick();
      show_messages();
      refresh_inv(lst_i);
    }
  }
  return handled;
}

extern Boolean took_time;
static Boolean perform_action(FormPtr frm, Word lst_i, obj_t *otmp,
			      Short iverb, Boolean *worked)
{
  //  ListPtr lst = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, lst_i));
  Boolean leave = true;
  Boolean ia = (FrmGetActiveFormID() == InvActionForm);
  tri_val_t result;
  switch(iverb) {
    // First the ones that don't exit:
  case ACT_PUTUP: inventory_item = -1; // fall through
  case ACT_WIELD: //
    do_inv_wield(lst_i); // does some of the work over again.. oh well
    if (ia) { end_turn_start_turn(); return true; }
    break;
  case ACT_AOFF: 
    if (ia) LeaveForm();
    *worked = do_remove_armor(otmp);
    if (*worked) took_time = true; // xxx
    if (ia) { end_turn_start_turn(); return true; }
    break;
  case ACT_ROFF:
    if (ia) LeaveForm(); // hmmm actually can this ever be from InvActionForm??
    *worked = do_remove_ring_helper(otmp);
    if (ia) { end_turn_start_turn(); return true; }
    break;
  case ACT_AWEAR: //
    if (ia) LeaveForm();
    *worked = do_wear_armor(otmp);
    if (*worked) took_time = true; // xxx
    if (ia) { end_turn_start_turn(); return true; }
    break;
  case ACT_RWEAR: //
    if (ia) LeaveForm();
    *worked = do_wear_ring(otmp);
    if (*worked) took_time = true; // xxx
    if (ia) { end_turn_start_turn(); return true; }
    break;
    // Next the ones that ALWAYS exit:
  case ACT_EAT: //
    free_inventory_select(frm);
    LeaveForm();
    *worked = do_eat(otmp);
    if (*worked) end_turn_start_turn();
    return leave;
  case ACT_QUAFF: //
    free_inventory_select(frm);
    LeaveForm();
    //   *worked = do_drink(otmp); // returns true if action is "complete"
    //   if (*worked) // otherwise, another form will finish the action for us.
    //     end_turn_start_turn();
    result = do_drink(otmp);
    took_time = (result != NO_OP);
    if (result != GO_ON) end_turn_start_turn();
    return leave;
  case ACT_READ: //
    free_inventory_select(frm);
    LeaveForm();
    result = do_read(otmp);
    took_time = (result != NO_OP);
    if (result != GO_ON) end_turn_start_turn();
    return leave;
  case ACT_ZAP: //
    free_inventory_select(frm);
    LeaveForm();
    if (do_zap(otmp)) { // returns true if we need to 'tick'
      took_time = true;
      end_turn_start_turn();
    }
    return leave;
  case ACT_APPLY: //
    free_inventory_select(frm);
    LeaveForm();
    if (do_apply(otmp)) { // returns true if we need to 'tick'
      took_time = true;
      end_turn_start_turn();
    }
    return leave;
    // Next the ones that always exit but maybe don't take time?
  case ACT_CALL:
    handle_inv_call();
    break;
  case ACT_NAME:
    handle_inv_name();
    break;
  case ACT_DROP: case ACT_DROPALL:
    handle_invbtn_drop(lst_i);
    break;
  case ACT_DIP:
    handle_inv_dip();
    break; // xxx should return true sometimes instead
  case ACT_THROW:
    return handle_invbtn_throw();// XXX need to TEST after this is implemented.
  case ACT_ENGRAVE:
    handle_inv_engrave();
    return leave;
  case ACT_REFRIGERATE:
    //    if (!otmp || !in_ice_box(otmp))
    //      flags.move = multi = 0;
    free_inventory_select(frm);
    LeaveForm();
    put_in_ice_box(otmp); // returns true if we actually did something.
    // in hack, messing with the ice box actually always takes time,
    // even if we dn't do anything.
    took_time = true;
    end_turn_start_turn();
    return leave;
  }
  return false;
}


static Boolean handle_invbtn_drop(Word lst_i)
{
  obj_t *otmp;
  //  Word curfrm = FrmGetActiveFormID();

  if (inventory_item != -1) {
    //    free_inventory_select(frm);
    if ((otmp = get_nth_item(inventory_item))) { // BUG if this isn't true..
      drop(otmp);
      show_messages(); // 'drop' probably printed a message
      dropped_something = true;
    }
    //    show_inven(frm);
    //    LstDrawList(FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, list_if)));
    //    dwimify_buttons(frm, inventory_item);
    refresh_inv(lst_i);
    // Note: need to take a turn.  XXXX I don't have a mechanism yet...
    /*      //calc_bonuses(); end_of_turn(); start_of_turn(); */
    /*      take_a_turn(false); */
    // Maybe I should make n drops (within a session) take 1 turn.
  }
  //  just_throwing = false;
  return true;
}

static Boolean handle_invbtn_throw()
{
  FormPtr frm = FrmGetActiveForm();
  obj_t *otmp;
  if (inventory_item == -1) return false;
  otmp = get_nth_item(inventory_item);
  if (!otmp) return false;

  free_inventory_select(frm);
  LeaveForm();

  curr_state.cmd = 't';
  curr_state.item = otmp;
  curr_state.mode = MODE_DIRECTIONAL;
  draw_directional();
  return true;
}

static Boolean handle_invbtn_cancel()
{
  FormPtr frm = FrmGetActiveForm();
  //  ListPtr lst;
  //  lst = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, list_if));
  free_inventory_select(frm);
  LeaveForm();
  if (dropped_something) {
    took_time = true;
    end_turn_start_turn();
  } else {
    tock();
    // need the refresh in case time passed and the dog moved;
    // need the show_messages to clear old msgs from before we entered inv;
    // need the print_stats in case we messed around with armor.
  }
  //  just_throwing = false;
  return true;
}


static void handle_inv_dip()
{
  if (inventory_item != -1) {
    message("dip!");
    if (!(curr_state.item = get_nth_item(inventory_item)))
      return; // BUG if this happens.
    LeaveForm();
  }
}
Int8 engrave_type;
extern Short engrave_or_what;
static void handle_inv_name()
{
  if (inventory_item != -1) {
    //    message("name!");
    if (!(curr_state.item = get_nth_item(inventory_item)))
      return; // BUG if this happens.
    LeaveForm();
    // EXPERIMENTAL:
    engrave_type = 0; // should be ignored anyway
    engrave_or_what = ACT_NAME;
    FrmPopupForm(EngraveForm);
  }
}
static void handle_inv_call()
{
  Short itype;
  obj_t *otmp;
  if (inventory_item != -1) {
    // restrict types..
    if (!(otmp = get_nth_item(inventory_item)))
      return; // BUG if this happens.
    itype = otmp->olet;
    if (itype == SCROLL_SYM || itype == POTION_SYM ||
	itype == RING_SYM || itype == WAND_SYM) {
      //      message("call!");
      LeaveForm();
      // EXPERIMENTAL:
      clone_for_call(otmp);
      engrave_type = 0; // should be ignored anyway
      engrave_or_what = ACT_CALL;
      FrmPopupForm(EngraveForm);
    } else {
      message("must be scroll, potion, ring, or wand");
    }
  }
}

static void do_inv_wield(Word lst_i)
{
  obj_t *wep = NULL;
  Word curfrm = FrmGetActiveFormID();

  if (inventory_item != -1)
    wep = get_nth_item(inventory_item);
  if (curfrm == InvActionForm)
    LeaveForm();
  if (do_wield(wep)) {
    tick();
    if (curfrm == InvForm) {
      show_messages();
      refresh_inv(lst_i);
    }
  }
}

static void handle_inv_engrave()
{
  //  obj_t *otmp = NULL;
  if (inventory_item != -1)
    curr_state.item = get_nth_item(inventory_item);
  else 
    curr_state.item = NULL;
  // might be NULL?
  LeaveForm();
  if (check_do_engrave())
    tick(); // "failed but took time."
}


/*
// swiped from HandEra330ified iRogue
static void ResizeList(FormPtr frm, UInt objid, UInt bot_objid, Short space)
{
  // Wonder if I need to erase the old space.
  Short lines, x, y, y_bot;
  ListPtr lst;
  UInt objIndex = FrmGetObjectIndex(frm, objid);
  lst = FrmGetObjectPtr(frm, objIndex);
  // Ok, the lists I'm calling this on all are positioned just below the
  // title bar, and end just above a row of buttons, one of which
  // is supplied in bot_objid.  Make the list as big as possible!
  //
  // In 160x160 these (x, y)'s probably start out at (0, 15) and (5, 141).
  // We really only care about the y's.
  FrmGetObjectPosition(frm, objIndex, &x, &y);
  FrmGetObjectPosition(frm, FrmGetObjectIndex(frm, bot_objid), &x, &y_bot);
  // Ok.  y_bot - y is the absolute max space available to the list.
  // We want to allow 'space' pixels below the list as well.
  // Figure out how many lines we can put into that space (yay int division).
  lines = ((y_bot - space) - y) / FntLineHeight();
  LstSetHeight(lst, lines);
}
*/

// Note: When I put in "scrolling" I had better swipe more HE300-iRogue code
// since the list's physical height is no longer constant.
static Boolean handle_invbtn_extra()
{
  /*
  FormPtr frm = FrmGetActiveForm();
  UInt highest_button;
  show_extra_inv_btns = !show_extra_inv_btns;
  if (show_extra_inv_btns)    highest_button = btn_if_dip;
  else    highest_button = btn_if_cancel;
  ResizeList(frm, list_if, highest_button, 11);
  FrmDrawForm(frm);
  inventory_item = -1;
  dwimify_buttons(frm, inventory_item);
  */
  return true;
}



// hacked up, seems to work but could use the double-line thing.
// Hey, I could make this take a 'filter by type' argument.
// Everything else would be cool with it, since the selection
// tolerates gaps.
#define LIST_WIDTH 154 /* was 156 */
#define LONG_WORD 10
static void show_inven(FormPtr frm, ListPtr lst, Boolean skip_some)
{
  Short i, j, invdisp_item = -1;
  //  Char tmp_val[160], c, ctmp = ' ';
  Char *tmp_val, c, buf[10], ctmp = ' ';
  Int width_str, length_str, uncut_len;
  Boolean fits;
  obj_t *otmp;
  Short inven_ctr;

  for (otmp = invent, inven_ctr = 0 ; otmp != NULL ; otmp = otmp->nobj)
    if (!skip_some || !skip_this_item(otmp))
      inven_ctr++;

  if (inven_ctr <= 0) {
    //    just_throwing = false;
    inventory_item = -1;
    // dwimify should take care of disabling buttons.
    return;
  }

  // first, figure out whether we should free existing list.
  if (inventory_display != NULL)
    free_inventory_select(frm);

  // count inventory... allocate space.  we'll need at most two lines per item.
  inventory_display = (Char **) md_malloc(sizeof(Char *) * 2*inven_ctr);
  inv_display_ix = (Int8 *) md_malloc(sizeof(Int8) * 2*inven_ctr);

  // Print the items
  c = 'a'-1;
  i = inv_display_ctr = 0; // i counts ITEMS, i_d_c counts LINES.
  for (otmp = invent ; otmp != NULL ; otmp = otmp->nobj, i++) {
    if (flags.invlet_constant)    c = otmp->invlet; // "Gaps"
    else    c = (c >= 'z') ? 'A' : c + 1; // "No gaps"
    if (skip_some && skip_this_item(otmp))
      continue;
    if (i == inventory_item) invdisp_item = inv_display_ctr;// select it later.
    tmp_val = doname(otmp);
    StrPrintF(buf, "%c - ", c);
    // calculate width of buf; subtract from width of List.
    uncut_len = StrLen(tmp_val) + StrLen(buf); // max chars to allow for all
    length_str = StrLen(buf);
    width_str = LIST_WIDTH - FntCharsWidth(buf, length_str);// for just tmp_val
    length_str = StrLen(tmp_val);
    FntCharsInWidth(tmp_val, &width_str, &length_str, &fits);
    // length_str is now set to fiting chars, width_str to fitting pixels.
    if (!fits) { // WRAP
      // I'm skipping the "subtract width of danged scroll-arrows" from kMoria
      j = 0; // try to find a ' ' to cut at.
      while (tmp_val[length_str-j] != ' ' && j < LONG_WORD) j++;
      if (j < LONG_WORD) length_str -= j; // found a ' ', go back to it.
      ctmp = tmp_val[length_str];
    }
    tmp_val[length_str] = '\0';
    inventory_display[inv_display_ctr] = (Char *) md_malloc(1 + length_str +
							    StrLen(buf));
    StrPrintF(inventory_display[inv_display_ctr], "%s%s", buf, tmp_val);
    inv_display_ix[inv_display_ctr] = i;
    inv_display_ctr++;
    if (!fits) { // second verse, same as the first
      tmp_val[length_str] = ctmp;
      inventory_display[inv_display_ctr] = (Char *) md_malloc(3 + 1 +
						     (uncut_len - length_str));
      StrPrintF(inventory_display[inv_display_ctr], "   %s",
		&tmp_val[length_str]);
      inv_display_ix[inv_display_ctr] = i;
      inv_display_ctr++;
    }
  }

  LstSetListChoices(lst, inventory_display, inv_display_ctr);
  LstSetSelection(lst, invdisp_item); // -1, or [0 .. inv_display_ctr-1]
}

/*
 *  Free some space that we used in the inventory-selection form
 */
static void free_inventory_select(FormPtr frm)
{
  Short i;
  VoidHand h;
  Word curfrm = FrmGetActiveFormID();
  ListPtr lst;

  //  if (inven_ctr <= 0) return;
  if (!invent) return;
  
  if (curfrm == InvForm)
    lst = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, list_if));
  else if (curfrm == InvActionForm) {
    lst = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, list_iaf));
  } else
    return; // someone screwed up..
  LstSetListChoices(lst, NULL, 0);
  for (i = 0 ; i < inv_display_ctr ; i++) {
    h = MemPtrRecoverHandle(inventory_display[i]);
    if (h) 
      MemHandleFree(h);
  }
  h = MemPtrRecoverHandle(inventory_display);
  if (h) 
    MemHandleFree(h);
  inventory_display = NULL;
  h = MemPtrRecoverHandle(inv_display_ix);
  if (h) 
    MemHandleFree(h);
  inv_display_ix = NULL;
}



// not actually needed
/*
static Short invlet_to_i(Char c)
{
  obj_t *otmp;
  Short i = 0;
  for (otmp = invent ; otmp != NULL ; otmp = otmp->nobj)
    if (otmp->invlet == c) return i;
    else i++;
  return -1;
}
*/

// This determins what string is printed on the frob button.
static Short do_what(obj_t *otmp)
{
  // amulet, tool, food, weapon, armor, chain, rock, ball, potion, scroll,
  // wand, ring, gem.
  Short item_type;
  if (!otmp) return ACT_NONE;
  item_type = otmp->olet;
  switch(item_type) {
  case WEAPON_SYM:
    if (uwep == otmp) return ACT_PUTUP;
    else return ACT_WIELD;
  case ARMOR_SYM :
    if (otmp->owornmask & (W_ARMOR | W_RING)) return ACT_AOFF;
    else return ACT_AWEAR;
  case RING_SYM  :
    if (otmp->owornmask & (W_ARMOR | W_RING)) return ACT_ROFF;
    else return ACT_RWEAR;
  case FOOD_SYM   : return ACT_EAT;
  case POTION_SYM : return ACT_QUAFF;
  case SCROLL_SYM : return ACT_READ;
  case WAND_SYM   : return ACT_ZAP;
  case TOOL_SYM   : return ACT_APPLY;
  case AMULET_SYM : // fall
  case BALL_SYM   : // fall
  case CHAIN_SYM  : // fall
  case GEM_SYM    : // fall
  case ROCK_SYM   : return ACT_NONE;
  }
  return ACT_NONE;
  // You can wield anything. Dammit I need more buttons...
  // other verbs: there's also
  // "put in"  (anything but expects food)
  // "drop money" and "pay"
  // "call" for ?!=/
  // 'E'ngrave or "write with" for #-)/
  // "dip" for <anything> INTO <potion>
  // "identify" for I think anything
  // "wield" for #-)  what's that mean?
}

// These probably need to change.
// I think hack has no 'refill'.  the 'read' can all be one read?
#define NUM_LABELS 11
const Char if_button_labels[NUM_LABELS][7] = {
  "apply", "eat", "quaff", "read",  "wear", "off",
  "wear",  "off", "wield", "put up", "zap"
};
static void dwimify_buttons(FormPtr frm, Short item)
{
  Short /*  itype, */ iverb;
  obj_t *otmp;
  ControlPtr btn;

  /*
  if (just_throwing) {
    // enable throw, everything else ok by default.
    btn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, btn_if_throw));
    if (item == -1) CtlHideControl(btn);
    else CtlShowControl(btn);
    return;
  }
  */

  //  btn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, btn_if_extra));
  //  CtlShowControl(btn);

  // hide everything if nothing selected
  if (item == -1 /*|| in_a_store*/ ) {
    btn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, btn_if_frob));
    CtlHideControl(btn);
    btn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, btn_if_throw));
    CtlHideControl(btn);
    btn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, btn_if_drop));
    CtlHideControl(btn);
    /*
    btn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, btn_if_dip));
    CtlHideControl(btn);
    btn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, btn_if_name));
    CtlHideControl(btn);
    btn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, btn_if_wield));
    CtlHideControl(btn);
    btn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, btn_if_call));
    CtlHideControl(btn);
    */
    return;
  }

  // no need to disable the 'drop' button when standing on object.. it stacks
  btn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, btn_if_drop));
  CtlShowControl(btn);
  btn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, btn_if_throw));
  CtlShowControl(btn);

  // based on type of item,
  // make button 1 usable or not, set its text.
  // it could be a function pointer if I felt evil, though,

  otmp = get_nth_item(item);
  if (!otmp) return; // BUG if this happens.

  //  itype = otmp->olet;
  iverb = do_what(otmp); // (itype);
  btn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, btn_if_frob));
  if (iverb == ACT_NONE)
    CtlHideControl(btn);
  else {
    CtlSetLabel(btn, if_button_labels[iverb-1]);
    CtlShowControl(btn);    
  }

  // based on type of item, maybe enable some extra buttons
  /*
  if (show_extra_inv_btns) {
    btn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, btn_if_dip));
    CtlShowControl(btn);
    btn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, btn_if_name));
    CtlShowControl(btn);
    btn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, btn_if_wield));
    if (itype != WEAPON_SYM) CtlShowControl(btn);
    else CtlHideControl(btn);
    btn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, btn_if_call));
    if (itype == SCROLL_SYM || itype == POTION_SYM ||
	itype == RING_SYM || itype == WAND_SYM)
      CtlShowControl(btn);
    else CtlHideControl(btn);
  }
  */
}


static const Word btn_ids[4] = {
  btn_if_frob, btn_if_drop, btn_if_throw, btn_if_cancel
};
static Boolean match_and_fire(FormPtr frm, Word lst_i, Char c, Short num_btns)
{
  ControlPtr btn;
  Short i;
  const Char *btnlabel;
  // Determine whether 'c' is the initial letter of a button in the form.
  // If so then returh the button objid.
  for (i = 0 ; i < num_btns ; i++) {
    btn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, btn_ids[i]));
    btnlabel = CtlGetLabel(btn);
    if (btnlabel[0] == c) {
      hit_button_if_usable(frm, btn_ids[i]);
      return true;
    }
  }
  // It didn't match a button.  There are some menu commands we could
  // try though.  Some have same letters...
  switch(c) {
  case 'd': handle_inv_dip(); break; // will never happen because of 'drop'.
  case 'n': handle_inv_name(); break;
  case 'c': handle_inv_call(); break;
  case 'w': do_inv_wield(lst_i); break; // might be 'wear'
  case 'e': handle_inv_engrave(); break; // might be 'eat'
  default:
    return false;
  }
  return true;
}


/* ----------------------------------------------------------------
getobj:

  (    use/apply
  0#%  put in
  0$#  drop
  #)   throw
  #    name
  ?!=/ call
  [    take off
  [    wear
  =    wear
  %    eat
  #-)/ write with
  !    drink
  #    dip
  !    dip into
  ?    read
  #-   wield
  /    zap
 */


static void update_ia_btns(FormPtr frm) SEC_4;
static void update_ia_btns(FormPtr frm)
{
  ControlPtr btn;
  btn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, btn_ia_frob));
  if (inventory_item != -1) {
    CtlShowControl(btn);
    CtlSetLabel(btn, getobj_info.word); // will it work?
  }
  else CtlHideControl(btn);
  btn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, btn_ia_all));
  CtlHideControl(btn);
  btn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, btn_ia_none));
  if (getobj_info.allownone)
    CtlShowControl(btn);
  else
    CtlHideControl(btn);
  btn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, btn_ia_cancel));
  CtlShowControl(btn);
}


Boolean InvAction_Form_HandleEvent(EventPtr e)
{
  Boolean handled = false;
  FormPtr frm;
  ListPtr lst;
  CharPtr label;
  RectangleType r;
  Char c;
  Short s;

  switch (e->eType) {

  case frmOpenEvent:
    frm = FrmGetActiveForm();
    /* ...init form... */
    inventory_item = -1;
    inventory_display = NULL;
    dropped_something = false;
    lst = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, list_iaf));
    show_inven(frm, lst, true);
    inventory_item = inv_display_ix[0];
    LstSetSelection(lst, 0);
    StrPrintF(ScratchBuffer, "%s what?", getobj_info.word);
    if ((c = ScratchBuffer[0]) >= 'a' && c <= 'z')
      ScratchBuffer[0] = c + ('A' - 'a');
    FrmCopyTitle(frm, ScratchBuffer);
    FrmDrawForm(frm);
    update_ia_btns(frm);
    handled = true;
    break;
    
  case lstSelectEvent:
    frm = FrmGetActiveForm();
    s = e->data.lstSelect.selection;// - 1;
    if (s >= 0 && s < inv_display_ctr)
      inventory_item = inv_display_ix[s];
    else
      inventory_item = -1;
    update_ia_btns(frm);
    // also clear the message space
    RctSetRectangle(&r, 0, 128, 156, 11);
    WinEraseRectangle(&r, 0);
    handled = true;
    break;

  case keyDownEvent:
    // hardware button -- or else graffiti.
    frm = FrmGetActiveForm();
    lst = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, list_iaf));
    switch(e->data.keyDown.chr) {
    case pageUpChr:
      if (LstScrollList(lst, winUp, 10)) {//  10 items visible at a time
	LstSetSelection(lst, -1);
	inventory_item = -1;
	update_ia_btns(frm);
      }
      handled = true;
      break;
    case pageDownChr:
      if (LstScrollList(lst, winDown, 10)) {
	LstSetSelection(lst, -1);
	inventory_item = -1;
	update_ia_btns(frm);
      }
      handled = true;
      break;
    case '\n': hit_button_if_usable(frm, btn_ia_frob); break;
    case '-': hit_button_if_usable(frm, btn_ia_none); break;
    case 27: hit_button_if_usable(frm, btn_ia_cancel); break;
    case ' ':
      // for some reason this doesn't seem to work right.
      // better fix it.
      s = LstGetSelection(lst);
      if (inventory_item == -1 || s == -1 || s >= inv_display_ctr - 1)
	s = 0;
      else if (inv_display_ix[s] == inv_display_ix[s+1])
	s += 2;
      else
	s++;
      inventory_item = inv_display_ix[s];
      LstSetSelection(lst, s);
      update_ia_btns(frm);
      handled = true;
      break;
    default:
      c = e->data.keyDown.chr;
      // Find the line starting with this letter. in order; some have 2 lines
      // (or will later when I add the multi-line display option)
      // and in hack, letter-indices CAN be skipped (grrr) e.g. 'f' w/o 'e'
      // so we must search from 0 to 2*s (or, heck, to the end of the list.)
      // I will assert that all continuation-lines start with a SPACE ' '.

      //      if (c >= 'a') s = e->data.keyDown.chr - 'a';
      //      else s = (e->data.keyDown.chr - 'A') + 26; // lowercase first..
      s = 0; // we can't skip to c-'a' because there may be letter-index gaps.

      inventory_item = -1; // keep track of whether we found a match.
      do {
	label = LstGetSelectionText(lst, s);
	if (label && (label[0] == c)) {
	  LstSetSelection(lst, s); // index in List,
	  inventory_item = inv_display_ix[s]; // index in inventory.
	  break;
	}
      } while (++s < inv_display_ctr);
      if (inventory_item == -1) LstSetSelection(lst, -1);
      update_ia_btns(frm);
      handled = true;	
      break;
    }
    break;


  case ctlSelectEvent:
    frm = FrmGetActiveForm();
    switch(e->data.ctlSelect.controlID) {
    case btn_ia_frob:
      {
	ControlPtr btn;
	btn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, btn_ia_cancel));
	CtlSetLabel(btn, "Done");
	CtlShowControl(btn);    
      }
      handled = handle_invbtn_frob(frm, list_iaf, false);
      if (handled) dropped_something = true; // xxx make it tick
      break;
    case btn_ia_all:
      //      handled = handle_invact_all();
      break;
    case btn_ia_none:
      handled = handle_invact_none();
      break;
    case btn_ia_cancel:
      // if took time, take ONE turn.
      handled = true;
      free_inventory_select(frm);
      LeaveForm();
      if (dropped_something) {
	took_time = true;
	end_turn_start_turn();
      }
      break;
    }

  default:
    break;
  }
  return handled;
}


static Boolean handle_invact_none()
{
  if (!getobj_info.allownone) return false; // should not happen.
  switch (getobj_info.action) {
  case ACT_WIELD:
    curr_state.item = NULL;
    inventory_item = -1;
    do_inv_wield(list_iaf);
    break;
  case ACT_ENGRAVE:
    curr_state.item = NULL;
    inventory_item = -1;
    handle_inv_engrave();
    break;
  default:
    return false;
  }
  return true;
}

// Tell show_inven() whether to skip THIS inventory item.
static Boolean skip_this_item(obj_t *otmp)
{
  if (getobj_info.allowall) return false;

  if (!*(getobj_info.let) || StrChr(getobj_info.let, otmp->olet) != NULL)
    if (!already_in_use(otmp, getobj_info.word))
      return false;
  return true;
}

// Determine whether we're already doing THIS verb to this object.
static Boolean already_in_use(obj_t *otmp, Char *word)
{
  if ((!StrCompare(word, "take off") &&
       !(otmp->owornmask & (W_ARMOR - W_ARM2)))
      || (!StrCompare(word, "wear") &&
	  (otmp->owornmask & (W_ARMOR | W_RING)))
      || (!StrCompare(word, "wield") &&
	  (otmp->owornmask & W_WEP)))
    return true;
  return false;
}

// Return false if we won't be needing to pop up a form.
Boolean getobj_init(Char *let, Char *word, UChar action)
{
  obj_t *otmp;
  Boolean already_verbing = false;
  Short valid_items;

  getobj_info.action = action;
  getobj_info.allowcnt = 0;
  getobj_info.allowgold = false;
  getobj_info.allowall = false;
  getobj_info.allownone = false;
  if (*let == '0') let++, getobj_info.allowcnt = 1;
  if (*let == '$') let++, getobj_info.allowgold = true;
  if (*let == '#') let++, getobj_info.allowall = true;
  if (*let == '-') let++, getobj_info.allownone = true;

  StrNCopy(getobj_info.let, let, 10);
  StrNCopy(getobj_info.word, word, 14);

  if ((getobj_info.allowgold && (you.ugold > 0)) ||
      getobj_info.allownone)
    return true;

  // Don't pop up a form if we don't have anything to verb.
  valid_items = 0;
  for (otmp = invent ; otmp ; otmp = otmp->nobj) {
    if (!skip_this_item(otmp))
      valid_items++;
    if (already_in_use(otmp, word))
      already_verbing = true;
  }
  if (valid_items <= 0) {
    StrPrintF(ScratchBuffer, "You don't have anything%sto %s.",
	      already_verbing ? " else " : " ", word);
    message(ScratchBuffer);
    //    show_messages();
    return false;
  }
  return true;
}




Boolean show_getobj()
{
  return false;
  // Set title to getobj_info.word ...

  // Not sure how to do the count yet.
  // Don't allow a count of higher than the selected item.

  // If we choose to do something to $,
  // after we set 'cnt' to user input or to 'you.ugold',
  // then create a 'mkgoldobj(cnt)' and pass it to the verb.
  // Maybe I could represent this option by putting
  // "$ 300 gold pieces" as the first item on the list,
  // and accepting '$' in addition to 'azAZ'.

  // If allownone, enable the 'Fingers' button.
  // Pass the '&zeroobj' address to the verb if this button is selected.

  // Cancel has the same behavior as passing NULL to the verb.

  // Some bits and pieces:
  /*
  if (cnt < 0 || otmp->quan < cnt) {
    pline("You don't have that many! [You have %u]", otmp->quan);
  }
  if (!allowall && let && !index(let, otmp->olet)) {
    StrPrintF(ScratchBuffer, "That is a silly thing to %s.", word);
    message(ScratchBuffer);
  }
  if (allowcnt == 2) {	// cnt given
    if (cnt == 0) return NULL;
    if (cnt != otmp->quan) {
      struct obj *obj;
      obj = splitobj(otmp, (Short) cnt);
      if (otmp == uwep) setuwep(obj);
    }
  }
  return(otmp);
  */
}
