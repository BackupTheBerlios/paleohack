/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
#include "paleohackRsc.h"

/* Form for inscribing an already selected object. 
   Note that inscribing should not take a turn whether cancelled or not. */
extern previous_state curr_state;
extern Int8 engrave_type;

static void init_engrave_fld(FormPtr frm) SEC_5;
static void commit_engrave_fld(FormPtr frm) SEC_5;
static void do_wish(Char *buf) SEC_5;

#define ROW_1 15
#define SCR_WIDTH 160
#define LINE_HEIGHT 11


// we expect this to be ACT_ENGRAVE, ACT_CALL, or ACT_NAME.  or ACT_CHRISTEN?
Short engrave_or_what;

Boolean Engrave_Form_HandleEvent(EventPtr e)
{
  Boolean handled = false;
  FormPtr frm;
  Char chr;

  switch (e->eType) {

  case frmOpenEvent:
    frm = FrmGetActiveForm();
    /* ...init form... */
    init_engrave_fld(frm);
#ifndef I_AM_OS_2
    //    if (IsVGA) PrvMoveForm(false);
#endif
    FrmDrawForm(frm);
    engrave_draw();
    FrmSetFocus(frm, FrmGetObjectIndex(frm, field_sb));
    handled = true;
    break;

#ifndef I_AM_OS_2
    //  case displayExtentChangedEvent:
    //    if (IsVGA) PrvMoveForm(true);
    //    return true;
#endif

  case keyDownEvent:
    frm = FrmGetActiveForm();
    chr = e->data.keyDown.chr;
    if (chr == '\n') {
      hit_button_if_usable(frm, btn_sb_ok);
      handled = true;
    }
    break;

  case ctlSelectEvent:
    frm = FrmGetActiveForm();
    switch(e->data.ctlSelect.controlID) {
    case btn_sb_ok:
      commit_engrave_fld(frm); // <-- includes LeaveForm
      handled = true;
      break;
    case btn_sb_cancel:
      LeaveForm();
      handled = true;
      break;
    }

  default:
    break;
  }
  return handled;
}

// Clone a scroll/potion/foo, whose "kind" the user is about to "call".
void clone_for_call(obj_t *otmp)
{
  static obj_t tmp_call_obj;
  tmp_call_obj = *otmp;
  tmp_call_obj.quantity = 1;
  do_name(&tmp_call_obj, NULL);//tmp_call_obj->onamelth = 0; Undo name (if any)
  curr_state.item = &tmp_call_obj;
  engrave_type = 0;
  engrave_or_what = ACT_CALL;
}

void engrave_draw()
{
  Char /*buf[160],*/ *p;
  Word wwlen, wwpixels=SCR_WIDTH-4;
  Short x=2, y=ROW_1, lines = 0;
  Char *str;

  // there's room to print up to four lines
  switch (engrave_or_what) {
  case ACT_ENGRAVE:
    StrPrintF(ScratchBuffer, "What do you want to %s on the floor here? ",
	      (engrave_type == ENGRAVE) ? "engrave" :
	      (engrave_type == BURN) ? "burn" : "write");
    break;
  case ACT_NAME:
    // XXX I wonder whether ScratchBuffer is long enough for all names?
    StrPrintF(ScratchBuffer, "What do you want to name %s?",
	      doname(curr_state.item));
    break;
  case ACT_CALL:
    str = xname(curr_state.item);
    StrPrintF(ScratchBuffer, "Call %s %s: ",
	      is_vowel(str[0]) ? "an" : "a", str);
    break;
  case GET_WISH:
    StrPrintF(ScratchBuffer, "You may wish for an object.  What do you want?");
    break;
  case GET_GENOCIDE:
    StrPrintF(ScratchBuffer,
	      "What monster do you want to genocide (Type the letter)?");
    break;
  case GET_VAULT:
    StrPrintF(ScratchBuffer,
	      "Suddenly one of the Vault's guards enters!  \"Hello stranger, who are you?\"");
    break;
  default:
    StrPrintF(ScratchBuffer, "You shouldn't be here! (%d)", engrave_or_what);
    break;
  }
  p = ScratchBuffer;
  wwlen = 0;
  wwpixels -= 2*x; // leave margins
  while (lines < 4 && StrLen(p) > 0) {
    wwlen = FntWordWrap(p, wwpixels);
    WinDrawChars(p, wwlen, x, y+LINE_HEIGHT*lines);
    p += wwlen;
    lines++;
  }
}


static void init_engrave_fld(FormPtr frm)
{
  VoidHand h;
  VoidPtr p;
  FieldPtr fld;
  // allocate memory for field
  // note: the old "do_oname" allowed up to 62 characters (plus \0) for "name".
  // and I think the old "docall" allowed arbitrary length for "call".
  h = MemHandleNew(sizeof(Char) * MAX_ENGR_LEN);
  p = MemHandleLock(h);
  ((Char *)p)[0] = '\0';
  MemPtrUnlock(p);
  fld = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, field_sb));
  FldSetTextHandle(fld, (Handle) h);
  switch (engrave_or_what) {
  case ACT_ENGRAVE:
    StrPrintF(ScratchBuffer, "Engrave");
    break;
  case ACT_NAME:
    StrPrintF(ScratchBuffer, "Name");
    break;
  case ACT_CALL:
    StrPrintF(ScratchBuffer, "Call");
    break;
  case GET_WISH:
    StrPrintF(ScratchBuffer, "Wish");
    break;
  case GET_GENOCIDE:
    StrPrintF(ScratchBuffer, "Genocide");
    break;
  case GET_VAULT:
    StrPrintF(ScratchBuffer, "Vault");
    break;
  default:
    StrPrintF(ScratchBuffer, "BUG");
    break;
  }
  FrmCopyTitle(frm, ScratchBuffer);
}

Boolean monstersym(Char ch) SEC_4; // read.c
static void commit_engrave_fld(FormPtr frm)
{
  FieldPtr fld;
  CharPtr textp;
  Char buf[MAX_ENGR_LEN];
  // Copy contents of field
  fld = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, field_sb));
  textp = FldGetTextPtr(fld); // if (textP) ...
  textp[MAX_ENGR_LEN - 1] = '\0';
  StrCopy(buf, textp);
  if ((engrave_or_what==GET_GENOCIDE) && (!buf || !*buf || !monstersym(*buf))){
    do_feep(400, 9);
    return;
  }
  // Get rid of the field
  FldReleaseFocus(fld);
  FldSetSelection(fld, 0, 0);
  FldFreeMemory(fld);
  LeaveForm();

  switch (engrave_or_what) {
  case ACT_ENGRAVE:
    do_engrave(curr_state.item, buf, engrave_type);
    // it might print some messages, and will take some turns.
    break;
  case ACT_NAME:
    do_name(curr_state.item, buf);
    break;
  case ACT_CALL:
    do_call(curr_state.item, buf);
    break;
  case GET_WISH: // Hmmmmmm...  will 40 chars be enough?
    do_wish(buf);
    break;
  case GET_GENOCIDE:
    do_genocide(buf);
    break;
  case GET_VAULT:
    do_vault(buf);
    break;
  default:
    message("that was a bug");
    break;
  }
}

static void do_wish(Char *buf)
{
  obj_t *otmp;
  otmp = readobjnam(buf);
  otmp = addinv(otmp);
  prinv(otmp); // pump the object name into message
}
