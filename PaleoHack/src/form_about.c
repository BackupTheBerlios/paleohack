/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
//#include "lock-externs.h"
#include "paleohackRsc.h"


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
Boolean Sense_Form_HandleEvent(EventPtr e)
{
  Boolean handled = false;
  FormPtr frm;
  obj_t *obj;
  monst_t *mtmp;
  trap_t *ttmp;
  gold_t *gtmp;
    
  switch (e->eType) {

  case frmOpenEvent:
    frm = FrmGetActiveForm();
    FrmDrawForm(frm);
    print_stats(0);
    switch(sense_what) {
    case SENSE_MONSTERS:
      for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
	if (mtmp->mx > 0)
	  animate_char(mtmp->my, mtmp->mx, mtmp->data->mlet, false);
      message("You sense the presence of monsters.");
      break;
    case SENSE_OBJECTS:
      for (obj = fobj; obj; obj = obj->nobj)
	animate_char(obj->oy, obj->ox, obj->olet, false);
      message("You sense the presence of objects.");
      break;
    case SENSE_GOLD:
      for (gtmp = fgold; gtmp; gtmp = gtmp->ngold)
	animate_char(gtmp->gy, gtmp->gx, GOLD_SYM, false);
      message("You feel very greedy, and sense gold!");
      break;
    case SENSE_GOLD_CONFUSED:
      for (ttmp = ftrap; ttmp; ttmp = ttmp->ntrap)
	animate_char(ttmp->ty, ttmp->tx, GOLD_SYM, false);
      message("You feel very greedy!");
      break;
    case SENSE_FOOD:
      for (obj = fobj; obj; obj = obj->nobj)
	if (obj->olet == FOOD_SYM)
	  animate_char(obj->oy, obj->ox, FOOD_SYM, false);
      message("Your nose tingles and you smell food!");
      break;
    case SENSE_FOOD_CONFUSED:
      for (obj = fobj; obj; obj = obj->nobj)
	if (obj->olet == POTION_SYM)
	  animate_char(obj->oy, obj->ox, FOOD_SYM, false);
      message("Your nose tingles and you smell something!");
      break;
    }
    prme();
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
      finish_do_drink(sense_by_what, false, false);
    else {
      // SENSE_GOLD_CONFUSED: known == false.  all else known==true.
      // SENSE_*_CONFUSED: confused == true.  all else confused==false.
      // hm, seems.. asymmetrical..
      finish_do_scroll(sense_by_what,
		       (sense_what != SENSE_GOLD_CONFUSED),
		       (sense_what == SENSE_GOLD_CONFUSED ||
			sense_what == SENSE_FOOD_CONFUSED));
    }
    sense_by_what = NULL;
    sense_what = SENSE_NONE;

    end_turn_start_turn();// might screw up messages if we pop up engraveform..

    handled = true;
    break;

  default:
    break;
  }
  return handled;
}
