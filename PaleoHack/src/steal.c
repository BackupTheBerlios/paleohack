/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
#include "paleohackRsc.h"

extern Short multi; // living in movesee.c right now..
extern Boolean stop_occupation_now;

static void stealarm() SEC_4;

/* actually returns something that fits in an int */
Long somegold()
{
  return( (you.ugold < 100) ? you.ugold :
	  (you.ugold > 10000) ? rnd(10000) : rnd((Int) you.ugold) );
}

void stealgold(monst_t *mtmp)
{
  gold_t *gold = gold_at(you.ux, you.uy);
  Long tmp;
  if (gold && ( !you.ugold || gold->amount > you.ugold || !rund(5))) {
    mtmp->mgold += gold->amount;
    freegold(gold);
    if (Invisible) newsym(you.ux, you.uy);
    StrPrintF(ScratchBuffer,
	      "%s quickly snatches some gold from between your feet!",
	      Monnam(mtmp));
    message(ScratchBuffer);
    if (!you.ugold || !rund(5)) {
      rloc(mtmp);
      mtmp->mflee_and_time |= M_FLEEING;
    }
  } else if (you.ugold) {
    you.ugold -= (tmp = somegold());
    message("Your purse feels lighter.");
    mtmp->mgold += tmp;
    rloc(mtmp);
    mtmp->mflee_and_time |= M_FLEEING;
    flags.botl = BOTL_GOLD;
  }
}
/* steal armor after he finishes taking it off */
UInt stealoid;		/* object to be stolen */
UInt stealmid;		/* monster doing the stealing */
static void stealarm()
{
  monst_t *mtmp;
  obj_t *otmp;

  for (otmp = invent ; otmp ; otmp = otmp->nobj)
    if (otmp->o_id == stealoid) {
      for (mtmp = fmon ; mtmp ; mtmp = mtmp->nmon)
	if (mtmp->m_id == stealmid) {
	  if (dist(mtmp->mx, mtmp->my) < 3) {
	    unlink_inv(otmp);
	    StrPrintF(ScratchBuffer, "%s steals %s!",
		      Monnam(mtmp), doname(otmp));
	    message(ScratchBuffer);
	    mpickobj(mtmp, otmp);
	    mtmp->mflee_and_time |= M_FLEEING;
	    rloc(mtmp);
	  }
	  break;
	}
      break;
    }
  stealoid = 0;
}

/* returns 1 when something was stolen */
/* (or at least, when N should flee now) */
/* avoid stealing the object stealoid */
Boolean steal(monst_t *mtmp) // xxx not implemented yet (missing stealarm)
{
  obj_t *otmp;
  Short tmp;
  Short named = 0;

  if (!invent) {
    if (Blind)
      message("Somebody tries to rob you, but finds nothing to steal.");
    else {
      StrPrintF(ScratchBuffer,
		"%s tries to rob you, but she finds nothing to steal!",
		Monnam(mtmp));
      message(ScratchBuffer);
    }
    return true;	/* let her flee */
  }
  tmp = 0;
  for (otmp = invent ; otmp ; otmp = otmp->nobj)
    if (otmp != uarm2)
      tmp += ((otmp->owornmask & (W_ARMOR | W_RING)) ? 5 : 1);
  tmp = rund(tmp);
  for (otmp = invent ; otmp ; otmp = otmp->nobj)
    if (otmp != uarm2)
      if ((tmp -= ((otmp->owornmask & (W_ARMOR | W_RING)) ? 5 : 1)) < 0)
	break;
  if (!otmp) {
    message("BUG: Steal fails!");
    return false;
  }
  if (otmp->o_id == stealoid)
    return false;
  if ((otmp->owornmask & (W_ARMOR | W_RING))) {
    switch(otmp->olet) {
    case RING_SYM:
      ringoff(otmp);
      break;
    case ARMOR_SYM:
      if (multi < 0 || otmp == uarms){
	setworn(NULL, otmp->owornmask & W_ARMOR);
	break;
      }
      { Short curssv = (0 != (otmp->bitflags & O_IS_CURSED));
      otmp->bitflags &= ~O_IS_CURSED;
      stop_occupation_now = true;//stop_occupation();
      StrPrintF(ScratchBuffer, "%s seduces you and %s off your %s.",
		Amonnam(mtmp, Blind ? "gentle" : "beautiful"),
		curssv ? "helps you to take" : "you start taking",
		(otmp == uarmg) ? "gloves" :
		(otmp == uarmh) ? "helmet" : "armor");
      message(ScratchBuffer);
      named++;
      armoroff(otmp);
      if (curssv) otmp->bitflags |= O_IS_CURSED;
      if (multi < 0) { // this was commented out:
				/*
				  multi = 0;
				  nomovemsg = 0;
				  afternmv = 0;
				*/
	stealoid = otmp->o_id;
	stealmid = mtmp->m_id;
	spin_multi("");
	stealarm();
	return false;
      }
      break;
      }
    default:
      message("BUG: Tried to steal a strange worn thing.");
      return false;
    }
  }
  else if (otmp == uwep)
    setuwep(NULL);
  if (otmp->olet == CHAIN_SYM) {
    message("BUG: How come you are carrying that chain?");
  }
  if (Punished && otmp == uball) {
    Punished = 0;
    unlink_obj(uchain);
    free_me((VoidPtr) uchain);
    uchain = NULL;
    uball->spe = 0;
    uball = NULL;	/* superfluous */
  }
  unlink_inv(otmp);
  StrPrintF(ScratchBuffer, "%s stole %s.",
	    named ? "She" : Monnam(mtmp), doname(otmp));
  message(ScratchBuffer);
  mpickobj(mtmp,otmp);
  return((multi < 0) ? 0 : 1);
}

void mpickobj(monst_t *mtmp, obj_t *otmp)
{
  otmp->nobj = mtmp->minvent;
  mtmp->minvent = otmp;
}

Boolean stealamulet(monst_t *mtmp)
{
  obj_t *otmp;

  for (otmp = invent ; otmp ; otmp = otmp->nobj) {
    if (otmp->olet == AMULET_SYM) {
      /* might be an imitation one */
      if (otmp == uwep) setuwep(NULL);
      unlink_inv(otmp);
      mpickobj(mtmp, otmp);
      StrPrintF(ScratchBuffer, "%s stole %s!", Monnam(mtmp), doname(otmp));
      message(ScratchBuffer);
      return true;
    }
  }
  return false;
}

/* release the objects the killed animal has stolen */
void release_objs(monst_t *mtmp, Boolean show)
{
  obj_t *otmp, *otmp2;

  for (otmp = mtmp->minvent ; otmp ; otmp = otmp2) {
    otmp->ox = mtmp->mx;
    otmp->oy = mtmp->my;
    otmp2 = otmp->nobj;
    otmp->nobj = fobj;
    fobj = otmp;
    stackobj(fobj);
    if (show & cansee(mtmp->mx,mtmp->my))
      print(otmp->ox,otmp->oy,otmp->olet);
  }
  mtmp->minvent = NULL;
  if (mtmp->mgold || mtmp->data->mlet == 'L') {
    Long tmp;

    tmp = (mtmp->mgold > 10000) ? 10000 : mtmp->mgold;
    mkgold((Long)(tmp + dice(dlevel,30)), mtmp->mx, mtmp->my);
    if (show & cansee(mtmp->mx,mtmp->my))
      print(mtmp->mx, mtmp->my, GOLD_SYM);
  }
}
