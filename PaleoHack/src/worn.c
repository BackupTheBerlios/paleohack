/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
#include "paleohackRsc.h"

typedef struct {
  Long w_mask;
  struct obj **w_obj;
} worn_t; // size = 8 bytes.

#define MAX_WORN 10
worn_t worn[MAX_WORN] = {
  { W_ARM,   &uarm },
  { W_ARM2,  &uarm2 },
  { W_ARMH,  &uarmh },
  { W_ARMS,  &uarms },
  { W_ARMG,  &uarmg },
  { W_RINGL, &uleft },
  { W_RINGR, &uright },
  { W_WEP,   &uwep },
  { W_BALL,  &uball },
  { W_CHAIN, &uchain }
};

static Boolean cursed(obj_t *otmp) SEC_2;

// move this to the .h
Boolean armoroff(obj_t *otmp); // was in do_wear.c

void setworn(obj_t *obj, Long mask)
{
  obj_t *o_obj;
  Int8 i;

  for (i = 0; i < MAX_WORN; i++) {
    // Find the position that we wanted to wear "obj" in..
    if (worn[i].w_mask & mask) {
      //
      // First, see if we're already wearing something in that position...
      o_obj = *(worn[i].w_obj);
      if (o_obj && !(o_obj->owornmask & worn[i].w_mask)) {
	// We're wearing something but it doesn't know it's being worn: Bug.
	alert_message("DEBUG: inconsistency in setworn");
	return;
      }
      if (o_obj) // Unwear what we're already wearing.
	o_obj->owornmask &= ~worn[i].w_mask;
      //
      // If we were already wearing something in ARM slot, move it to ARM2
      if (obj && o_obj && (worn[i].w_mask == W_ARM)) {
	if (uarm2) {
	  // Hm, ARM2 is full but ARM is empty: Bug.
	  alert_message("DEBUG: uarm2 set?");
	  return;
	} else
	  setworn(uarm, W_ARM2); // whimper..
      }
      //
      // Ok, set the new value of our slot (and make sure obj knows it).
      *(worn[i].w_obj) = obj;
      if (obj) obj->owornmask |= worn[i].w_mask;
      break; // I think we can stop looking for mask now :-)
    }
  }
  // Always fill ARM before ARM2.
  if (uarm2 && !uarm) {
    uarm = uarm2;
    uarm2 = 0;
    uarm->owornmask ^= (W_ARM | W_ARM2);
  }
}



/* called e.g. when obj is destroyed */
// untested?
void setnotworn(obj_t *obj)
{
  worn_t *wp;
  Short i = 0;

  for (wp = worn; i < MAX_WORN && wp->w_mask; wp++, i++)
    if (obj == *(wp->w_obj)) {
      *(wp->w_obj) = NULL;
      obj->owornmask &= ~wp->w_mask;
    }
  if (uarm2 && !uarm) {
    uarm = uarm2;
    uarm2 = NULL;
    uarm->owornmask ^= (W_ARM | W_ARM2);
  }
}


Boolean do_remove_armor(obj_t *otmp) // was in do_wear.c  -  was doremarm()
{
  if (!otmp) return false;
  if (!(otmp->owornmask & (W_ARMOR - W_ARM2))) {
    message("You can't take that off.");
    return false;
  }
  if (otmp == uarmg && uwep &&
      (uwep->bitflags & O_IS_CURSED) ) {	/* myers@uwmacc */
    // "You seem not able to take off the gloves while holding your weapon."
    message("Your (cursed) weapon is in the way.");    // edited for brevity.
    return false;
  }
  armoroff(otmp);
  return true;
}


Boolean do_remove_ring(obj_t *otmp) // was doremring
{
  if (otmp != uright && otmp != uleft) {
    message("But you are not wearing that.");
    return false;
  }
  if (cursed(otmp)) return false;
  ringoff(otmp);
  message("Removed.");//  off_msg(otmp);
  return true;
}


static Boolean cursed(obj_t *otmp)
{
  if (otmp->bitflags & O_IS_CURSED) {
    message("You can't.  It appears to be cursed.");
    return true;
  }
  return false;
}


Boolean armoroff(obj_t *otmp) // was in do_wear.c
{
  Short delay = -objects[otmp->otype].oc_delay;
  if (cursed(otmp)) return false;
  setworn(NULL, (otmp->owornmask & W_ARMOR));
  if (delay) {
    //    nomul(delay); // XXXXXXX need implementing/testing/revising/thingy
    switch(otmp->otype) {
    case HELMET:
      //      nomovemsg = "You finished taking off your helmet.";
      message("You finished taking off your helmet.");
      break;
    case PAIR_OF_GLOVES:
      //      nomovemsg = "You finished taking off your gloves";
      message("You finished taking off your gloves.");
      break;
    default:
      //      nomovemsg = "You finished taking off your suit.";
      message("You finished taking off your suit.");
    }
  } else {
    //off_msg(otmp); /* ("You were wearing %s.", doname(otmp)); */
    message("You take it off.");
  }
  return true;
}




Boolean do_wear_armor(obj_t *otmp) // was doweararm - was do_wear.c
{
  Short delay;
  Long mask = 0;

  //  otmp = getobj("[", "wear");
  if (!otmp) return false;
  if (otmp->owornmask & W_ARMOR) {
    message("You are already wearing that!");
    return false;
  }

  switch(otmp->otype) {
  case HELMET:
    if (uarmh) {
      message("You are already wearing a helmet.");
      return false;
    } else
      mask = W_ARMH;
    break;
  case SHIELD:
    if (uarms) {
      message("You are already wearing a shield.");
      return false;
    }
    if (uwep && uwep->otype == TWO_HANDED_SWORD) {
      // message("You cannot wear a shield and wield a two-handed sword.");
      message("You are already using both hands.");
      return false;
    }
    mask = W_ARMS;
    break;
  case PAIR_OF_GLOVES:
    if (uarmg) {
      message("You are already wearing gloves.");
      return false;
    } else if (uwep && (uwep->bitflags & O_IS_CURSED)) {
      // message("You cannot wear gloves over your weapon.");
      message("Your (cursed) weapon is in the way.");
      return false;
    } else
      mask = W_ARMG;
    break;
  default:
    if (uarm && (otmp->otype != ELVEN_CLOAK || uarm2)) {
      message("You are already wearing some armor.");
      return false;
    }
    mask = W_ARM;
    break;
  }

  // Uh.  _Is_ it possible to wield a cursed item and then try to wear it?
  if (otmp == uwep && (uwep->bitflags & O_IS_CURSED)) {
    // pline("%s is welded to your hand.", Doname(uwep));
    message("It is already welded to your hand!");
    return false;
  }
  setworn(otmp, mask);
  if (otmp == uwep)
    setuwep(NULL);
  delay = -objects[otmp->otype].oc_delay;
  if (delay) {
    //    nomul(delay); // XXXXXXX need implementing/testing/revising/thingy
    //    nomovemsg = "You finished your dressing manoeuvre.";
    message("You finish your dressing manoeuvre.");
  }
  otmp->bitflags |= O_IS_KNOWN; //otmp->known = 1;
  return true;
}


Boolean do_wear_ring(obj_t *otmp)
{
  Long mask = 0;
  Long oldprop;

  if (uleft && uright) {
    message("There are no more ring-fingers to fill.");
    return false;
  }
  if (!otmp) return false;
  if (otmp->owornmask & W_RING) {
    message("You are already wearing that!");
    return false;
  }
  if (otmp == uleft || otmp == uright) {
    message("You are already wearing that.");
    return false;
  }
  if (otmp == uwep && (uwep->bitflags & O_IS_CURSED)) {
    //    message("%s is welded to your hand.", Doname(uwep));
    message("It's already welded to your hand.");
    return false;
  }
  if (uleft) mask = RIGHT_RING;
  else if (uright) mask = LEFT_RING;
  else { // Not that it matters anymore...
    if (FrmAlert(RightLeftP) == 0)
      mask = RIGHT_RING;
    else
      mask = LEFT_RING;
  }

  setworn(otmp, mask);
  if (otmp == uwep)
    setuwep(NULL);
  // XXXX The EFFECTS of wearing a ring are not tested yet!
  oldprop = you.uprops[PROP(otmp->otype)].p_flags;
  you.uprops[PROP(otmp->otype)].p_flags |= mask;
  switch(otmp->otype) {
  case RIN_LEVITATION:
    if (!oldprop)
      float_up();
    break;
  case RIN_PROTECTION_FROM_SHAPE_CHANGERS:
    res_cham();
    break;
  case RIN_GAIN_STRENGTH:
    you.ustr += otmp->spe;
    you.ustrmax += otmp->spe;
    if (you.ustr > 118) you.ustr = 118;
    if (you.ustrmax > 118) you.ustrmax = 118;
    flags.botl |= BOTL_STR;
    break;
  case RIN_INCREASE_DAMAGE:
    you.udaminc += otmp->spe;
    break;
  }
  prinv(otmp); // basically just pumps the object name into message()
  return true;
}


void ringoff(obj_t *obj)
{
  Long mask;
  mask = obj->owornmask & W_RING;
  setworn(NULL, obj->owornmask);
  if (!(you.uprops[PROP(obj->otype)].p_flags & mask))
    message("Strange... I didn't know you had that ring."); // BUG if that.
  you.uprops[PROP(obj->otype)].p_flags &= ~mask;
  switch(obj->otype) {
  case RIN_FIRE_RESISTANCE:
    // Bad luck if the player is in hell... --jgm
    if (!Fire_resistance && dlevel >= 30) {
      message("The flames of Hell burn you to a crisp.");
      killer = "stupidity in hell"; // XXX
      done("burned"); // XXX
      return;
    }
    break;
  case RIN_LEVITATION:
    if (!Levitation) {	// no longer floating
      float_down();
    }
    break;
  case RIN_GAIN_STRENGTH:
    you.ustr -= obj->spe;
    you.ustrmax -= obj->spe;
    if (you.ustr > 118) you.ustr = 118;
    if (you.ustrmax > 118) you.ustrmax = 118;
    flags.botl |= BOTL_STR;
    break;
  case RIN_INCREASE_DAMAGE:
    you.udaminc -= obj->spe;
    break;
  }

}



void find_ac()
{
  Short uac = 10;
  if (uarm) uac -= ARM_BONUS(uarm);
  if (uarm2) uac -= ARM_BONUS(uarm2);
  if (uarmh) uac -= ARM_BONUS(uarmh);
  if (uarms) uac -= ARM_BONUS(uarms);
  if (uarmg) uac -= ARM_BONUS(uarmg);
  if (uleft && uleft->otype == RIN_PROTECTION) uac -= uleft->spe;
  if (uright && uright->otype == RIN_PROTECTION) uac -= uright->spe;
  if (uac != you.uac){
    you.uac = uac;
    flags.botl |= BOTL_AC;
  }
}



void slippery_fingers() // was "glibr()"
{
  obj_t *otmp;
  Boolean also = false;
  if (!uarmg) if (uleft || uright) {
    /* Note: at present also cursed rings fall off */ // Bug or feature? sprite
    if (uleft && uright) message("Your rings slip off your fingers.");
    else                 message("Your ring slips off your finger.");
    also = true;
    if ((otmp = uleft) != NULL) {
      ringoff(uleft);
      dropx(otmp);
    }
    if ((otmp = uright) != NULL) {
      ringoff(uright);
      dropx(otmp);
    }
  }
  if ((otmp = uwep) != NULL) {
    /* Note: at present also cursed weapons fall */ // Bug or feature? sprite
    setuwep(NULL);
    dropx(otmp);
    // If 'also' then say "also slips" instead.  Nah.  I'm short on space.
    message("Your weapon slips from your hands.");
  }
}

obj_t * some_armor()
{
  obj_t *otmph = uarm;
  if (uarmh && (!otmph || !rund(4))) otmph = uarmh;
  if (uarmg && (!otmph || !rund(4))) otmph = uarmg;
  if (uarms && (!otmph || !rund(4))) otmph = uarms;
  return otmph;
}



void corrode_armor()
{
  obj_t *otmph = some_armor();
  if (otmph) {
    if ((otmph->bitflags & O_IS_RUSTFREE) ||
	otmph->otype == ELVEN_CLOAK ||
	otmph->otype == LEATHER_ARMOR ||
	otmph->otype == STUDDED_LEATHER_ARMOR) {
      StrPrintF(ScratchBuffer, "Your %s not affected!",
		aobjnam(otmph, "are"));
      message(ScratchBuffer);
      return;
    }
    StrPrintF(ScratchBuffer, "Your %s!", aobjnam(otmph, "corrode"));
    message(ScratchBuffer);
    otmph->spe--;
  }
}

