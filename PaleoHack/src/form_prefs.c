/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
//#include "lock-externs.h"
#include "paleohackRsc.h"

#ifdef I_AM_COLOR
//extern Boolean found_color;
Boolean found_color = false;
#endif

extern Boolean took_time;

static void init_checkboxes_1() SEC_5;
static void init_checkboxes_2() SEC_5;
static void init_lists(Short rw, Short ws) SEC_5;
static void show_box_color(Boolean show) SEC_5;
static Boolean prefs_save_lists(Short rw, Short ws) SEC_5;
static Boolean prefs_save_checkboxes_1() SEC_5;
static Boolean prefs_save_checkboxes_2() SEC_5;
static void prefs_form_save_prefs(Short rw, Short ws) SEC_5;


Boolean Prefs_Form_HandleEvent(EventPtr e)
{
  Boolean handled = false;
  FormPtr frm;
  static Short rw = 0;
  static Short ws = 0;
    
  switch (e->eType) {

  case frmOpenEvent:
    frm = FrmGetActiveForm();
    rw = max(1, my_prefs.run_walk_border - my_prefs.walk_center_border);
    ws = max(1, my_prefs.walk_center_border);
    init_checkboxes_1();
    init_checkboxes_2();
    FrmDrawForm(frm);
    init_lists(rw, ws);
    show_box_color(true);
    handled = true;
    break;

  case ctlSelectEvent:
    switch(e->data.ctlSelect.controlID) {
    case btn_prefs_ok:
      prefs_form_save_prefs(rw, ws);
      handled = true;
      break;
    case btn_prefs_cancel:
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



static void show_box_color(Boolean show)
{
  FormPtr frm;
  ControlPtr checkbox;
  frm = FrmGetActiveForm();
  checkbox = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, check_prf_14));
#ifdef I_AM_COLOR
  if (found_color && show)
    CtlShowControl(checkbox);
  else
#endif
    CtlHideControl(checkbox);
}

static void init_lists(Short rw, Short ws)
{
  FormPtr frm;
  ListPtr lst;
  frm = FrmGetActiveForm();
  /* set initial settings for lists */
  lst = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, list_prf_1));
  LstSetSelection(lst, ws-1);
  lst = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, list_prf_2));
  LstSetSelection(lst, rw-1);
}

static void init_checkboxes_1()
{
  FormPtr frm;
  ControlPtr checkbox;
  frm = FrmGetActiveForm();

  checkbox = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, check_prf_4));
  CtlSetValue(checkbox, (my_prefs.sound ? 1 : 0));
  checkbox = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, check_prf_13));
  CtlSetValue(checkbox, (my_prefs.black_bg ? 1 : 0));
  checkbox = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, check_prf_14));
  CtlSetValue(checkbox, (my_prefs.color_on ? 1 : 0));
}
static void init_checkboxes_2()
{
  FormPtr frm;
  ControlPtr checkbox;
  frm = FrmGetActiveForm();
  checkbox = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, check_prf_2));
  CtlSetValue(checkbox, (my_prefs.relative_move ? 1 : 0));
}


static Boolean prefs_save_lists(Short rw, Short ws)
{
  Boolean dirty = false;
  if (my_prefs.overlay_circles &&
      (my_prefs.run_walk_border != rw+ws ||
       my_prefs.walk_center_border != ws))
    dirty = true; // need to erase old circles
  my_prefs.run_walk_border = rw+ws;
  my_prefs.walk_center_border = ws;
  return dirty;
}

static Boolean prefs_save_checkboxes_1()
{
  FormPtr frm;
  ControlPtr checkbox;
  Boolean val, dirty = false;
  frm = FrmGetActiveForm();

  checkbox = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, check_prf_4));
  my_prefs.sound = (CtlGetValue(checkbox) != 0);

  // Inverted background:
  checkbox = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, check_prf_13));
  val = (CtlGetValue(checkbox) != 0);
  if (my_prefs.black_bg != val)
    dirty = true;
  my_prefs.black_bg = val;

  // Color:
  checkbox = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, check_prf_14));
  val = (CtlGetValue(checkbox) != 0);
  if (my_prefs.color_on != val)
    dirty = true;
  my_prefs.color_on = val;
  return dirty;
}

static Boolean prefs_save_checkboxes_2()
{
  FormPtr frm;
  ControlPtr checkbox;
  Boolean val, dirty = false;
  frm = FrmGetActiveForm();

  checkbox = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, check_prf_2));
  val = (CtlGetValue(checkbox) != 0); // decide to erase old circles
  if (my_prefs.overlay_circles && (val != my_prefs.relative_move))
    dirty = true;
  my_prefs.relative_move = val;

  checkbox = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, check_prf_3));
  val = (CtlGetValue(checkbox) != 0); // decide to draw or erase circles
  if (my_prefs.overlay_circles != val)
    dirty = true;
  my_prefs.overlay_circles = val;

  return dirty;
}

void move_visible_window(Short left_x, Short top_y, Boolean center);//display.c
void clear_visible();//display.c
static void prefs_form_save_prefs(Short rw, Short ws)
{
  Boolean dirty, verydirty;
  dirty = prefs_save_lists(rw, ws);
  verydirty = prefs_save_checkboxes_1();
  dirty = prefs_save_checkboxes_2() || dirty;

  writePrefs();
  LeaveForm();
  if (verydirty) { // clear-and-redraw absolutely everything
    clear_visible();
    move_visible_window(you.ux, you.uy, true);
    show_messages();
    refresh();
    flags.botl = BOTL_ALL;
    print_stats(0);
  } else if (dirty) { // don't need to redraw msgs or stats
    // ?
    move_visible_window(you.ux, you.uy, true);
    refresh();
  }

}
