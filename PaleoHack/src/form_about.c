/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
//#include "lock-externs.h"
#include "paleohackRsc.h"

extern Boolean took_time;


Boolean About_Form_HandleEvent(EventPtr e)
{
  Boolean handled = false;
  FormPtr frm;
    
  switch (e->eType) {

  case frmOpenEvent:
    frm = FrmGetActiveForm();
    FrmDrawForm(frm);
    handled = true;
    break;

  case ctlSelectEvent:
    switch(e->data.ctlSelect.controlID) {
    case btn_about_ok:
      LeaveForm();
      handled = true;
      break;
    case btn_about_license:
      FrmHelp(LicenseStr);
      handled = true;
      break;
    case btn_about_credits:
      FrmHelp(CreditStr);
      handled = true;
      break;
    }
    break;

  case keyDownEvent:
    switch(e->data.keyDown.chr) {
    case '\n':
      LeaveForm();
      handled = true;
      break;
    }
    break;

  default:
    break;
  }
  return handled;
}




Boolean SnowCrash_Form_HandleEvent(EventPtr e)
{
  Boolean handled = false;
  FormPtr frm;
    
  switch (e->eType) {

  case frmOpenEvent:
    frm = FrmGetActiveForm();
    FrmDrawForm(frm);
    handled = true;
    break;

  default:
    break;
  }
  return handled;
}


void put_char_at(Short row, Short col, Char ch, Boolean bold); // display.c
UChar sense_what;
obj_t *sense_by_what;
void end_turn_start_turn();
Boolean use_map_mode;
static void map_animate_char(Short x, Short y, Char c) SEC_5;
void map_draw_char_init();
void map_draw_char(Short x, Short y, Char c);

Boolean Sense_Form_HandleEvent(EventPtr e)
{
  Boolean handled = false;
  FormPtr frm;
  obj_t *obj;
  monst_t *mtmp;
  trap_t *ttmp;
  gold_t *gtmp;
  tri_val_t result;
    
  switch (e->eType) {

  case frmOpenEvent:
    frm = FrmGetActiveForm();
    FrmDrawForm(frm);
    if (use_map_mode)
      map_draw_char_init();
    flags.botl = BOTL_ALL; print_stats(0); // XXX
    switch(sense_what) {
    case SENSE_MONSTERS:
      for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
	if (mtmp->mx > 0)
	  map_animate_char(mtmp->mx, mtmp->my, mtmp->data->mlet);
      message("You sense the presence of monsters.");
      break;
    case SENSE_OBJECTS:
      for (obj = fobj; obj; obj = obj->nobj)
	map_animate_char(obj->ox, obj->oy, obj->olet);
      message("You sense the presence of objects.");
      break;
    case SENSE_GOLD:
      for (gtmp = fgold; gtmp; gtmp = gtmp->ngold)
	map_animate_char(gtmp->gx, gtmp->gy, GOLD_SYM);
      message("You feel very greedy, and sense gold!");
      break;
    case SENSE_GOLD_CONFUSED:
      for (ttmp = ftrap; ttmp; ttmp = ttmp->ntrap)
	map_animate_char(ttmp->tx, ttmp->ty, GOLD_SYM);
      message("You feel very greedy!");
      break;
    case SENSE_FOOD:
      for (obj = fobj; obj; obj = obj->nobj)
	if (obj->olet == FOOD_SYM)
	  map_animate_char(obj->ox, obj->oy, FOOD_SYM);
      message("Your nose tingles and you smell food!");
      break;
    case SENSE_FOOD_CONFUSED:
      for (obj = fobj; obj; obj = obj->nobj)
	if (obj->olet == POTION_SYM)
	  map_animate_char(obj->ox, obj->oy, FOOD_SYM);
      message("Your nose tingles and you smell something!");
      break;
    }
    if (use_map_mode) map_draw_char(you.ux, you.uy, '@');
    else prme(); // XXXXX why isn't this working?
    show_messages();
    handled = true;
    break;

  case penDownEvent:
  case keyDownEvent:
    LeaveForm();
    // XXX we also need to call tick, because do_drink could not!!!
    // hmm, also need to clear the (previous) message from mainform.
    message("the vision fades");
    if (sense_what == SENSE_MONSTERS || sense_what == SENSE_OBJECTS)
      // returns true if it's ok for us to tick,
      // false if we should postpone tick....  xxxxxx
      result = finish_do_drink(sense_by_what, false, false);
    else {
      // SENSE_GOLD_CONFUSED: known == false.  all else known==true.
      // SENSE_*_CONFUSED: confused == true.  all else confused==false.
      // hm, seems.. asymmetrical..
      result = finish_do_read(sense_by_what,
			      (sense_what != SENSE_GOLD_CONFUSED),
			      (sense_what == SENSE_GOLD_CONFUSED ||
			       sense_what == SENSE_FOOD_CONFUSED));
    }
    sense_by_what = NULL;
    sense_what = SENSE_NONE;

    if (result == NO_OP)
      took_time = false;
    if (result != GO_ON)
      end_turn_start_turn();// don't call if we're going on to EngraveForm.

    handled = true;
    break;

  default:
    break;
  }
  return handled;
}


static Boolean sense_mon_vis() SEC_5;
static Boolean sense_gold_vis() SEC_5;
static Boolean sense_trap_vis() SEC_5;
static Boolean sense_obj_vis(Char let) SEC_5;


void clear_visible(); // display.c
void sense_init_screen()
{
  //  extern Boolean itsy_on;
  //  Boolean itsy_was_on = itsy_on;
  //  Char c = '\0';

  use_map_mode = false;

  // If we're in large font and can't see anything, switch.
  // (If this didn't help, switch back.)
  /*
  if (!itsy_on) {
    switch(sense_what) {
    case SENSE_MONSTERS:
      if (!sense_mon_vis()) {
	toggle_itsy();
	if (!sense_mon_vis())
	  toggle_itsy();
      }
      break;
    case SENSE_GOLD:
      if (!sense_gold_vis()) {
	toggle_itsy();
	if (!sense_gold_vis())
 	  toggle_itsy();
      }
      break;
    case SENSE_GOLD_CONFUSED:
      if (!sense_trap_vis()) {
	toggle_itsy();
	if (!sense_trap_vis())
 	  toggle_itsy();
      }
      break;
    case SENSE_FOOD:
      if (!c) c = FOOD_SYM;
    case SENSE_FOOD_CONFUSED:
      if (!c) c = POTION_SYM;
    case SENSE_OBJECTS:
      if (!sense_obj_vis(c)) {
	toggle_itsy();
	if (!sense_obj_vis(c))
 	  toggle_itsy();
      }
      break;
    }
  }
  */

  // If we can't see anything, we will need to use map mode.
  switch(sense_what) {
  case SENSE_MONSTERS:      use_map_mode = !sense_mon_vis(); break;
  case SENSE_GOLD:          use_map_mode = !sense_gold_vis(); break;
  case SENSE_GOLD_CONFUSED: use_map_mode = !sense_trap_vis(); break;
  case SENSE_OBJECTS:       use_map_mode = !sense_obj_vis(0); break;
  case SENSE_FOOD:          use_map_mode = !sense_obj_vis(FOOD_SYM); break;
  case SENSE_FOOD_CONFUSED: use_map_mode = !sense_obj_vis(POTION_SYM); break;
  }
  
  /*
  if (!itsy_was_on && itsy_on) { // we toggled itsy...
    clear_visible();
    refresh();
  }
  */

}

// This version returns true if ANY are visible
/*
static Boolean sense_mon_vis()
{
  monst_t *mtmp;
  for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
    if (mtmp->mx > 0)
      if (fits_on_screen(mtmp->mx, mtmp->my))
	return true;
  return false;
}
static Boolean sense_gold_vis()
{
  gold_t *gtmp;
  for (gtmp = fgold; gtmp; gtmp = gtmp->ngold)
    if (fits_on_screen(gtmp->gx, gtmp->gy))
      return true;
  return false;
}
static Boolean sense_trap_vis()
{
  trap_t *ttmp;
  for (ttmp = ftrap; ttmp; ttmp = ttmp->ntrap)
    if (fits_on_screen(ttmp->tx, ttmp->ty))
      return true;
  return false;
}
static Boolean sense_obj_vis(Char let)
{
  obj_t *obj;
  for (obj = fobj; obj; obj = obj->nobj)
    if (!let || obj->olet == let)
      if (fits_on_screen(obj->ox, obj->oy))
      return true;
  return false;
}
*/
// This version returns true if ALL are visible
static Boolean sense_mon_vis()
{
  monst_t *mtmp;
  for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
    if (mtmp->mx > 0)
      if (!fits_on_screen(mtmp->mx, mtmp->my))
	return false;
  return true;
}
static Boolean sense_gold_vis()
{
  gold_t *gtmp;
  for (gtmp = fgold; gtmp; gtmp = gtmp->ngold)
    if (!fits_on_screen(gtmp->gx, gtmp->gy))
      return false;
  return true;
}
static Boolean sense_trap_vis()
{
  trap_t *ttmp;
  for (ttmp = ftrap; ttmp; ttmp = ttmp->ntrap)
    if (!fits_on_screen(ttmp->tx, ttmp->ty))
      return false;
  return true;
}
static Boolean sense_obj_vis(Char let)
{
  obj_t *obj;
  for (obj = fobj; obj; obj = obj->nobj)
    if (!let || obj->olet == let)
      if (!fits_on_screen(obj->ox, obj->oy))
      return false;
  return true;
}

static void map_animate_char(Short x, Short y, Char c)
{
  if (use_map_mode) map_draw_char(x, y, c);
  else animate_char(y, x, c, false);
}
