/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
#include "bit.h" // for do_throw

// all this stuff was in do.c
static void dropy(obj_t *obj) SEC_5;
static void mon_accept_gift(monst_t *mon, obj_t *obj) SEC_5;

/* 
Boolean dodrop()
{
  return drop(getobj("0$#", "drop"));
}
*/

// return "number of things dropped"
// (used to be "return true if you dropped something")
Short drop(obj_t *obj)
{
  if (!obj) return 0;
  if (obj->olet == '$') {		/* pseudo object */
     // this section works now
    Long amount = (Long) obj->oextra; // XXXX!

    if (amount <= 0) // (shouldn't happen, given current UI)
      message("You didn't drop any gold pieces.");
    else {
      mkgold(amount, you.ux, you.uy);
      StrPrintF(ScratchBuffer, "You dropped %ld gold piece%s",
		amount, (amount==1) ? "." : "s.");
      message(ScratchBuffer);
      if (Invisible) newsym(you.ux, you.uy);
    }
    free_me((VoidPtr) obj); //    free((Char *) obj); // not obfree, eh
    return 1;
    // end of formerly-untested-and-unreachable region
  }
  if (obj->owornmask & (W_ARMOR | W_RING)) {
    message("You cannot drop something you are wearing.");
    return 0;
  }
  if (obj == uwep) {
    if (uwep->bitflags & O_IS_CURSED) {
      message("Your weapon is welded to your hand!");
      return 0;
    }
    setuwep(NULL);
  }
  StrPrintF(ScratchBuffer, "You dropped %s.", doname(obj));
  message(ScratchBuffer);
  dropx(obj);
  return 1;
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




extern PointType bhitpos; // used in bhit
// was in do.c
// Caller must first getobj AND get a direction.
Boolean do_throw(obj_t *obj)
{
  monst_t *mon;
  Short tmp;

  //  obj = getobj("#)", "throw");   /* it is also possible to throw food */
  /* (or jewels, or iron balls ... ) */
  //  if (!obj || !getdir(1))	       /* ask "in what direction?" */
  //    return false;
  if (!obj) return false;
  if (!you.dx && !you.dy && !you.dz) return false;

  if (obj->owornmask & (W_ARMOR | W_RING)) {
    message("You can't throw something you are wearing.");
    return false;
  }

  you_wipe_engr(2);

  if (obj == uwep) {
    if (obj->bitflags & O_IS_CURSED) {
      message("Your weapon is welded to your hand.");
      return true;
    }
    if (obj->quantity > 1)
      setuwep(splitobj(obj, 1));
    else
      setuwep((obj_t *) NULL);
  }
  else if (obj->quantity > 1)
    splitobj(obj, 1);
  unlink_inv(obj);
  if (you.uswallow) {
    mon = you.ustuck;
    bhitpos.x = mon->mx;
    bhitpos.y = mon->my;
  } else if (you.dz) {
    if (you.dz < 0) {
      // Object was thrown "UP"
      StrPrintF(ScratchBuffer, 
		"%s hits the ceiling, then falls back on top of your head.",
		Doname(obj));		/* note: obj->quan == 1 */
      message(ScratchBuffer);
      if (obj->olet == POTION_SYM)
	potionhit(NULL, obj); // Potion hits YOU!
      else {
	if (uarmh) message("Fortunately, you are wearing a helmet!");
	losehp(uarmh ? 1 : rnd((Short)(obj->owt)), "falling object");
	dropy(obj);
      }
    } else {
      // Object was thrown "DOWN"
      StrPrintF(ScratchBuffer, "%s hits the floor.", Doname(obj));
      message(ScratchBuffer);
      if (obj->otype == EXPENSIVE_CAMERA) {
	message("It is shattered in a thousand pieces!");
	free_obj(obj, NULL); //	obfree(obj, Null(obj));
      } else if (obj->otype == EGG) {
	message("\"Splash!\"");
	free_obj(obj, NULL); //	obfree(obj, Null(obj));
      } else if (obj->olet == POTION_SYM) {
	message("The flask breaks, and you smell a peculiar odor ...");
	potionbreathe(obj);
	free_obj(obj, NULL); //	obfree(obj, Null(obj));
      } else {
	dropy(obj);
      }
    }
    return true;
  } else if (obj->otype == BOOMERANG) {
    Boolean you_caught_it = false;
    mon = boomhit(you.dx, you.dy, &you_caught_it);
    if (you_caught_it) {
      addinv(obj);
      return true;
    }
  } else {
    if (obj->otype == PICK_AXE && shkcatch(obj))
      return true;

    mon = bhit(you.dx, you.dy, ( (obj->otype == ICE_BOX) ? 1 :
				 (!Punished || obj != uball) ? 8 :
				 !you.ustuck ? 5 : 1 ),
	       obj->olet, NULL, obj);
  }
  if (mon) {
    /* awake monster if sleeping */
    wakeup(mon);

    if (obj->olet == WEAPON_SYM) {
      tmp = -1 + you.ulevel + mon->data->ac + abon();
      if (obj->otype < ROCK) {
	if (!uwep || (uwep->otype != obj->otype + (BOW-ARROW)) )
	  tmp -= 4;
	else
	  tmp += uwep->spe;
      } else
	if (obj->otype == BOOMERANG)
	  tmp += 4;
      tmp += obj->spe;
      if (you.uswallow || tmp >= rnd(20)) {
	if (hit_mon(mon, obj, 1) == true) {
	  /* mon still alive */
#ifndef NOWORM
	  cutworm(mon, bhitpos.x, bhitpos.y, obj->otype);
#endif NOWORM
	} else mon = NULL;
				/* weapons thrown disappear sometimes */
	if (obj->otype < BOOMERANG && rund(3)) {
	  /* check bill; free */
	  free_obj(obj, NULL);
	  return true;
	}
      } else miss_message(oc_names + objects[obj->otype].oc_name_offset, mon);
    } else if (obj->otype == HEAVY_IRON_BALL) {
      tmp = -1 + you.ulevel + mon->data->ac + abon();
      if (!Punished || obj != uball) tmp += 2;
      if (you.utrap) tmp -= 2;
      if (you.uswallow || tmp >= rnd(20)) {
	if (false == hit_mon(mon, obj, 1))
	  mon = NULL;	/* he died */
      } else miss_message("iron ball", mon);
    } else if (obj->olet == POTION_SYM && you.ulevel > rund(15)) {
      potionhit(mon, obj);
      return true;
    } else {
      if (cansee(bhitpos.x, bhitpos.y)) {
	StrPrintF(ScratchBuffer, "You miss %s.",monnam(mon));
	message(ScratchBuffer);
      } else message("You miss it.");

      if (obj->olet == FOOD_SYM && mon->data->mlet == 'd')
	if (tamedog(mon,obj))
	  return true;

      if (obj->olet == GEM_SYM && mon->data->mlet == 'u' &&
	  !(mon->bitflags & M_IS_TAME)) {
	if ((obj->bitflags & O_IS_DESCKNOWN) &&
	    BITTEST(oc_name_known, obj->otype)) {
	  if (objects[obj->otype].g_val > 0) {
	    you.uluck += 5;
	    mon_accept_gift(mon, obj);
	    return true;
	  } else {
	    StrPrintF(ScratchBuffer, "%s is not interested in your junk.",
		      Monnam(mon));
	    message(ScratchBuffer);
	  }
	} else { /* value unknown to @ */
	  you.uluck++;
	  mon_accept_gift(mon, obj);
	  return true;
	}
      }

    }
  }
  /* the code following might become part of dropy() */
  if (obj->otype == CRYSKNIFE)
    obj->otype = WORM_TOOTH;
  obj->ox = bhitpos.x;
  obj->oy = bhitpos.y;
  obj->nobj = fobj;
  fobj = obj;
  /* prevent him from throwing articles to the exit and escaping */
  /* subfrombill(obj); */ // This was commented out when I found it.
  stackobj(obj);
  if (Punished && obj==uball && (bhitpos.x != you.ux || bhitpos.y != you.uy)) {
    unlink_obj(uchain);
    unpobj(uchain);
    if (you.utrap) {
      if (you.utraptype == TT_PIT)
	message("The ball pulls you out of the pit!");
      else {
	Long side = rund(3) ? LEFT_SIDE : RIGHT_SIDE;
	message("The ball pulls you out of the bear trap.");
	StrPrintF(ScratchBuffer, "Your %s leg is severely damaged.",
		  (side == LEFT_SIDE) ? "left" : "right");
	message(ScratchBuffer);
	set_wounded_legs(side, 500+rund(1000));
	losehp(2, "thrown ball");
      }
      you.utrap = 0;
    }
    unsee();
    uchain->nobj = fobj;
    fobj = uchain;
    you.ux = uchain->ox = bhitpos.x - you.dx;
    you.uy = uchain->oy = bhitpos.y - you.dy;
    setsee();
    (void) inshop();
  }
  if (cansee(bhitpos.x, bhitpos.y)) prl(bhitpos.x,bhitpos.y);
  return true;
}

static void mon_accept_gift(monst_t *mon, obj_t *obj)
{
  if (you.uluck > LUCKMAX)	/* dan@ut-ngp */
    you.uluck = LUCKMAX;
  StrPrintF(ScratchBuffer, "%s graciously accepts your gift.", Monnam(mon));
  message(ScratchBuffer);
  mpickobj(mon, obj);
  rloc(mon);
}
