/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
//#include "lock-externs.h"
#include "paleohackRsc.h"

you_t you;
extern Char plname[PL_NSIZ]; // shk.c
static void chargen_init_name(FormPtr frm) SEC_5;
static void chargen_commit_name(FormPtr frm) SEC_5;
static void init_by_class() SEC_2;
static void maybe_frob_qty(Short ti, Short trobj_i, Short *qty) SEC_2;
static void init_inv(Short ti) SEC_2;
static void zero_you() SEC_2;


// Need a way to change cave-person to -man or -woman...
// these are the same as appear on the buttons.
// actually I know how to change button labels ok.
Char char_class_names[MAX_CLASS][13] = {
  "Tourist",
  "Speleologist",
  "Fighter",
  "Knight",
  "Cave-person",
  "Wizard"
};

Boolean came_from_generation = false;
Boolean Chargen1_Form_HandleEvent(EventPtr e)
{
  Boolean handled = false;
  FormPtr frm;
  ControlPtr btn, pushbtn;
  //  Short btn_i;
    
  switch (e->eType) {

  case frmOpenEvent:
    you.dead = true; // fix the "exit before creating first character" issue
    frm = FrmGetActiveForm();
    chargen_init_name(frm);
    FrmDrawForm(frm);
    pushbtn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, pbtn_gender_0));
    CtlSetValue(pushbtn, my_prefs.is_male);
    pushbtn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, pbtn_gender_1));
    CtlSetValue(pushbtn, !(my_prefs.is_male));
    came_from_generation = true;
    handled = true;
    break;

  case ctlSelectEvent:
    switch(e->data.ctlSelect.controlID) {
    case btn_cg1_yes:
      chargen_commit_name(FrmGetActiveForm());
      FrmGotoForm(Chargen2Form);
      handled = true;
      break;
    case btn_cg1_no:
      frm = FrmGetActiveForm();
      btn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, btn_cg1_no));
      CtlHideControl(btn);
      btn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, btn_cg1_yes));
      CtlHideControl(btn);
      btn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, btn_cg1_ok));
      CtlShowControl(btn);
      you.character_class = rund(MAX_CLASS);
      {
	Char buf[80];
	StrPrintF(buf, "I'll choose a character for you.");
	WinDrawChars(buf, StrLen(buf), 5, 40+11+11);
	StrPrintF(buf, "This game you will be a %s.",
		  char_class_names[you.character_class]);
	// XXX.  I should read the prefs, to get gender,
	// and switch "person" to "man" or "woman" each time I
	// use char_class_names.  other gender spots: get_default_username
	WinDrawChars(buf, StrLen(buf), 5, 40+11+11+11);
      }      
      handled = true;
      break;
    case btn_cg1_ok:
      chargen_commit_name(FrmGetActiveForm());
      FrmGotoForm(MainForm);
      handled = true;
      break;
    case pbtn_gender_0:
      my_prefs.is_male = true;
      handled = true;
      break;
    case pbtn_gender_1:
      my_prefs.is_male = false;
      handled = true;
      break;
    }
    break;

    /*
  case keyDownEvent:
    btn_i = -1;
    switch(e->data.keyDown.chr) {
    case 'y': case 'Y':
      btn_i = btn_cg1_yes;
      break;
    case 'n': case 'N':
      btn_i = btn_cg1_no;
      break;
    case '\n':
      btn_i = btn_cg1_ok;
      break;
    }
    if (btn_i != -1) {
      frm = FrmGetActiveForm();
      btn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, btn_i));
      if (btn->attr.usable)
	CtlHitControl(btn);
    }
    handled = true;
    break;
    */

  default:
    break;
  }
  return handled;
}

static void chargen_init_name(FormPtr frm)
{
  FieldPtr fld;
  CharPtr p;
  VoidHand vh;

  if (!my_prefs.name[0])
    get_default_username(my_prefs.name, PL_NSIZ);

  fld = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, field_charname));
  vh = (VoidHand) FldGetTextHandle(fld);
  if (!vh) {
    vh = MemHandleNew(PL_NSIZ * sizeof(Char));
  }
  FldSetTextHandle(fld, (Handle) 0);
  p = MemHandleLock(vh);

  StrNCopy(p, my_prefs.name, PL_NSIZ);
  p[PL_NSIZ-1] = '\0';
  MemHandleUnlock(vh);
  FldSetTextHandle(fld, (Handle) vh);
}
static void chargen_commit_name(FormPtr frm)
{
  FieldPtr fld;
  CharPtr p;
  VoidHand vh;
  fld = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, field_charname));
  vh = (VoidHand) FldGetTextHandle(fld);
  if (vh) {
    p = MemHandleLock(vh);
    if (p && p[0])
      StrNCopy(my_prefs.name, p, PL_NSIZ);
    MemHandleUnlock(vh);
    my_prefs.name[PL_NSIZ-1] = '\0';
  }
  writePrefs();
}

Boolean Chargen2_Form_HandleEvent(EventPtr e)
{
  Boolean handled = false;
  FormPtr frm;
  Char c;
  Short i;
    
  switch (e->eType) {

  case frmOpenEvent:
    frm = FrmGetActiveForm();
    FrmDrawForm(frm);
    came_from_generation = true;
    handled = true;
    break;

  case ctlSelectEvent:
    switch(e->data.ctlSelect.controlID) {
    case btn_cg2_1:
    case btn_cg2_2:
    case btn_cg2_3:
    case btn_cg2_4:
    case btn_cg2_5:
    case btn_cg2_6:
      you.character_class = e->data.ctlSelect.controlID - btn_cg2_1;
      FrmGotoForm(MainForm);
      handled = true;
      break;
    }
    break;

  case keyDownEvent:
    c = e->data.keyDown.chr;
    if (c > 'Z') c -= ('a' - 'A');
    for (i = 0 ; i < MAX_CLASS ; i++) {
      if (c == char_class_names[i][0]) {
	you.character_class = i;
	FrmGotoForm(MainForm);
	break;
      }
    }
    handled = true;
    break;

  default:
    break;
  }
  return handled;
}


// 0 tourist, 1 speleologist, 2 fighter, 3 knight, 4 cave-dude, 5 wizard
#define TINOPENER 6
static void init_by_class()
{
  you.usym = '@';
  you.ulevel = 1;
  dlevel = maxdlevel = 1;
  you.uhunger = 900;
  you.uhs = NOT_HUNGRY;
  flags.beginner = true;
  // xxx Also:  uarm=uarm2=uarmh=uarms=uarmg=uwep=uball=uchain=uleft=uright=0;
  switch(you.character_class) {
  case CLASS_TOURIST:
    //    Tourist[3].trquan = 20 + rnd(20); // see maybe_frob_qty
    you.ugold = you.ugold0 = rnd(1000);
    you.uhp = you.uhpmax = 10;
    you.ustr = you.ustrmax = 8;
    init_inv(you.character_class);
    if (!rund(25)) init_inv(TINOPENER);
    break;
  case CLASS_SPELEOLOGIST:
    //    Fast = INTRINSIC;
    //    Stealth = INTRINSIC;
    you.uhp = you.uhpmax = 12;
    you.ustr = you.ustrmax = 10;
    init_inv(you.character_class);
    if (!rund(10)) init_inv(TINOPENER);
    break;
  case CLASS_FIGHTER:
    you.uhp = you.uhpmax = 12;
    you.ustr = you.ustrmax = 10;
    init_inv(you.character_class);
    break;
  case CLASS_KNIGHT:
    you.uhp = you.uhpmax = 14;
    you.ustr = you.ustrmax = 17;
    init_inv(you.character_class);
    break;
  case CLASS_CAVEPERSON:
    //    Cave_man[2].trquan = 12 + rnd(9)*rnd(9);  // see maybe_frob_qty
    you.uhp = you.uhpmax = 16;
    you.ustr = you.ustrmax = 18;
    init_inv(you.character_class);
    break;
  case CLASS_WIZARD:
    //    for (i=1; i<=4; i++) if (!rund(5))
    //      Wizard[i].trquan += rund(3) - 1;  // see maybe_frob_qty
    you.uhp = you.uhpmax = 15;
    you.ustr = you.ustrmax = 16;
    init_inv(you.character_class);
    break;
  default:      /* should be impossible */
    you.uhp = you.uhpmax = 12;
    you.ustr = you.ustrmax = 16;
    break;
  }
  find_ac();
  if (!rund(20)) {
    Short d = rund(7) - 2;       /* biased variation */
    you.ustr += d;
    you.ustrmax += d;
  }

  /* make sure he can carry all he has - especially for T's */
  while (inv_weight() > 0 && you.ustr < 118) {
    you.ustr++;
    you.ustrmax++;
  }
}

static void maybe_frob_qty(Short ti, Short trobj_i, Short *qty)
{
  switch(you.character_class) {
  case 0:
    if (trobj_i == 3)
      *qty = 20 + rnd(20);
    break;
  case 4:
    if (trobj_i == 2)
      *qty = 12 + rnd(9) * rnd(9);
    break;
  case 5:
    if ((trobj_i >= 1 && trobj_i <= 4) && !rund(5))
      *qty += rund(3) - 1;
    break;
  }
}

// for initializing inventory:
#define        UNDEF_TYP       0
#define        UNDEF_SPE       '\177'
typedef struct {
  UChar trotyp;
  Int8 trspe;
  Char trolet;
  UChar quan_known; // 6 bits low, 1 bit high
} trobj_t;
// Initialize your inventory
extern Short *trobj_start;
static void init_inv(Short ti)
{
  obj_t *obj;
  Short i, qty;
  Short trobj_index, trobj_end;
  trobj_t *trop;

  // the record is a little funky.  we have a short, N.  then we have N
  // shorts.  then we have an array (into which they are indices).
  Short num_inven_kinds;
  Short * stmp = trobj_start;
  num_inven_kinds = stmp[0];
  stmp++;
  trobj_index = stmp[ti];
  trobj_end = stmp[ti+1];
  stmp += num_inven_kinds; // hmmm
  trop = (trobj_t *) stmp;

  for (i = trobj_index ; i < trobj_end ; i++) {
    // read stuff in trop[i].  make sure that we don't WRITE to trop[i].
    qty = trop[i].quan_known & 0x3f; // (get rid of high 2 bits)
    maybe_frob_qty(ti, i - trobj_index, &qty);
    do {
      /*
      {
	Char buf[40];
	StrPrintF(buf, "%d %d %c %x", trop[i].trotyp, trop[i].trspe,
		  trop[i].trolet, trop[i].quan_known);
	WinDrawChars(buf, StrLen(buf), 5, 10+11*(i - trobj_index));
      }
      */
      obj = mkobj(trop[i].trolet);
      if (trop[i].quan_known & 0x80) // "known"
	obj->bitflags |= O_IS_KNOWN;

      //      obj->bitflags |= O_IS_KNOWN; // remove! this is just for debugging!
      //      obj->bitflags |= O_IS_DESCKNOWN; // remove! this is just for debugging!
      // hm, doesn't work... oh yeah.  nameknown_merge thingy.  nuts.

      obj->bitflags &= ~O_IS_CURSED;
      // If it's a weapon, make it "all" and make sure we'll exit the loop.
      if (obj->olet == WEAPON_SYM) {
	obj->quantity = qty;
	qty = 1;
      }
      if (trop[i].trspe != UNDEF_SPE)
	obj->spe = trop[i].trspe;
      if (trop[i].trotyp != UNDEF_TYP)
	obj->otype = trop[i].trotyp;
      else
	if (obj->otype == WAN_WISHING)	/* gitpyr!robert */
	  obj->otype = WAN_DEATH;
      obj->owt = weight(obj);	/* defined after setting otyp+quan */
      obj = addinv(obj);
      if (obj->olet == ARMOR_SYM) {
	switch(obj->otype) {
	case SHIELD:         if (!uarms) setworn(obj, W_ARMS);  break;
	case HELMET:         if (!uarmh) setworn(obj, W_ARMH);  break;
	case PAIR_OF_GLOVES: if (!uarmg) setworn(obj, W_ARMG);  break;
	case ELVEN_CLOAK:    if (!uarm2) setworn(obj, W_ARM);   break;
	default:             if (!uarm)  setworn(obj, W_ARM);   break;
	}
      }
      if ((obj->olet == WEAPON_SYM) && (!uwep))
	setuwep(obj);
      qty--;
    } while (qty > 0);
  }
}





extern Short multi; // living in movesee.c right now..
void greet_player()
{
  StrNCopy(plname, my_prefs.name, PL_NSIZ);
  if (!plname[0])
    get_default_username(plname, PL_NSIZ);
  plname[PL_NSIZ-1] = '\0';

  preempt_messages();
  StrPrintF(ScratchBuffer, "Hello %s, welcome to hack!", plname);
  message(ScratchBuffer);
  flags.botl = BOTL_ALL;
  multi = 0;
}
// also, decide whether to print "You are lucky! Full moon tonight."
void moon_player()
{
  flags.moon_phase = phase_of_the_moon();
  if (flags.moon_phase == FULL_MOON) {
    message("You are lucky!  Full moon tonight.");
    you.uluck++;
  } else if (flags.moon_phase == NEW_MOON)
    message("Be careful!  New moon tonight.");
}

// the lazy way to zero it (I have zeroing in md_malloc)
static void zero_you()
{
  you_t *zyou;
  zyou = (you_t *) md_malloc(sizeof(you_t));
  you = *zyou;
  // hmmm need to free it.
  MemPtrFree(zyou);
  // other things need to be zeroed also:
  moves = 0;
  // also I should "erase" old messages....
}



void init_player()
{
  Int8 tmp = you.character_class;
  zero_you(); // theoretically this should work.
  you.character_class = tmp;

  // I really should seed the random number generator, too:
  you.birthdate = TimGetSeconds();
  SysRandom(you.birthdate); // heh reproducible up to a save/restore.

  // hmmm, should make sure to free these when a character is killed/quit:
  fobj = fcobj = invent = NULL;//list of obj_t.   while we're at it, let's add:
  uwep = uarm=uarm2 = uarmh = uarms=uarmg = uright=uleft = uchain=uball = NULL;
  fmon = fallen_down = mydogs = NULL;    // list of monst_t
  ftrap = NULL;                 // list of trap_t
  fgold = NULL;                 // list of gold_t

  flags.ident = 1; // initialize the "monster uid" counter

  init_objects(); // this is not fully implemented yet: won't randomize "descr"
  // (isn't it, now, though?)

  init_by_class();// u_init(); -- make sure we are not missing any of this now!
  mklev(); // need to DEBUG: this has "impossible mkobj" sometimes...
  you.ux = xupstair;
  you.uy = yupstair;
  /*
  inshop(); // shk.c
  */
  setsee();
  flags.botl = BOTL_ALL; // "print a fresh status line".  keen.

  makedog(); // dog.c.  Comment this to get rid of da puppy.

  // We've placed you at the stairs; if there's a monster there already,
  // shove it aside so that you can be there.
  { monst_t *mtmp;
  if ((mtmp = mon_at(you.ux, you.uy))) mnexto(mtmp);        // riv05!a3
  }
  seemons(); // pri.c
  /*
  docrt(); // This has been replaced by refresh() which is called in just a bit
  */
  get_default_username(plname, PL_NSIZ);
  greet_player();
  /*
  pickup(1); // hack.c
  read_engr_at(u.ux,u.uy); // engrave.c
  flags.move = 1;
  */
}
