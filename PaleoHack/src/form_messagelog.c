/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
#include "paleohackRsc.h"

Short msglog_mode;

extern Char *old_messages[SAVED_MSGS];
VoidHand msglog_TextHandle = NULL;
static void init_messagelog_view(FormPtr frm) SEC_5;
static Short get_info_size() SEC_5;
static Short write_info(Char *txtP) SEC_5;
static void draw_messagelog(FormPtr frm) SEC_5;
static void cleanup_help() SEC_5;

// Note:  In main.c I've blocked all hardware buttons (including power)
// from operation in messagelog form - this is because when you
// exit the form by leaving the application, it doesn't do the
// right thing for the SHOW_LHELP or SHOW_SHELP case - it crashes.)

Boolean MsgLog_Form_HandleEvent(EventPtr e)
{
  Boolean handled = false;
  FormPtr frm;
  FieldPtr fld;
    
  switch (e->eType) {

  case frmOpenEvent:
    frm = FrmGetActiveForm();
    init_messagelog_view(frm);
    FrmDrawForm(frm);
    draw_messagelog(frm);
    handled = true;
    break;

  case ctlSelectEvent:
    switch(e->data.ctlSelect.controlID) {
    case btn_ml_ok:
      if (msglog_mode == SHOW_LHELP || msglog_mode == SHOW_SHELP)
	cleanup_help();
      LeaveForm(); // LeaveForm(true);
      if (msglog_mode == SHOW_DEAD) init_topten();
      handled = true;
      break;
    }
    break;

  case ctlRepeatEvent:
    /*     "Repeating controls don't repeat if handled is set true." */
    frm = FrmGetActiveForm();
    fld = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, field_ml));
    switch(e->data.ctlRepeat.controlID) {
    case repeat_ml_up:
      FldScrollField(fld, 1, winUp);
      update_field_scrollers(frm, fld, repeat_ml_up, repeat_ml_down);
      break;
    case repeat_ml_down:
      FldScrollField(fld, 1, winDown);
      update_field_scrollers(frm, fld, repeat_ml_up, repeat_ml_down);
      break;
    }
    break;

  case keyDownEvent:
    /* hardware button -- or else graffiti. */
    frm = FrmGetActiveForm();
    fld = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, field_ml));
    switch(e->data.keyDown.chr) {
    case pageUpChr:
      page_scroll_field(frm, fld, winUp);
      update_field_scrollers(frm, fld, repeat_ml_up, repeat_ml_down);
      handled = true;
      break;
    case pageDownChr: case ' ':
      page_scroll_field(frm, fld, winDown);
      update_field_scrollers(frm, fld, repeat_ml_up, repeat_ml_down);
      handled = true;
      break;
    case '\n': // Keyboard support.  Woo hoo.
      hit_button_if_usable(frm, btn_ml_ok);
      handled = true;
      break;
    }
    break;

  default:
    break;
  }

  return handled;
}



/**********************************************************************
                       INIT_MESSAGELOG_VIEW
 IN: frm = a form that has a text-view widget
 PURPOSE:
 Initializes the displayed form 'frm' with a list of the last
 SAVED_MSGS messages.  Called when frm is being initialized.
 **********************************************************************/
static const Char *msglog_title[6] = {"Message Log", "Discoveries", "Goodbye",
				      "Long Help", "Short Help", "Bug"};
static void init_messagelog_view(FormPtr frm)
{
  FieldPtr fld;
  CharPtr txtP;
  Short len = 10; // some extra space to be sure
  // Get the text field
  fld = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, field_ml));
  if (msglog_mode == SHOW_LHELP) {
    msglog_TextHandle = DmGetResource(strRsc, Help1Str);
  } else if (msglog_mode == SHOW_SHELP) {
    msglog_TextHandle = DmGetResource(strRsc, Help2Str);
  } else {
    // Create a mem. handle and lock it
    len += get_info_size();
    msglog_TextHandle = MemHandleNew(len);
    txtP = MemHandleLock(msglog_TextHandle);
    len = write_info(txtP);
    // Unlock the handle.  Set the field to display the handle's text.
    MemHandleUnlock(msglog_TextHandle);
  }
  FldSetTextHandle(fld, (Handle) msglog_TextHandle);
  FrmCopyTitle(frm, msglog_title[msglog_mode]);
}



// Return size of text handle needed
static Short get_info_size()
{
  Short i, len = 0, items;
  switch(msglog_mode) {
  case SHOW_MSGLOG:
    for (i = 0, len = 0 ; i < SAVED_MSGS ; i++)
      len += StrLen(old_messages[i]);
    len++;
    break;
  case SHOW_DISCOVERED:
    for (i = 0, items = 0 ; i < MAX_OBJCLASS ; i++) {
      if (interesting_to_discover(i)) {
	items++;
	len += typename_len(i);
      }
    }
    if (!items) len = 40; //enough space for the "none" message
    break;
  case SHOW_DEAD:
    len = done_postRIP_size();
    break;
  default:
    len = 1;
    break;
  }
  return len;
}

// Write random info to ri_TextHandle.
// Return number of chars written.  (not including \0)
static Short write_info(Char *txtP)
{
  Short i, len = 0, items;
  Char *buf;
  switch(msglog_mode) {
  case SHOW_MSGLOG:
    // Use MemMove and/or MemSet to copy text to the mem. handle
    for (i = 0, len = 0 ; i < SAVED_MSGS ; i++) {
      MemMove(txtP+len, old_messages[i], StrLen(old_messages[i])+1);
      len += StrLen(old_messages[i]);
    }
    break;
  case SHOW_DISCOVERED:
    txtP[0] = '\0';
    for (i = 0, len = 0, items = 0 ; i < MAX_OBJCLASS ; i++) {
      if (interesting_to_discover(i)) {
	items++;
	buf = typename(i);
	MemMove(txtP+len, buf, StrLen(buf)+1);
	len += StrLen(buf);
      }
    }
    if (!items) {
      StrPrintF(txtP, "You haven't discovered anything yet...");
      len = StrLen(txtP);
    }
    break;
  case SHOW_DEAD:
    done_postRIP(txtP);
    len = StrLen(txtP);
    break;
  default:
    txtP[0] = '\0';
    len = 1;
    break;
  }
  // this is to prevent a trailing newline:
  if ((len > 0) && (txtP[len-1] == '\n')) MemSet(txtP+len-1, 1, '\0');
  return len;
}


static void draw_messagelog(FormPtr frm)
{
  FieldPtr fld;
  fld = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, field_ml));
  switch(msglog_mode) {
  case SHOW_MSGLOG:
    while (FldScrollable(fld, winDown))
      page_scroll_field(frm, fld, winDown);
    break;
  default:
    break;
  }
  update_field_scrollers(frm, fld, repeat_ml_up, repeat_ml_down);
}

static void cleanup_help()
{
  FormPtr frm;
  FieldPtr fld;
  frm = FrmGetActiveForm();
  fld = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, field_ml));
  FldSetTextHandle(fld, NULL); // will that work?
}
