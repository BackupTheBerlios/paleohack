/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
#include "paleohackRsc.h"

extern Short multi; // living in movesee.c right now..

/*
 * monhityou: monster hits you
 *	  returns true if monster dies (e.g. 'y', 'F'), false otherwise
 */
Boolean mon_hit_you(monst_t *mtmp) // was mhitu
{
  permonst_t *mdat = mtmp->data;
  Short tmp, ctmp;

  nomul(0);

  /* If swallowed, can only be affected by hissers and by you.ustuck */
  if (you.uswallow) {
    if (mtmp != you.ustuck) {
      if (mdat->mlet == 'c' && !rund(13)) {
	StrPrintF(ScratchBuffer, "Outside, you hear %s's hissing!",
		  monnam(mtmp));
	message(ScratchBuffer);
	StrPrintF(ScratchBuffer, "%s gets turned to stone!",
		  Monnam(you.ustuck));
	message(ScratchBuffer);
	message("And the same fate befalls you.");
	done_in_by(mtmp); // xxx
				/* "notreached": not return true; */
      }
      return false;
    }
    switch(mdat->mlet) {	/* now mtmp == you.ustuck */
    case ',':
      youswld(mtmp, ((you.uac > 0) ? you.uac+4 : 4), 5, "The trapper");
      break;
    case '\'':
      youswld(mtmp, rnd(6), 7, "The lurker above");
      break;
    case 'P':
      youswld(mtmp, dice(2,4),12,"The purple worm");
      break;
    default:
      /* This is not impossible! */
      message("The mysterious monster totally digests you.");
      you.uhp = 0;
    }
    if (you.uhp < 1) done_in_by(mtmp); // xxx
    return false;
  }

  if (mdat->mlet == 'c' && Stoned)
    return false;

  /* make eels visible the moment they hit/miss us */
  if (mdat->mlet == ';' &&
      (mtmp->bitflags & M_IS_INVISIBLE) &&
      cansee(mtmp->mx, mtmp->my)) {
    mtmp->bitflags &= ~M_IS_INVISIBLE;
    pmon(mtmp);
  }
  if (!StrChr("1&DuxynNF", mdat->mlet))
    tmp = hit_you(mtmp, dice(mdat->damn,mdat->damd));
  else
    tmp = 0;
  if (StrChr(UNDEAD, mdat->mlet) && midnight())
    tmp += hit_you(mtmp, dice(mdat->damn,mdat->damd));

  ctmp = tmp && !(mtmp->bitflags & M_IS_CANCELLED) &&
    (!uarm || objects[uarm->otype].a_can < rnd(3) || !rund(50));
  switch(mdat->mlet) {
  case '1':
    if (wiz_hit(mtmp)) return true;	/* he disappeared */
    break;
  case '&':
    if (!(mtmp->bitflags & (M_IS_CHAMELEON | M_IS_CANCELLED)) && !rund(13)) {
      makemon(PM_DEMON, you.ux, you.uy);
    } else {
      hit_you(mtmp, dice(2,6));
      hit_you(mtmp, dice(2,6));
      hit_you(mtmp, rnd(3));
      hit_you(mtmp, rnd(3));
      hit_you(mtmp, rund(4)+2);
    }
    break;
  case ',':
    if (tmp) justswld(mtmp,"The trapper");
    break;
  case '\'':
    if (tmp) justswld(mtmp, "The lurker above");
    break;
  case ';':
    if (ctmp) {
      if (!you.ustuck && !rund(10)) {
	StrPrintF(ScratchBuffer, "%s swings itself around you!",
		  Monnam(mtmp));
	message(ScratchBuffer);
	you.ustuck = mtmp;
      } else if (you.ustuck == mtmp &&
		 get_cell_type(floor_info[mtmp->mx][mtmp->my]) == POOL) {
	StrPrintF(ScratchBuffer, "%s drowns you ...", Monnam(mtmp));
	message(ScratchBuffer);
	done("drowned"); // xxx
	return false;
      }
    }
    break;
  case 'A':
    if (ctmp && rund(2)) {
      if (Poison_resistance)
	message("The sting doesn't seem to affect you.");
      else {
	message("You feel weaker!");
	losestr(1);
      }
    }
    break;
  case 'C':
    hit_you(mtmp, rnd(6));
    break;
  case 'c':
    if (!rund(5)) {
      StrPrintF(ScratchBuffer, "You hear %s's hissing!", monnam(mtmp));
      message(ScratchBuffer);
      if (ctmp || !rund(20) || (flags.moon_phase == NEW_MOON
				&& !carrying(DEAD_LIZARD))) {
	Stoned = 5; // these were COMMENTED OUT:
				/* pline("You get turned to stone!"); */
				/* done_in_by(mtmp); */
      }
    }
    break;
  case 'D':
    if (rund(6) || (mtmp->bitflags & M_IS_CANCELLED)) {
      hit_you(mtmp, dice(3,10));
      hit_you(mtmp, rnd(8));
      hit_you(mtmp, rnd(8));
      break;
    }
    kludge("%s breathes fire!", "The dragon");
    buzz(-1, mtmp->mx, mtmp->my, you.ux-mtmp->mx, you.uy-mtmp->my);
    break;
  case 'd':
    hit_you(mtmp, dice(2, (flags.moon_phase == FULL_MOON) ? 3 : 4));
    break;
  case 'e':
    hit_you(mtmp, dice(3,6));
    break;
  case 'F':
    if (mtmp->bitflags & M_IS_CANCELLED) break;
    kludge("%s explodes!", "The freezing sphere");
    if (Cold_resistance) message("You don't seem affected by it.");
    else {
      Int8 dn;
      if (17-(you.ulevel/2) > rnd(20)) {
	message("You get blasted!");
	dn = 6;
      } else {
	message("You duck the blast...");
	dn = 3;
      }
      losehp_m(dice(dn,6), mtmp);
    }
    mondead(mtmp);
    return true;
  case 'g':
    if (ctmp && multi >= 0 && !rund(3)) {
      kludge("You are frozen by %ss juices", "the cube'");
      nomul(-rnd(10));
    }
    break;
  case 'h':
    if (ctmp && multi >= 0 && !rund(5)) {
      nomul(-rnd(10));
      kludge("You are put to sleep by %ss bite!", "the homunculus'");
    }
    break;
  case 'j':
    tmp = hit_you(mtmp, rnd(3));
    tmp &= hit_you(mtmp, rnd(3)); // intriguing
    if (tmp) {
      hit_you(mtmp, rnd(4));
      hit_you(mtmp, rnd(4));
    }
    break;
  case 'k':
    if ((hit_you(mtmp, rnd(4)) || !rund(3)) && ctmp) {
      poisoned("bee's sting", mon_names + mdat->mname_offset);
    }
    break;
  case 'L':
    if (tmp) stealgold(mtmp);
    break;
  case 'N':
    if ((mtmp->bitflags & M_IS_CANCELLED) && !Blind) {
      StrPrintF(ScratchBuffer,
		"%s tries to seduce you, but you seem uninterested.",
		Amonnam(mtmp, "plain"));
      message(ScratchBuffer);
      if (rund(3)) rloc(mtmp);
    } else if (steal(mtmp)) {
      rloc(mtmp);
      mtmp->mflee_and_time |= M_FLEEING;
    }
    break;
  case 'n':
    if (!uwep && !uarm && !uarmh && !uarms && !uarmg) {
      StrPrintF(ScratchBuffer, "%s hits! (I hope you don't mind)",
		Monnam(mtmp));
      message(ScratchBuffer);
      you.uhp += rnd(7);
      if (!rund(7)) you.uhpmax++;
      if (you.uhp > you.uhpmax) you.uhp = you.uhpmax;
      flags.botl = BOTL_HP;
      if (!rund(50)) rloc(mtmp);
    } else {
      hit_you(mtmp, dice(2,6));
      hit_you(mtmp, dice(2,6));
    }
    break;
  case 'o':
    tmp = hit_you(mtmp, rnd(6));
    if (hit_you(mtmp, rnd(6)) && tmp &&	/* hits with both paws */
	!you.ustuck && rund(2)) {
      you.ustuck = mtmp;
      kludge("%s has grabbed you!", "The owlbear");
      you.uhp -= dice(2,8);
    } else if (you.ustuck == mtmp) {
      you.uhp -= dice(2,8);
      message("You are being crushed.");
    }
    break;
  case 'P':
    if (ctmp && !rund(4))
      justswld(mtmp, "The purple worm");
    else
      hit_you(mtmp, dice(2,4));
    break;
  case 'Q':
    hit_you(mtmp, rnd(2));
    hit_you(mtmp, rnd(2));
    break;
  case 'R':
    //    if (tmp && uarmh && !(uarmh->bitflags & O_IS_RUSTFREE) &&
    // Bugfix from 1980s:
    if (ctmp && uarmh && !(uarmh->bitflags & O_IS_RUSTFREE) &&
       (Short) uarmh->spe >= -1) {
      message("Your helmet rusts!");
      uarmh->spe--;
    } else
      if (ctmp && uarm && !(uarm->bitflags & O_IS_RUSTFREE) &&/* Mike Newton */
	 uarm->otype < STUDDED_LEATHER_ARMOR &&
	 (Short) uarm->spe >= -1) {
	message("Your armor rusts!");
	uarm->spe--;
      }
    break;
  case 'S':
    if (ctmp && !rund(8)) {
      poisoned("snake's bite", mon_names + mdat->mname_offset);
    }
    break;
  case 's':
    //    if (tmp && !rund(8)) {
    // Bugfix from 1980s:
    if (ctmp && !rund(8)) {
      poisoned("scorpion's sting", mon_names + mdat->mname_offset);
    }
    hit_you(mtmp, rnd(8));
    hit_you(mtmp, rnd(8));
    break;
  case 'T':
    hit_you(mtmp, rnd(6));
    hit_you(mtmp, rnd(6));
    break;
  case 't':
    if (!rund(5)) rloc(mtmp);
    break;
  case 'u':
    mtmp->mflee_and_time |= M_FLEEING;
    break;
  case 'U':
    hit_you(mtmp, dice(3,4));
    hit_you(mtmp, dice(3,4));
    break;
  case 'v':
    if (ctmp && !you.ustuck) you.ustuck = mtmp;
    break;
  case 'V':
    if (tmp) you.uhp -= 4;
    if (ctmp) losexp();
    break;
  case 'W':
    if (ctmp) losexp();
    break;
#ifndef NOWORM
  case 'w':
    if (tmp) wormhit(mtmp);
#endif NOWORM
    break;
  case 'X':
    hit_you(mtmp, rnd(5));
    hit_you(mtmp, rnd(5));
    hit_you(mtmp, rnd(5));
    break;
  case 'x':
    {
      Long side = rund(2) ? RIGHT_SIDE : LEFT_SIDE;
      StrPrintF(ScratchBuffer, "%s pricks in your %s leg!",
		Monnam(mtmp), (side == RIGHT_SIDE) ? "right" : "left");
      message(ScratchBuffer);
      set_wounded_legs(side, rnd(50));
      losehp_m(2, mtmp);
      break;
    }
  case 'y':
    if (mtmp->bitflags & M_IS_CANCELLED) break;
    mondead(mtmp);
    if (!Blind) {
      message("You are blinded by a blast of light!");
      Blind = dice(4,12);
      seeoff(0);
    }
    return true;
  case 'Y':
    hit_you(mtmp, rnd(6));
    break;
  }
  if (you.uhp < 1) done_in_by(mtmp); // xxx
  return false;
}


/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
extern Boolean stop_occupation_now;
Boolean hit_you(monst_t *mtmp, Short dam) // was hitu
{
  Short tmp, res;

  nomul(0);
  if (you.uswallow) return false;

  if ((mtmp->bitflags & M_IS_HIDER) &&
      (mtmp->bitflags & M_IS_UNDETECTED)) {
    mtmp->bitflags &= ~M_IS_UNDETECTED;
    if (!Blind) {
      obj_t *obj;
      if ((obj = obj_at(mtmp->mx,mtmp->my))) {
	StrPrintF(ScratchBuffer, "%s was hidden under %s!",
		  Xmonnam(mtmp), doname(obj));
	message(ScratchBuffer);
      }
    }
  }

  tmp = you.uac;
  /* give people with Ac = -10 at least some vulnerability */
  if (tmp < 0) {
    dam += tmp;		/* decrease damage */
    if (dam <= 0) dam = 1;
    tmp = -rund(-tmp);
  }
  tmp += mtmp->data->mlevel;

  if (multi < 0) tmp += 4; /* XXXXX Decrease AC when you can't move
  / * XXXXX Ooops.  I took out all the negative multi's.  Need them back! */

  if ((Invis && mtmp->data->mlet != 'I') ||
      !(mtmp->mcansee_and_blinded & M_CAN_SEE))
    tmp -= 2;
  if (mtmp->bitflags & M_IS_TRAPPED) tmp -= 2;
  if (tmp <= rnd(20)) {
    if (Blind) message("It misses.");
    else {
      StrPrintF(ScratchBuffer, "%s misses.", Monnam(mtmp));
      message(ScratchBuffer);
    }
    res = false;
  } else {
    if (Blind) message("It hits!");
    else {
      StrPrintF(ScratchBuffer, "%s hits!", Monnam(mtmp));
      message(ScratchBuffer);
    }
    losehp_m(dam, mtmp);
    res = true;
  }
  stop_occupation_now = true;// stop_occupation(); // hmmmmm.
  return(res);
}
