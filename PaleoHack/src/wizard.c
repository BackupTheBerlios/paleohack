/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
#include "paleohackRsc.h"
static void aggravate() SEC_4;
static void clonewiz(monst_t *mtmp) SEC_4;

#define	WIZSHOT	    6	/* one chance in WIZSHOT that wizard will try magic */
#define	BOLT_LIM    8	/* from this distance D and 1 will try to hit you */
Char wizapp[] = "@DNPTUVXcemntx";

void amulet()
{
  obj_t *otmp;
  monst_t *mtmp;

  if (!flags.made_amulet || !flags.no_of_wizards)
    return;
  /* find wizard, and wake him if necessary */
  for (mtmp = fmon ; mtmp ; mtmp = mtmp->nmon)
    if (mtmp->data->mlet == '1' && (mtmp->bitflags & M_IS_ASLEEP) && !rund(40))
      for (otmp = invent ; otmp ; otmp = otmp->nobj)
	if (otmp->olet == AMULET_SYM && !otmp->spe) {
	  mtmp->bitflags &= ~M_IS_ASLEEP;
	  if (dist(mtmp->mx,mtmp->my) > 2)
	    message("You get the creepy feeling that somebody noticed your taking the Amulet.");
	  return;
	}
}

Boolean wiz_hit(monst_t *mtmp)
{
  /* if we have stolen or found the amulet, we disappear */
  if (mtmp->minvent && mtmp->minvent->olet == AMULET_SYM &&
      mtmp->minvent->spe == 0) {
    /* vanish -- very primitive */
    fall_down(mtmp);
    return true;
  }

  /* if it is lying around someplace, we teleport to it */
  if (!carrying(AMULET_OF_YENDOR)) {
    obj_t *otmp;

    for (otmp = fobj ; otmp ; otmp = otmp->nobj)
      if (otmp->olet == AMULET_SYM && !otmp->spe) {
	if ((you.ux != otmp->ox || you.uy != otmp->oy) &&
	    !mon_at(otmp->ox, otmp->oy)) {

	  /* teleport to it and pick it up */
	  mtmp->mx = otmp->ox;
	  mtmp->my = otmp->oy;
	  unlink_obj(otmp);
	  mpickobj(mtmp, otmp);
	  pmon(mtmp);
	  return false;
	}
	goto hit_him;
      }
    return false;				/* we don't know where it is */
  }

 hit_him:
  if (rund(2)) {				/* hit - perhaps steal */
    /* if hit, 1 in 20 steals amulet & vanish - amulet is on level 26 again. */
    if (hit_you(mtmp, dice(mtmp->data->damn,mtmp->data->damd)) && !rund(20))
      stealamulet(mtmp);
  } else
    inrange(mtmp);			/* try magic */
  return false;
}


#define sgn(a) (((a) > 0) ? 1 : ((a) == 0) ? 0 : -1)

void inrange(monst_t *mtmp)
{
  Int8 tx,ty;

  /* do nothing if cancelled (but make '1' say something) */
  if (mtmp->data->mlet != '1' && (mtmp->bitflags & M_IS_CANCELLED))
    return;

  /* spit fire only when both in a room or both in a corridor */
  if (inroom(you.ux,you.uy) != inroom(mtmp->mx,mtmp->my)) return;
  tx = you.ux - mtmp->mx;
  ty = you.uy - mtmp->my;
  if ((!tx && abs(ty) < BOLT_LIM) || (!ty && abs(tx) < BOLT_LIM)
      || (abs(tx) == abs(ty) && abs(tx) < BOLT_LIM)) {
    switch(mtmp->data->mlet) {
    case 'D':
      /* spit fire in the direction of @ (not nec. hitting) */
      buzz(-1, mtmp->mx, mtmp->my, sgn(tx), sgn(ty));
      break;
    case '1':
      if (rund(WIZSHOT)) break;
      /* if you zapped wizard with wand of cancellation,
	 he has to shake off the effects before he can throw
	 spells successfully.  1/2 the time they fail anyway */
      if ((mtmp->bitflags & M_IS_CANCELLED) || rund(2)) {
	if (canseemon(mtmp)) {
	  StrPrintF(ScratchBuffer, "%s makes a gesture, then curses.",
		    Monnam(mtmp));
	  message(ScratchBuffer);
	} else
	  message("You hear mumbled cursing.");
	if (!rund(3)) {
	  mtmp->mspeed = 0;
	  mtmp->bitflags &= ~M_IS_INVISIBLE;
	}
	if (!rund(3))
	  mtmp->bitflags &= ~M_IS_CANCELLED;
      } else {
	if (canseemon(mtmp)){
	  if (!rund(6) && !Invis) {
	    StrPrintF(ScratchBuffer, "%s hypnotizes you.", Monnam(mtmp));
	    message(ScratchBuffer);
	    nomul(rund(3) + 3);
	    break;
	  } else {
	    StrPrintF(ScratchBuffer, "%s chants an incantation.",
		      Monnam(mtmp));
	    message(ScratchBuffer);
	  }
	} else
	  message("You hear a mumbled incantation.");
	switch(rund(Invis ? 5 : 6)) {
	case 0:
	  /* create a nasty monster from a deep level */
	  /* (for the moment, 'nasty' is not implemented) */
	  makemon(NULL, you.ux, you.uy);
	  break;
	case 1:
	  message("\"Destroy the thief, my pets!\"");
	  aggravate();	/* aggravate all the monsters */
	  /* fall into next case */
	case 2:
	  if (flags.no_of_wizards == 1 && rnd(5) == 0)
	    /* if only 1 wizard, clone himself */
	    clonewiz(mtmp);
	  break;
	case 3: // haste!
	  if (mtmp->mspeed == MSLOW)
	    mtmp->mspeed = 0;
	  else
	    mtmp->mspeed = MFAST;
	  break;
	case 4: // invis!
	  mtmp->bitflags |= M_IS_INVISIBLE;
	  break;
	case 5:
	  /* Only if not Invisible */
	  message("You hear a clap of thunder!");
	  /* shoot a bolt of fire or cold, or a sleep ray */
	  buzz(-rnd(3),mtmp->mx,mtmp->my,sgn(tx),sgn(ty));
	  break;
	}
      }
    }
    if (you.uhp < 1) done_in_by(mtmp);// xxx
  }
}


static void aggravate()
{
  monst_t *mtmp;

  for (mtmp = fmon ; mtmp ; mtmp = mtmp->nmon) {
    mtmp->bitflags &= ~M_IS_ASLEEP;
    if ((mtmp->bitflags & M_IS_FROZEN) && !rund(5))
      mtmp->bitflags &= ~M_IS_FROZEN;
  }
}


static void clonewiz(monst_t *mtmp)
{
  monst_t *mtmp2;

  if ((mtmp2 = makemon(PM_WIZARD, mtmp->mx, mtmp->my))) {
    flags.no_of_wizards = 2;
    unpmon(mtmp2);
    mtmp2->mappearance = wizapp[rund(sizeof(wizapp)-1)];
    pmon(mtmp);
  }
}
