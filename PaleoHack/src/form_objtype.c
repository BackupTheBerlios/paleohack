/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
#include "paleohackRsc.h"
#include "bit.h"

// NOTE.  this string MUST match the order of the buttons in the form.
Char droppable_symbols[NUM_OBJ_SYMBOLS+1] = "$!%?[()=*/\"0ua";
// You can't drop the chain, 'cause you can't pick it up either.  I think.
// (droppable_symbols = obj_symbols - BALL_SYM + 'u' + 'a', roughly.)
Boolean droppable[NUM_OBJ_SYMBOLS];
Boolean selected[NUM_OBJ_SYMBOLS];

Boolean drop_not_identify = false;
static void ggetobj_start();// SEC_5;
Short ggetobj_end(Char *olets, Boolean drop_not_identify,
		  Boolean allflag, Boolean unpaidflag); // XXX

// Replacement for ggetobj!

Boolean ObjType_Form_HandleEvent(EventPtr e)
{
  Boolean handled = false;
  FormPtr frm;
  ControlPtr pushbtn;
  Short btn_i, i, j, total;
  Char *tmp, selected_symbols[NUM_OBJ_SYMBOLS+1];
    
  switch (e->eType) {

  case frmOpenEvent:
    frm = FrmGetActiveForm();
    if (drop_not_identify)
      FrmCopyTitle(frm, "Select Types to Drop");
    FrmDrawForm(frm);
    ggetobj_start(); // initialize 'droppable'.
    for (btn_i = pbtn_ot_0 ; btn_i <= pbtn_ot_MAX ; btn_i++) {
      pushbtn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, btn_i));
      CtlSetValue(pushbtn, false);
      if (droppable[btn_i - pbtn_ot_0])	CtlShowControl(pushbtn);
      else CtlHideControl(pushbtn);
      selected[btn_i - pbtn_ot_0] = false;
    }
    handled = true;
    break;

  case ctlSelectEvent:
    switch(e->data.ctlSelect.controlID) {
    case btn_ot_ok:
      for (i = 0 ; i < NUM_OBJ_SYMBOLS-1 ; i++)
	selected_symbols[i] = '\0';
      for (i = 0, j = 0 ; i < NUM_OBJ_SYMBOLS-2 ; i++) {
	// leave out 'a' and 'u' ...
	if (droppable[i] && selected[i]) {
	  selected_symbols[j] = droppable_symbols[i];
	  selected_symbols[++j] = '\0';
	}
      }
      LeaveForm();
      total = ggetobj_end(selected_symbols, drop_not_identify,
	  (droppable[NUM_OBJ_SYMBOLS-1] && selected[NUM_OBJ_SYMBOLS-1]), // a
	  (droppable[NUM_OBJ_SYMBOLS-2] && selected[NUM_OBJ_SYMBOLS-2]) ); // u
      if (total) {
	extern Boolean took_time;
	void end_turn_start_turn(); // in main.c
	took_time = true;
	end_turn_start_turn();
      } else if (!drop_not_identify) {
	// User didn't want to ID any of this 'type', give them another
	// chance until they ID something.  (this is proper behavior.)
	drop_not_identify = false;
	FrmPopupForm(ObjTypeForm);
      }
      handled = true;
      break;
    case btn_ot_cancel:
      LeaveForm();
      if (!drop_not_identify) {
	// Actually I should not allow you to cancel an identify at all.
	extern Boolean took_time;
	void end_turn_start_turn(); // in main.c
	took_time = true;
	end_turn_start_turn();
      }
      handled = true;
      break;
    default:
      btn_i = e->data.ctlSelect.controlID;
      if (btn_i >= pbtn_ot_0 && btn_i <= pbtn_ot_MAX) {
	i = btn_i - pbtn_ot_0;
	if (droppable[i]) { // Toggle...
	  frm = FrmGetActiveForm();
	  pushbtn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, btn_i));
	  selected[i] = !selected[i];
	  CtlSetValue(pushbtn, selected[i]);
	}
	handled = true;
      }
      break;
    }
    break;

  case keyDownEvent:
    frm = FrmGetActiveForm();
    switch(e->data.keyDown.chr) {
    case '\n':
      hit_button_if_usable(frm, btn_ot_ok);
      handled = true;
      break;
    case 27:
      hit_button_if_usable(frm, btn_ot_cancel);
      handled = true;
      break;
    default:
      tmp = StrChr(droppable_symbols, e->data.keyDown.chr);
      if (tmp) {
	i = tmp - droppable_symbols;
	pushbtn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, pbtn_ot_0 + i));
	selected[i] = !selected[i];
	CtlSetValue(pushbtn, selected[i]);
	handled = true;
      }
      break;
    }
    break;

  default:
    break;
  }
  return handled;
}


static void ggetobj_start()
{
  Boolean allowgold = (you.ugold && drop_not_identify);
  Short i;
  if (!invent && !allowgold) {
    // Caller should have checked that already.  But just in case.
    StrPrintF(ScratchBuffer, "You have nothing to %s.",
	      (drop_not_identify ? "drop" : "identify"));
    message(ScratchBuffer);
    LeaveForm();
  } else {
    obj_t *otmp = invent;

    for (i = 0 ; i < NUM_OBJ_SYMBOLS ; i++)
      droppable[i] = false;
    if (invent) droppable[NUM_OBJ_SYMBOLS-1] = true;
    if (allowgold) droppable[0] = true; // ilets[iletct++] = '$';
    while (otmp) {
      Char *tmp = StrChr(droppable_symbols, otmp->olet);
      Short i;
      if (!tmp) continue;
      i = tmp - droppable_symbols;
      droppable[i] = true;
      if (otmp->bitflags & O_IS_UNPAID) droppable[NUM_OBJ_SYMBOLS-2] = true;
      otmp = otmp->nobj;
    }
  }
}
