/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"

extern Char plname[PL_NSIZ];
extern permonst_t pm_ghost;

//Char bones[] = "bones_xx";

/* save bones and possessions of a deceased adventurer */
void savebones()
{
  //  Int fd;
  obj_t *otmp;
  trap_t *ttmp;
  monst_t *mtmp;
  if (dlevel <= 0 || dlevel > MAXLEVEL) return;
  if (!rund(1 + dlevel/2)) return;	/* not so many ghosts on low levels */
  

  // Check whether a bones exists for this dlevel,
  // and if so, don't create one.
  // (not implemented yet.  scan all 'bones' records.)

  /* drop everything; the corpse's possessions are usually cursed */
  otmp = invent;
  while (otmp) {
    otmp->ox = you.ux;
    otmp->oy = you.uy;
    otmp->age = 0;		/* very long ago */
    otmp->owornmask = 0;
    if (rund(5)) otmp->bitflags |= O_IS_CURSED;
    if (!otmp->nobj) {
      otmp->nobj = fobj;
      fobj = invent;    // XXXX leak.... I should free the fobj list first...
      invent = NULL;	/* superfluous */
      break;
    }
    otmp = otmp->nobj;
  }
  if (!(mtmp = makemon(PM_GHOST, you.ux, you.uy))) return;
  mtmp->mx = you.ux;
  mtmp->my = you.uy;
  mtmp->bitflags |= M_IS_ASLEEP;
  mtmp->name = (Char *) md_malloc((StrLen(plname) + 1) * sizeof(Char));
  StrCopy(mtmp->name, plname);
  mkgold(somegold() + dice(dlevel,30), you.ux, you.uy);
  // Reset monsters, traps, objects....
  for (mtmp = fmon ; mtmp ; mtmp = mtmp->nmon) {
    mtmp->m_id = 0;
    if (mtmp->bitflags & M_IS_TAME)
      mtmp->bitflags &= ~(M_IS_TAME | M_IS_PEACEFUL);
    mtmp->mlastmoved = 0;
    if (mtmp->bitflags & M_IS_DISPLAYED) unpmon(mtmp);
  }
  for (ttmp = ftrap ; ttmp ; ttmp = ttmp->ntrap)
    ttmp->trap_info &= ~SEEN_TRAP;
  for (otmp = fobj ; otmp ; otmp = otmp->nobj) {
    otmp->o_id = 0;
    /* otmp->o_cnt_id = 0; - superfluous */
    if (ONAME(otmp)) do_name(otmp, NULL);
    otmp->bitflags &= ~O_IS_KNOWN;
    otmp->invlet = 0;
    if (otmp->olet == AMULET_SYM && !otmp->spe) {
      otmp->spe = -1;      /* no longer the actual amulet */
      otmp->bitflags |= O_IS_CURSED;    /* flag as gotten from a ghost */
    }
  }

  savelev(dlevel, false);
}



Boolean getbones()
{
  Short x,y;

  if (rund(3)) return false;	/* only once in three times do we find bones */
  // XXXX  There should be a Version Check performed here to make sure
  //       that the phBonesDB has the same format as the phSaveDB...  xxxxx
  if (false) return false;

  if (!getlev(dlevel, false)) return false; // No bones for this level.

  // hm, maybe this should be done before saving it in the first place?
  for (x = 0 ; x < DCOLS ; x++)
    for (y = 0 ; y < DROWS ; y++)
      floor_info[x][y] &= ~(NEW_CELL | SEEN_CELL);

  return true;
}
