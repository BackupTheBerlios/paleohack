/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
#include "paleohackRsc.h"

static void monstone(monst_t *mdef) SEC_3;

static Boolean far_noise;
static Long noisetime;

/* hitmm returns 0 (miss), 1 (hit), or 2 (kill) */
#define HIT_MISS 0
#define HIT_HIT 1
#define HIT_KILL 2
Short hitmm(monst_t *magr, monst_t *mdef) // aggressor, defender
{
  permonst_t *pa = magr->data, *pd = mdef->data;
  Short hit;
  Int8 tmp;
  Boolean vis;
  if (StrChr("Eauy", pa->mlet)) return HIT_MISS;
  if (magr->bitflags & M_IS_FROZEN) return HIT_MISS;	    /* riv05!a3 */
  tmp = pd->ac + pa->mlevel;
  if (mdef->bitflags & (M_IS_CONFUSED | M_IS_FROZEN | M_IS_ASLEEP)) {
    tmp += 4;
    if (mdef->bitflags & M_IS_ASLEEP) mdef->bitflags &= ~M_IS_ASLEEP;
  }
  hit = (tmp > rnd(20));
  if (hit) mdef->bitflags &= ~M_IS_ASLEEP;
  vis = (cansee(magr->mx,magr->my) && cansee(mdef->mx,mdef->my));
  if (vis) {
    if (mdef->bitflags & M_IS_MIMIC) see_mimic(mdef);
    if (magr->bitflags & M_IS_MIMIC) see_mimic(magr);
    // need to split this up due to Monnam and monnam using the same buffer:
    StrPrintF(ScratchBuffer, "%s %s", Monnam(magr), (hit ? "hits" : "misses"));
    StrPrintF(ScratchBuffer+StrLen(ScratchBuffer), " %s.", monnam(mdef));
    message(ScratchBuffer);
  } else {
    Boolean far = (dist(magr->mx, magr->my) > 15);
    if (far != far_noise || moves - noisetime > 10) {
      far_noise = far;
      noisetime = moves;
      StrPrintF(ScratchBuffer, "You hear some noises%s",
		far ? " in the distance." : ".");
      message(ScratchBuffer);
    }
  }
  if (hit) {
    if (magr->data->mlet == 'c' && !(magr->bitflags & M_IS_CHAMELEON)) {
      magr->mhpmax += 3;
      if (vis) {
	StrPrintF(ScratchBuffer, "%s is turned to stone!", Monnam(mdef));
	message(ScratchBuffer);
      } else if (mdef->bitflags & M_IS_TAME)
	message("You have a peculiarly sad feeling for a moment, then it passes.");
      monstone(mdef);
      hit = 2;
    } else
      if ((mdef->mhp -= dice(pa->damn,pa->damd)) < 1) {
	magr->mhpmax += 1 + rund(pd->mlevel+1);
	if ((magr->bitflags & M_IS_TAME) && magr->mhpmax > 8*pa->mlevel) {
	  /*
	  if (pa == &li_dog) magr->data = pa = &dog;
	  else if (pa == &dog) magr->data = pa = &la_dog;
	  */ // Small --> Medium --> Large!
	  if (pa == PM_SM_DOG) magr->data = pa = PM_MD_DOG;
	  else if (pa == PM_MD_DOG) magr->data = pa = PM_LG_DOG;
	}
	if (vis) {
	  StrPrintF(ScratchBuffer, "%s is killed!", Monnam(mdef));
	  message(ScratchBuffer);
	} else if (mdef->bitflags & M_IS_TAME)
	  message("You have a sad feeling for a moment, then it passes.");
	mondied(mdef);
	hit = 2;
      }
  }
  return(hit);
}

/* drop (perhaps) a cadaver and remove monster */
void mondied(monst_t *mdef) // This should be moved to fight.c....
{
  permonst_t *pd = mdef->data;
  if (letter(pd->mlet) && rund(3)) {
    mkobj_at(pd->mlet, mdef->mx, mdef->my);
    if (cansee(mdef->mx, mdef->my)){
      unpmon(mdef);
      print(mdef->mx, mdef->my, fobj->olet);
    }
    stackobj(fobj);
  }
  mondead(mdef);
}

/* drop a rock and remove monster */
const Char mlarge[] = "bCDdegIlmnoPSsTUwY',&";
static void monstone(monst_t *mdef)
{
  if (StrChr(mlarge, mdef->data->mlet))
    mksobj_at(ENORMOUS_ROCK, mdef->mx, mdef->my);
  else
    mksobj_at(ROCK, mdef->mx, mdef->my);
  if (cansee(mdef->mx, mdef->my)) {
    unpmon(mdef);
    print(mdef->mx, mdef->my, fobj->olet);
  }
  mondead(mdef);
}

Short fightm(monst_t *mtmp)
{
  monst_t *mon;
  for (mon = fmon ; mon ; mon = mon->nmon) {
    if (mon != mtmp) {
      if (DIST(mon->mx,mon->my,mtmp->mx,mtmp->my) < 3)
	if (rund(4))
	  return(hitmm(mtmp,mon));
    }
  }
  return(-1);
}

const Char vowels[] = "aeiou";
// I have eliminated "setan" which was in objnam.c and was used only here.
/* u is hit by sth, but not a monster */
Boolean thing_hit_you(Short tlev, Short dam, Char *name)
{
  Boolean use_an = (NULL != StrChr(vowels, name[0]));
  if (you.uac + tlev <= rnd(20)) {
    if (Blind) message("It misses.");
    else {
      StrPrintF(ScratchBuffer, "You are almost hit by %s %s!",
		(use_an ? "an" : "a"), name);
      message(ScratchBuffer);
    }
    return false;
  } else {
    if (Blind) message("You are hit!");
    else {
      StrPrintF(ScratchBuffer, "You are hit by %s %s!",
		(use_an ? "an" : "a"), name);
      message(ScratchBuffer);
    }
    losehp(dam, name);
    return true;
  }
}



/* return TRUE if mon still alive */
Boolean hit_mon(monst_t *mon, obj_t *obj, Short thrown) // was hmon
{
  Short tmp;
  Boolean hittxt = false;

  if (!obj) {
    tmp = rnd(2);	/* attack with bare hands */
    if (mon->data->mlet == 'c' && !uarmg) {
      message("You hit the cockatrice with your bare hands.");
      message("You turn to stone ...");
      done_in_by(mon); // XXXX
    }
  } else if (obj->olet == WEAPON_SYM || obj->otype == PICK_AXE) {
    if (obj == uwep && (obj->otype > SPEAR || obj->otype < BOOMERANG))
      tmp = rnd(2);
    else {
      if (StrChr(mlarge, mon->data->mlet)) {
	tmp = rnd(objects[obj->otype].wldam);
	if (obj->otype == TWO_HANDED_SWORD) tmp += dice(2,6);
	else if (obj->otype == FLAIL) tmp += rnd(4);
      } else {
	tmp = rnd(objects[obj->otype].wsdam);
      }
      tmp += obj->spe;
      if (!thrown && obj == uwep && obj->otype == BOOMERANG
	  && !rund(3)) {
	StrPrintF(ScratchBuffer, 
		  "As you hit %s, the boomerang breaks into splinters.",
		  monnam(mon));
	message(ScratchBuffer);
	unlink_inv(obj);
	setworn(NULL, obj->owornmask);
	free_obj(obj, NULL);
	tmp++;
      }
    }
    if (mon->data->mlet == 'O' && obj->otype == TWO_HANDED_SWORD &&
	!StrCompare(ONAME(obj), "Orcrist")) // XXX Orcrist not tested yet
      tmp += rnd(10);
  } else switch(obj->otype) {
  case HEAVY_IRON_BALL:
    tmp = rnd(25); break;
  case EXPENSIVE_CAMERA:
    message("You succeed in destroying your camera. Congratulations!");
    unlink_inv(obj);
    if (obj->owornmask)
      setworn(NULL, obj->owornmask);
    free_obj(obj, NULL);
    return true;
  case DEAD_COCKATRICE:
    StrPrintF(ScratchBuffer, "You hit %s with the cockatrice corpse.",
	      monnam(mon));
    message(ScratchBuffer);
    if (mon->data->mlet == 'c') {
      tmp = 1;
      hittxt = true;
      break;
    }
    StrPrintF(ScratchBuffer, "%s is turned to stone!", Monnam(mon));
    message(ScratchBuffer);
    killed(mon);
    return false;
  case CLOVE_OF_GARLIC:		/* no effect against demons */
    if (StrChr(UNDEAD, mon->data->mlet))
      mon->mflee_and_time |= M_FLEEING;
    tmp = 1;
    break;
  default:
    /* non-weapons can damage because of their weight */
    /* (but not too much) */
    tmp = obj->owt/10;
    if (tmp < 1) tmp = 1;
    else tmp = rnd(tmp);
    if (tmp > 6) tmp = 6;
  }

  /****** NOTE: perhaps obj is undefined!! (if !thrown && BOOMERANG) */

  tmp += you.udaminc + dbon();
  if (you.uswallow) {
    if ((tmp -= you.uswallowedtime) <= 0) {
      message("Your arms are no longer able to hit.");
      return true;
    }
  }
  if (tmp < 1) tmp = 1;
  mon->mhp -= tmp;
  if (mon->mhp < 1) {
    killed(mon);
    return false;
  }

  if (mon->bitflags & M_IS_TAME) {
    UChar ctr = (mon->mflee_and_time & ~M_FLEEING);
    if (ctr || !(mon->mflee_and_time & M_FLEEING)) /* Rick Richardson: */
      mon->mflee_and_time = M_FLEEING | (ctr + 10*rnd(tmp));
  }

  if (!hittxt) {
    if (thrown)
      /* this assumes that we cannot throw plural things */
      hit_message( xname(obj)  /* or: objects[obj->otype].oc_name */,
		   mon, exclaim(tmp) );
    else if (Blind)
      message("You hit it.");
    else {
      StrPrintF(ScratchBuffer, "You hit %s%c", monnam(mon), exclaim(tmp));
      message(ScratchBuffer);
    }
  }

  if (you.umconf && !thrown) {
    if (!Blind) {
      message("Your hands stop glowing blue.");
      if (!(mon->bitflags & (M_IS_FROZEN | M_IS_ASLEEP))) {
	StrPrintF(ScratchBuffer, "%s appears confused.", Monnam(mon));
	message(ScratchBuffer);
      }
    }
    mon->bitflags |= M_IS_CONFUSED;
    you.umconf = 0;
  }
  return true;	/* mon still alive */
}




/* try to attack; return false if monster evaded */
/* you.dx and you.dy must be set */
Boolean attack(monst_t *mtmp)
{
  Int8 tmp;
  Boolean malive = true;
  permonst_t *mdat;
  mdat = mtmp->data;

  you_wipe_engr(3);   /* andrew@orca: prevent unlimited pick-axe attacks */
  
  if (mdat->mlet == 'L' &&
      !(mtmp->bitflags & (M_IS_FROZEN | M_IS_ASLEEP | M_IS_CONFUSED)) &&
      (mtmp->mcansee_and_blinded & M_CAN_SEE) && !rund(7) &&
      (m_move(mtmp, 0) == 2 /* he died */ || /* he moved: */
       mtmp->mx != you.ux+you.dx || mtmp->my != you.uy+you.dy))
    return false;

  if (mtmp->bitflags & M_IS_MIMIC) {
    if (!you.ustuck && !(mtmp->mflee_and_time & M_FLEEING))
      you.ustuck = mtmp;
    switch(floor_symbol[you.ux+you.dx][you.uy+you.dy]) {
    case DOOR_SYM:
      message("The door actually was a Mimic.");
      break;
    case GOLD_SYM:
      message("The chest was a Mimic!");
      break;
    default:
      message("Wait! That's a Mimic!");
    }
    wakeup(mtmp);	/* clears mtmp->mimic */
    return true;
  }

  wakeup(mtmp);

  if ((mtmp->bitflags & M_IS_HIDER) && (mtmp->bitflags & M_IS_UNDETECTED)) {
    obj_t *obj;

    mtmp->bitflags &= ~M_IS_UNDETECTED;
    if ((obj = obj_at(mtmp->mx,mtmp->my)) && !Blind) {
      StrPrintF(ScratchBuffer, "Wait! There's a %s hiding under %s!",
		mon_names + mdat->mname_offset/*mdat->mname*/, doname(obj));
      message(ScratchBuffer);
    }
    return true;
  }

  tmp = you.uluck + you.ulevel + mdat->ac + abon();
  if (uwep) {
    if (uwep->olet == WEAPON_SYM || uwep->otype == PICK_AXE)
      tmp += uwep->spe;
    if (uwep->otype == TWO_HANDED_SWORD) tmp -= 1;
    else if (uwep->otype == DAGGER) tmp += 2;
    else if (uwep->otype == CRYSKNIFE) tmp += 3;
    else if (uwep->otype == SPEAR &&
	     StrChr("XDne", mdat->mlet)) tmp += 2;
  }
  if (mtmp->bitflags & M_IS_ASLEEP) {
    mtmp->bitflags &= ~M_IS_ASLEEP;
    tmp += 2;
  }
  if (mtmp->bitflags & M_IS_FROZEN) {
    tmp += 4;
    if (!rund(10)) mtmp->bitflags &= ~M_IS_FROZEN;
  }
  if (mtmp->mflee_and_time & M_FLEEING) tmp += 2;
  if (you.utrap) tmp -= 3;

  /* with a lot of luggage, your agility diminishes */
  tmp -= (inv_weight() + 40)/20;

  if (tmp <= rnd(20) && !you.uswallow) {
    if (Blind) message("You miss it.");
    else {
      StrPrintF(ScratchBuffer, "You miss %s.",monnam(mtmp));
      message(ScratchBuffer);
    }
  } else {
    /* we hit the monster; be careful: it might die! */

    if ((malive = hit_mon(mtmp,uwep,0)) == true) {
      /* monster still alive */
      if (!rund(25) && mtmp->mhp < mtmp->mhpmax/2) {
	if (!rund(3)) mtmp->mflee_and_time = rnd(100) | M_FLEEING;
	else mtmp->mflee_and_time |= M_FLEEING;
	if (you.ustuck == mtmp && !you.uswallow)
	  you.ustuck = 0;
      }
#ifndef NOWORM
      if (mtmp->wormno)
	cutworm(mtmp, you.ux+you.dx, you.uy+you.dy,
		uwep ? uwep->otype : 0);
#endif NOWORM
    }
    if (mdat->mlet == 'a') {
      if (rund(2)) {
	message("You are splashed by the blob's acid!");
	losehp_m(rnd(6), mtmp);
	if (!rund(30)) corrode_armor();
      }
      if (!rund(6)) corrode_weapon();
    }
  }
  if (malive && mdat->mlet == 'E' && canseemon(mtmp)
      && !(mtmp->bitflags & M_IS_CANCELLED) && rund(3)) {
    if (mtmp->mcansee_and_blinded & M_CAN_SEE) {
      message("You are frozen by the floating eye's gaze!");
      nomul((you.ulevel > 6 || rund(4)) ? rund(20)-21 : -200);
    } else {
      message("The blinded floating eye cannot defend itself.");
      if (!rund(500)) if ((Short)you.uluck > LUCKMIN) you.uluck--;
    }
  }
  return true;
}
