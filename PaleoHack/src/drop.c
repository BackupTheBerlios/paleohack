/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"

// all this stuff was in do.c
static void dropy(obj_t *obj) SEC_2;

/* 
Boolean dodrop()
{
  return drop(getobj("0$#", "drop"));
}
*/

// return true if you dropped something
Boolean drop(obj_t *obj)
{
  if (!obj) return false;
  if (obj->olet == '$') {		/* pseudo object */
    /* // XXX this section is not implemented yet
    Long amount = OGOLD(obj);

    if (amount == 0)
      message("You didn't drop any gold pieces.");
    else {
      mkgold(amount, you.ux, you.uy);
      StrPrintF(buf, "You dropped %ld gold piece%s",
		amount, (amount==1) ? "." : "s.");
      message(buf);
      if (Invisible) newsym(you.ux, you.uy);
    }
    free_me((VoidPtr) obj); //    free((Char *) obj); // not obfree, eh
    return true;
    */
  }
  if (obj->owornmask & (W_ARMOR | W_RING)) {
    message("You cannot drop something you are wearing.");
    return false;
  }
  if (obj == uwep) {
    if (uwep->bitflags & O_IS_CURSED) {
      message("Your weapon is welded to your hand!");
      return false;
    }
    setuwep(NULL);
  }
  StrPrintF(ScratchBuffer, "You dropped %s.", doname(obj));
  message(ScratchBuffer);
  dropx(obj);
  return true;
}

/* Called in several places - do not print out anything. */
void dropx(struct obj *obj)
{
  unlink_inv(obj); //freeinv(obj);
  dropy(obj);
}

static void dropy(struct obj *obj)
{
  if (obj->otype == CRYSKNIFE)
    obj->otype = WORM_TOOTH;
  obj->ox = you.ux;
  obj->oy = you.uy;
  obj->nobj = fobj;
  fobj = obj;
  if (Invisible) newsym(you.ux,you.uy);
  subfrombill(obj);
  stackobj(obj);
}

/* drop several things */
/*
Short doddrop()
{
  return(ggetobj("drop", drop, 0));
}
*/
