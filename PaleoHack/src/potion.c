/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
#include "paleohackRsc.h"
#include "bit.h"

static void ghost_from_bottle() SEC_2;

#define MAX_BOTTLES 7
Char *bottlenames[] = { // max 6 chars + \0
  "bottle", "phial", "flagon", "carafe", "flask", "jar", "vial"
};

extern Short multi; // living in movesee.c right now..


extern UChar sense_what;// convey info to SenseForm
extern obj_t *sense_by_what;// convey info to SenseForm

// Note: this is a litte different, it will return true
// iff the action is COMPLETE, and false if there was no action OR
// the action is needing to finish up (after leaving SenseForm).
// So if (do_drink) the caller should exit invform _and_ tick,
// otherwise the caller should just exit invform.
// (exiting the invform occurs _before_ calling do_drink..)
// .. Ok, I've modified this again so that it lets the caller distinguish
// between "it worked" and "it worked and I popped up EngraveForm".
tri_val_t do_drink(obj_t *otmp)
{
  obj_t *objs;
  //  monst_t *mtmp;
  Boolean unkn = false, nothing = false, b;
  Short tmp;

  if (!otmp) return NO_OP;
  tmp = oc_descr_offset[otmp->otype];
  if (tmp >= 0 && (0 == StrNCompare("smoky", oc_descrs+tmp, 5)) && !rund(13)) {
    ghost_from_bottle();
    useup(otmp);
    return DONE;
  }
  switch(otmp->otype) {
  case POT_RESTORE_STRENGTH:
    unkn = true;
    message("Wow!  This makes you feel great!");
    if (you.ustr < you.ustrmax) {
      you.ustr = you.ustrmax;
      flags.botl |= BOTL_STR;
    }
    break;
  case POT_BOOZE:
    unkn = true;
    message("Ooph!  This tastes like liquid fire!");
    Confusion += dice(3,8);
    /* the whiskey makes us feel better */
    if (you.uhp < you.uhpmax) losehp(-1, "bottle of whiskey");
    if (!rund(4)) {
      tri_val_t t;
      //      Short coma;
      message("You pass out.");
      multi = -rnd(15);
      spin_multi("You awake with a headache.");
      //      coma = rnd(15); //
      //      do { tick(); } while (--coma > 0);
      //      message("You awake with a headache.");
      // // nomovemsg = "You awake with a headache.";
      t = finish_do_drink(otmp, nothing, unkn); //.... I guess....
      return (t==DONE ? NO_OP : t); // so that we don't take any _more_ turns
    }
    break;
  case POT_INVISIBILITY:
    if (Invis || See_invisible)
      nothing = true;
    else {
      if (!Blind)
	message("Gee!  All of a sudden, you can't see yourself.");
      else {
	message("You feel rather airy.");
	unkn = true;
      }
      newsym(you.ux,you.uy);
    }
    Invis += rund(15)+31;
    break;
  case POT_FRUIT_JUICE:
    message("This tastes like fruit juice.");
    lesshungry(20);
    break;
  case POT_HEALING:
    message("You begin to feel better.");
    you.uhp += rnd(10);
    if (you.uhp > you.uhpmax)
      you.uhp = ++you.uhpmax;
    if (Blind) Blind = 1;	/* you'll see again on the next move */
    if (Sick) Sick = 0;
    flags.botl |= BOTL_HP;
    break;
  case POT_PARALYSIS:
    if (Levitation)
      message("You are motionlessly suspended.");
    else
      message("Your feet are frozen to the floor!");
    nomul(-(rund(10)+25));
    break;
  case POT_MONSTER_DETECTION:
    if (!fmon) {
      b = strange_feeling(otmp, "You feel threatened.");
      return ((b) ? GO_ON : DONE);
    } else {
      sense_what = SENSE_MONSTERS;
      sense_by_what = otmp;
      sense_init_screen();
      FrmPopupForm(SenseForm);
      return GO_ON; // to postpone the tick!
    }
    break;
  case POT_OBJECT_DETECTION:
    if (!fobj) {
      b = strange_feeling(otmp, "You feel a pull downward.");
      return ((b) ? GO_ON : DONE);
    } else {
      for (objs = fobj; objs; objs = objs->nobj)
	if (objs->ox != you.ux || objs->oy != you.uy) {
	  sense_what = SENSE_OBJECTS;
	  sense_by_what = otmp;
	  sense_init_screen();
	  FrmPopupForm(SenseForm);
	  return GO_ON; // to postpone the tick!
	}
      message("You sense the presence of objects close nearby.");
      break;
    }
    break;
  case POT_SICKNESS:
    message("Yech! This stuff tastes like poison.");
    if (Poison_resistance)
      message("(But in fact it was biologically contaminated orange juice.)");
    losestr(rund(4)+3);
    losehp(rnd(10), "contaminated potion");
    flags.botl |= BOTL_STR;
    break;
  case POT_CONFUSION:
    if (!Confusion)
      message("Huh, What?  Where am I?");
    else
      nothing = true;
    Confusion += rund(7)+16;
    break;
  case POT_GAIN_STRENGTH:
    message("Wow do you feel strong!");
    if (you.ustr >= 118) break;	/* > 118 is impossible */
    if (you.ustr > 17) you.ustr += rnd(118-you.ustr);
    else you.ustr++;
    if (you.ustr > you.ustrmax) you.ustrmax = you.ustr;
    flags.botl |= BOTL_STR;
    break;
  case POT_SPEED:
    if (Wounded_legs) {
      heal_legs();
      unkn = true;
      break;
    }
    if (!(Fast & ~INTRINSIC))
      message("You are suddenly moving much faster.");
    else {
      message("Your legs get new energy.");
      unkn = true;
    }
    Fast += rund(10)+100;
    break;
  case POT_BLINDNESS:
    if (!Blind)
      message("A cloud of darkness falls upon you.");
    else
      nothing = true;
    Blind += rund(100)+250;
    seeoff(false);
    break;
  case POT_GAIN_LEVEL: 
    pluslvl();
    break;
  case POT_EXTRA_HEALING:
    message("You feel much better.");
    you.uhp += dice(2,20)+1;
    if (you.uhp > you.uhpmax)
      you.uhp = (you.uhpmax += 2);
    if (Blind) Blind = 1;
    if (Sick) Sick = 0;
    flags.botl |= BOTL_HP;
    break;
  case POT_LEVITATION:
    if (!Levitation)
      float_up();
    else
      nothing = true;
    Levitation += rnd(100);
    //    you.uprops[PROP(RIN_LEVITATION)].p_tofn = float_down; // UNNEEDED,
    // it has been replaced by tweaking timeout.c to call float_down directly!
    break;
  default:
    StrPrintF(ScratchBuffer, "What a funny potion! (%u)", otmp->otype);
    message(ScratchBuffer);
    return NO_OP;
  }
  return finish_do_drink(otmp, nothing, unkn);
}
// 'cause we also need to call this after leaving the SenseForm!
Int8 engrave_type;
extern Short engrave_or_what;
tri_val_t finish_do_drink(obj_t *otmp, Boolean nothing, Boolean unkn)
{
  Boolean go_on = false;
  if (!otmp) return NO_OP; // bug if that happens
  if (nothing) {
    unkn = true;
    message("You have a peculiar feeling for a moment, then it passes.");
  }
  if ((otmp->bitflags & O_IS_DESCKNOWN) &&
      !BITTEST(oc_name_known, otmp->otype)) {
    if (!unkn) {
      BITSET(oc_name_known, otmp->otype);//objects[otmp->otyp].oc_name_known=1;
      more_experienced(0,10);
    } else if (!oc_has_uname(otmp->otype)) {
      // here's what I'll do instead of do_call/docall(otmp) :
      show_all_messages();
      clone_for_call(otmp); // so we don't have to worry about useup!
      FrmPopupForm(EngraveForm);
      go_on = true;
    }
  }
  useup(otmp);
  if (go_on) return GO_ON;
  else       return DONE;
}


// pluslvl has been moved to another file.

// also used for scrolls.
Boolean strange_feeling(obj_t *obj, Char *txt)
{
  Boolean engrave = false;
  if (flags.beginner)
    message("You have a strange feeling for a moment, then it passes.");
  else
    message(txt);
  if (!BITTEST(oc_name_known, obj->otype) &&
      !oc_has_uname(obj->otype)) {
    // here's what I'll do instead of do_call/docall(obj) :
    show_all_messages();
    clone_for_call(obj); // so we don't have to worry about useup!
    FrmPopupForm(EngraveForm);
    engrave = true;
  }
  useup(obj);
  return engrave;
}


// NOTE: To hit "you" with a potion p, call "potionhit(NULL, p)". no "youmonst"
void potionhit(monst_t *mon, obj_t *obj)
{
  Char *botlnam = bottlenames[rund(MAX_BOTTLES)];
  Boolean uclose; // , isyou = (mon==NULL);

  if (!mon) {
    uclose = true;
    StrPrintF(ScratchBuffer, "The %s crashes on your head and breaks into shivers.",
	      botlnam);
    message(ScratchBuffer);
    losehp(rnd(2), "thrown potion");
  } else {
    uclose = (dist(mon->mx,mon->my) < 3);
    /* perhaps 'E' and 'a' have no head? */
    StrPrintF(ScratchBuffer, "The %s crashes on %s's head and breaks into shivers.",
	  botlnam, monnam(mon));
    message(ScratchBuffer);
    if (rund(5) && mon->mhp > 1)
      mon->mhp--;
  }
  StrPrintF(ScratchBuffer, "The %s evaporates.", xname(obj));
  message(ScratchBuffer);

  if (mon && !rund(3)) {
    switch(obj->otype) {
    case POT_RESTORE_STRENGTH:
    case POT_GAIN_STRENGTH:
    case POT_HEALING:
    case POT_EXTRA_HEALING:
      if (mon->mhp < mon->mhpmax) {
	mon->mhp = mon->mhpmax;
	StrPrintF(ScratchBuffer, "%s looks sound and hale again!",
		  Monnam(mon));
	message(ScratchBuffer);
      }
      break;
    case POT_SICKNESS:
      if (mon->mhpmax > 3)
	mon->mhpmax /= 2;
      if (mon->mhp > 2)
	mon->mhp /= 2;
      break;
    case POT_CONFUSION:
    case POT_BOOZE:
      mon->bitflags |= M_IS_CONFUSED; // mon->mconf = 1;
      break;
    case POT_INVISIBILITY:
      unpmon(mon);
      mon->bitflags |= M_IS_INVISIBLE; // mon->minvis = 1;
      pmon(mon);
      break;
    case POT_PARALYSIS:
      mon->bitflags |= M_IS_FROZEN; // mon->mfroz = 1;
      break;
    case POT_SPEED:
      mon->mspeed = MFAST;
      break;
    case POT_BLINDNESS:
      mon->mcansee_and_blinded |= 64 + rund(64); // "|="?  "=" to turn off SEE?
      break;
      /*	
	case POT_GAIN_LEVEL:
	case POT_LEVITATION:
	case POT_FRUIT_JUICE:
	case POT_MONSTER_DETECTION:
	case POT_OBJECT_DETECTION:
	break;
      */
    default: break;
    }
  }
  if (uclose && rund(5))
    potionbreathe(obj);
  free_obj(obj, NULL);
}


// Note: The caller must 'free_me(obj)' after potionbreathe returns.
// this is called only here and by do_throw.
void potionbreathe(obj_t *obj)
{
  switch(obj->otype) {
  case POT_RESTORE_STRENGTH:
  case POT_GAIN_STRENGTH:
    if (you.ustr < you.ustrmax) {
      you.ustr++;
      flags.botl |= BOTL_STR;
    }
    break;
  case POT_HEALING:
  case POT_EXTRA_HEALING:
    if (you.uhp < you.uhpmax) {
      you.uhp++;
      flags.botl |= BOTL_HP;
    }
    break;
  case POT_SICKNESS:
    if (you.uhp <= 5) you.uhp = 1;
    else              you.uhp -= 5;
    flags.botl |= BOTL_HP;
    break;
  case POT_CONFUSION:
  case POT_BOOZE:
    if (!Confusion)
      message("You feel somewhat dizzy.");
    Confusion += rnd(5);
    break;
  case POT_INVISIBILITY:
    message("For an instant you couldn't see your right hand.");
    break;
  case POT_PARALYSIS:
    message("Something seems to be holding you.");
    nomul(-rnd(5));
    break;
  case POT_SPEED:
    Fast += rnd(5);
    message("Your knees seem more flexible now.");
    break;
  case POT_BLINDNESS:
    if (!Blind) message("It suddenly gets dark.");
    Blind += rnd(5);
    seeoff(false);
    break;
    /*	
	case POT_GAIN_LEVEL:
	case POT_LEVITATION:
	case POT_FRUIT_JUICE:
	case POT_MONSTER_DETECTION:
	case POT_OBJECT_DETECTION:
	break;
    */
  default: break;
  }
  /* note: no obfree() */
}


/*
 * -- rudimentary -- to do this correctly requires much more work
 * -- all sharp weapons get one or more qualities derived from the potions
 * -- texts on scrolls may be (partially) wiped out; do they become blank?
 * --   or does their effect change, like under Confusion?
 * -- all objects may be made invisible by POT_INVISIBILITY
 * -- If the flask is small, can one dip a large object? Does it magically
 * --   become a jug? Etc.
 */
Boolean do_dip(obj_t *potion, obj_t *obj)
{
  message("Interesting...");
  if (obj->otype == ARROW || 
      obj->otype == DART ||
      obj->otype == CROSSBOW_BOLT) {
    if (potion->otype == POT_SICKNESS) {
      useup(potion);
      if (obj->spe < 7) obj->spe++;	/* %% */
    }
  }
  return true;
}


static void ghost_from_bottle()
{
  monst_t *mtmp;

  mtmp = makemon(PM_GHOST,you.ux,you.uy);
  if (!mtmp) {
    message("This bottle turns out to be empty.");
    return;
  }
  mnexto(mtmp);
  message("As you open the bottle, an enormous ghost emerges!");
  message("You are frightened to death, and unable to move.");
  nomul(-3);
}
