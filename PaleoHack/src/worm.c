/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"

/*
This file has not been gone over thoroughly yet.
Also, there are some files that have dummy procedures compiled in,
in place of these.  do a grep for these function names in *.c.
*/

#ifndef NOWORM

wseg_t *wsegs[MAX_WORM];	/* linked list, tail first */
wseg_t *wheads[MAX_WORM];
Long wgrowtime[MAX_WORM];

static void remseg(wseg_t *wtmp) SEC_3;

Boolean getwn(monst_t *mtmp)
{
  Short tmp;
  for (tmp = 1 ; tmp < MAX_WORM ; tmp++)
    if (!wsegs[tmp]) {
      mtmp->wormno = tmp;
      return true;
  }
  return false;  /* level infested with worms */
}

/* called to initialize a worm unless cut in half */
void initworm(monst_t *mtmp)
{
  wseg_t *wtmp;
  Short tmp = mtmp->wormno;
  if (!tmp) return;
  wtmp = (wseg_t *) md_malloc(sizeof(wseg_t));
  wheads[tmp] = wsegs[tmp] = wtmp;
  wgrowtime[tmp] = 0;
  wtmp->wx = mtmp->mx;
  wtmp->wy = mtmp->my;
  /*	wtmp->wdispl = 0; */
  wtmp->nseg = 0;
}

void worm_move(monst_t *mtmp)
{
  wseg_t *wtmp, *whd;
  Short tmp = mtmp->wormno;
  wtmp = (wseg_t *) md_malloc(sizeof(wseg_t));
  wtmp->wx = mtmp->mx;
  wtmp->wy = mtmp->my;
  wtmp->nseg = 0;
  /*	wtmp->wdispl = 0; */
  (whd = wheads[tmp])->nseg = wtmp;
  wheads[tmp] = wtmp;
  if (cansee(whd->wx,whd->wy)) {
    unpmon(mtmp);
    print(whd->wx, whd->wy, '~');
    whd->wdispl = true;
  } else	whd->wdispl = false;
  if (wgrowtime[tmp] <= moves) {
    if (!wgrowtime[tmp]) wgrowtime[tmp] = moves + rnd(5);
    else wgrowtime[tmp] += 2+rnd(15);
    mtmp->mhpmax += 3;
    mtmp->mhp += 3;
    return;
  }
  whd = wsegs[tmp];
  wsegs[tmp] = whd->nseg;
  remseg(whd);
}

void worm_nomove(monst_t *mtmp)
{
  Short tmp;
  wseg_t *wtmp;
  tmp = mtmp->wormno;
  wtmp = wsegs[tmp];
  if (wtmp == wheads[tmp]) return;
  if (wtmp == 0 || wtmp->nseg == 0) {
    message("BUG at worm_nomove"); // was "panic!"
    return;
  }
  wsegs[tmp] = wtmp->nseg;
  remseg(wtmp);
  mtmp->mhp -= 3;	/* mhpmax not changed ! */
}

void wormdead(monst_t *mtmp)
{
  Short tmp = mtmp->wormno;
  wseg_t *wtmp, *wtmp2;
  if (!tmp) return;
  mtmp->wormno = 0;
  for (wtmp = wsegs[tmp] ; wtmp ; wtmp = wtmp2) {
    wtmp2 = wtmp->nseg;
    remseg(wtmp);
  }
  wsegs[tmp] = 0;
}

void wormhit(monst_t *mtmp)
{
  Short tmp = mtmp->wormno;
  wseg_t *wtmp;
  if (!tmp) return;	/* worm without tail */
  for (wtmp = wsegs[tmp] ; wtmp ; wtmp = wtmp->nseg)
    hit_you(mtmp, 1);
}

void wormsee(UInt tmp)
{
  wseg_t *wtmp = wsegs[tmp];
  if (!wtmp) {
    message("BUG in wormsee: wtmp==0");
    return;
  }
  for( ; wtmp->nseg ; wtmp = wtmp->nseg)
    if (!cansee(wtmp->wx,wtmp->wy) && wtmp->wdispl) {
      newsym(wtmp->wx, wtmp->wy);
      wtmp->wdispl = false;
    }
}

void pwseg(wseg_t *wtmp)
{
  if (!wtmp->wdispl) {
    print(wtmp->wx, wtmp->wy, '~');
    wtmp->wdispl = true;
  }
}

/* uwep->otyp or 0 */
void cutworm(monst_t *mtmp, Short x, Short y, UChar weptyp)
{
  wseg_t *wtmp, *wtmp2;
  monst_t *mtmp2;
  Short tmp,tmp2;
  if (mtmp->mx == x && mtmp->my == y) return;	/* hit headon */

  /* cutting goes best with axe or sword */
  tmp = rnd(20);
  if (weptyp == LONG_SWORD || weptyp == TWO_HANDED_SWORD ||
      weptyp == AXE) tmp += 5;
  if (tmp < 12) return;

  /* if tail then worm just loses a tail segment */
  tmp = mtmp->wormno;
  wtmp = wsegs[tmp];
  if (wtmp->wx == x && wtmp->wy == y){
    wsegs[tmp] = wtmp->nseg;
    remseg(wtmp);
    return;
  }

  /* cut the worm in two halves */
  mtmp2 = (monst_t *) md_malloc(sizeof(monst_t)); // mtmp2 = newmonst(0);
  *mtmp2 = *mtmp;
  //  mtmp2->mxlth = mtmp2->mnamelth = 0; // XXXX not implemented yet!

  /* sometimes the tail end dies */
  if (rund(3) || !getwn(mtmp2)){
    monfree(mtmp2);
    tmp2 = NULL;
  } else {
    tmp2 = mtmp2->wormno;
    wsegs[tmp2] = wsegs[tmp];
    wgrowtime[tmp2] = 0;
  }
  do {
    if (wtmp->nseg->wx == x && wtmp->nseg->wy == y){
      if (tmp2) wheads[tmp2] = wtmp;
      wsegs[tmp] = wtmp->nseg->nseg;
      remseg(wtmp->nseg);
      wtmp->nseg = 0;
      if (tmp2){
	message("You cut the worm in half.");
	mtmp2->mhpmax = mtmp2->mhp =
	  dice(mtmp2->data->mlevel, 8);
	mtmp2->mx = wtmp->wx;
	mtmp2->my = wtmp->wy;
	mtmp2->nmon = fmon;
	fmon = mtmp2;
	pmon(mtmp2);
      } else {
	message("You cut off part of the worm's tail.");
	remseg(wtmp);
      }
      mtmp->mhp /= 2;
      return;
    }
    wtmp2 = wtmp->nseg;
    if (!tmp2) remseg(wtmp);
    wtmp = wtmp2;
  } while (wtmp->nseg);
  message("BUG in cutworm: Cannot find worm segment");
}

static void remseg(wseg_t *wtmp)
{
  if (wtmp->wdispl)
    newsym(wtmp->wx, wtmp->wy);
  free_me((VoidPtr) wtmp); // XXXX XXXX XXXX
}
#endif NOWORM
