/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
#include "lock-externs.h" // ScratchBuffer.
#include "paleohackRsc.h" // THINGFORM_H
#include "display.h"


#define LineHeight 11 /* xxx 160-based */
#define visible_char_h_itsy 6
extern Short SCR_WIDTH; // 160.  display.c

extern Boolean pending_messages;
extern Char *old_messages[SAVED_MSGS];
extern Short last_old_message_shown;
extern Boolean itsy_on;


static void clear_thingform() SEC_3;
static void show_msg();// SEC_3;
static void calc_space_needed(Char *msg, Short *maxw, Short *lines) SEC_3;
static Short print_descr(Short y, Char *msg) SEC_3;

static void clear_thingform()
{
  RectangleType r;
  Word maxwid= (SCR_WIDTH/20)*19; // xx 160-based.  152.
  RctSetRectangle(&r, 0, 0, maxwid, THINGFORM_H+visible_char_h_itsy); /* x,y, W, and H */
  WinEraseRectangle(&r, 0);
}

static void show_hp_in_inv()
{
  RectangleType r;
  //  Word maxwid= (SCR_WIDTH/20)*19; // xx 160-based.  152.
  Short w, vch, x;
  Char buf[10];
  Boolean large = false; 
  DWord version;
  FtrGet(sysFtrCreator, sysFtrNumROMVersion, &version);
#ifdef I_AM_OS_2
  if (version < 0x03000000l) FntSetFont(ledFont);
  else large = true;
#else
  if (version >= 0x03000000l) FntSetFont(129);
  else large = true;
#endif
  //  w = FntCharsWidth("hp000/000 ", 10);
  StrPrintF(buf, "HP%d/%d", you.uhp, you.uhpmax);
  w = FntCharsWidth(buf, StrLen(buf));
  x = 0; // maxwid-w; right justify
  vch = (large ? LineHeight : visible_char_h_itsy);
  RctSetRectangle(&r, x, THINGFORM_H, w, vch); /* x,y, W, and H */
  WinEraseRectangle(&r, 0);
  WinDrawChars(buf, StrLen(buf), x, THINGFORM_H);
#ifdef I_AM_OS_2
  FntSetFont(stdFont);
#else
  FntSetFont(stdFont);
#endif
}

// All The News That Fits, We Print.
static void show_msg()
{
  Short y = 0, lines_used;
  Word maxwid= (SCR_WIDTH/20)*19; // xx 160-based.  152.
  //  Char buf[20];

  clear_thingform();

  while (last_old_message_shown < SAVED_MSGS-1) {
    lines_used = print_descr(y, old_messages[last_old_message_shown+1]);
    //    StrPrintF(buf, "%d %d", lines_used, last_old_message_shown);
    //    WinDrawChars(buf, StrLen(buf), lines_used*5, y);
    if (lines_used <= 0) break;
    last_old_message_shown++; // why isn't it working?
    y += lines_used * LineHeight;
  }
  if (last_old_message_shown < SAVED_MSGS-1)
    show_a_more(maxwid, y+visible_char_h_itsy, false);
  else
    pending_messages = false;

  show_hp_in_inv();
}

static void calc_space_needed(Char *msg, Short *maxw, Short *lines)
{
  Char *p;
  Word wwlen, maxwid= (SCR_WIDTH/20)*19; // xx 160-based.  152.
  Short w, y = 0;

  p = msg;
  *lines = 0;
  *maxw = 0;
  while (y < THINGFORM_H) {
    wwlen = FntWordWrap(p, maxwid);
    w = FntCharsWidth(p, wwlen);
    if (w > *maxw) *maxw = w;
    y += LineHeight;
    (*lines)++;
    if (wwlen >= StrLen(p)) break;
    p += wwlen;
  }
}

static Short print_descr(Short y, Char *msg)
{
  Char *p;
  Word wwlen, maxwid= (SCR_WIDTH/20)*19; // xx 160-based.  152.
  Short maxw=0, x, used = 0, lines = 0;
  Short msglen;
  Boolean newline = false;

  msglen = StrLen(msg);
  if (msg[msglen-1] == '\n') {
    msg[msglen-1] = '\0';
    newline = true;
  }

  calc_space_needed(msg, &maxw, &lines);
  if (y + lines * LineHeight <= THINGFORM_H) {

    x = (SCR_WIDTH - maxw) / 2;
    // not going to certer vertically anymore, we might print >1 message.
    //    y = ((y >= THINGFORM_H) ? 0 : (THINGFORM_H-y)/2 );
    p = msg; // rewind...
    while (y < THINGFORM_H) {
      wwlen = FntWordWrap(p, maxwid);
      WinDrawChars(p, wwlen, x, y);
      used++;
      if (wwlen >= StrLen(p)) break; // whew, we printed the whole message
      p += wwlen;
      y += LineHeight;
    }
  } // else not enough space to print it; used = 0.

  if (newline) msg[msglen-1] = '\n';
  
  return used;
}

// This form just displays a message.
// Currently tall enough to fit 4 lines.
Boolean InvMsg_Form_HandleEvent(EventPtr e)
{
  Boolean handled = false;
  FormPtr frm;

  switch (e->eType) {

  case frmOpenEvent:
    frm = FrmGetActiveForm();
    FrmDrawForm(frm);
    show_msg();
    return true;

    // Note: you have to tap WITHIN the form in order to leave the form.
    // or write any character; or (see buttonsHandleEvent) certain hw buttons.
  case keyDownEvent:
    // ignore the key that they pressed to "get here" in case it's held down
    // (a robotfindskittenism; irrelevant for PaleoHack.)
    //       if (e->data.keyDown.chr == last_key) return true;
    // else Fall Through.    
  case penDownEvent:
    if (last_old_message_shown >= SAVED_MSGS-1) LeaveForm(); // no more msgs
    else show_msg();
    //    LeaveForm();
    handled = true;
    break;

  default:
    return true;
  } // end switch event type!
  return handled;
}

