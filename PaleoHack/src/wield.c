/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
#include "paleohackRsc.h" // to LeaveForm when stoned.

void setuwep(struct obj *obj)
{
  setworn(obj, W_WEP);
}

// return true if player was fatally stupid
Boolean oops_cockatrice(obj_t *wep)
{
  if (!wep) return false;
  if (!uarmg && wep->otype == DEAD_COCKATRICE) {
    if (FrmGetActiveFormID() != MainForm) LeaveForm(); // XXXX ?????
    message("You wield the dead cockatrice in your bare hands.");
    message("You turn to stone ...");
    killer="dead cockatrice";
    done("died");
    return true;
  }
  return false;
}

// Return 'true' if we actually Did Something
// note: Some of the messages are shortened to fit in one line.
Boolean do_wield(obj_t *wep)
{
  if (wep == uwep && FrmGetActiveFormID() == InvForm)
    wep = NULL; // this is my little ACT_PUTUP kludge.

  if (wep == NULL) {
    if (uwep == 0)
      message("You are already empty handed.");
    else {
      setuwep(NULL);
      message("You are empty handed.");
      return true;
    }
    return false;
  }

  if (wep == uwep)
    message("You are already wielding that!");
  else if (uwep && (uwep->bitflags & O_IS_CURSED)) {
    //    message("Your weapon is welded to your hand!");
    // need "aobjnam(uwep, "are"));"
    StrPrintF(ScratchBuffer, "The %s welded to your hand!",
	      aobjnam(uwep, "are"));
    message(ScratchBuffer);    
  }
  // The cockatrice case is a bugfix from the 1980s.
  else if (oops_cockatrice(wep)) {
    return true;
  } // end of 80s bugfix.
  else if (uarms && wep->otype == TWO_HANDED_SWORD)
    //pline("You cannot wield a two-handed sword and wear a shield.");
    message("First you must take off your shield.");
  else if (wep->owornmask & (W_ARMOR | W_RING))
    message("You cannot wield that!");
  else {
    setuwep(wep);
    if (uwep->bitflags & O_IS_CURSED) {
      StrPrintF(ScratchBuffer, "The %s %s to your hand!",
		aobjnam(uwep, "weld"),
		(uwep->quantity == 1) ? "itself" : "themselves"); /* a3 */
      message(ScratchBuffer);
      // if (uwep->quantity == 1) message("It welds itself to your hand!");
      // else message("They weld themselves to your hand!");
    } else {
      prinv(uwep);
    }
    return true;
  }
  return false;
}

void corrode_weapon()
{
  if (!uwep || uwep->olet != WEAPON_SYM) return;	/* %% */
  if (uwep->bitflags & O_IS_RUSTFREE) {
    StrPrintF(ScratchBuffer, "Your %s not affected.", aobjnam(uwep, "are"));
    message(ScratchBuffer);
  } else {
    StrPrintF(ScratchBuffer, "Your %s!", aobjnam(uwep, "corrode"));
    message(ScratchBuffer);
    uwep->spe--;
  }
}


tri_val_t chwepon(obj_t *otmp, Short amount)
{
  if (!uwep || uwep->olet != WEAPON_SYM) {
    if (strange_feeling(otmp,
			(amount > 0) ? "Your hands twitch."
			: "Your hands itch."))
      return GO_ON;
    else return NO_OP;
  }

  if (uwep->otype == WORM_TOOTH && amount > 0) {
    uwep->otype = CRYSKNIFE;
    message("Your weapon seems sharper now.");
    uwep->bitflags &= ~O_IS_CURSED; //  uwep->cursed = 0;
    return DONE;
  }

  if (uwep->otype == CRYSKNIFE && amount < 0) {
    uwep->otype = WORM_TOOTH;
    message("Your weapon looks duller now.");
    return DONE;
  }

  /* there is a (soft) upper limit to uwep->spe */
  if ((amount > 0) && (uwep->spe > 5) && rund(3)) {
    StrPrintF(ScratchBuffer, 
	      "Your %s violently green for a while and then evaporate%s",
	      aobjnam(uwep, "glow"),
	      ((uwep->quantity == 1) ? "s." : "."));
    message(ScratchBuffer);
    /* let all of them disappear. note: 'uwep->quan = 1' is nogood if unpaid */
    while (uwep)
      useup(uwep);
    return DONE;
  }
  if (!rund(6)) amount *= 2;
  StrPrintF(ScratchBuffer, "Your %s %s for a %s.", // "NOUN+VERB COLOR TIME."
	aobjnam(uwep, "glow"),
	((amount < 0) ? "black" : "green"),
	((amount*amount == 1) ? "moment" : "while"));
  message(ScratchBuffer);
  uwep->spe += amount;
  if (amount > 0) uwep->bitflags &= ~O_IS_CURSED; //  uwep->cursed = 0;
  return DONE;
}
