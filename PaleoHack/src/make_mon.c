/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"

Char fut_geno[MAX_GENOCIDE];
Char genocided[MAX_GENOCIDE]; // XXX these should get copied one to the other somewhere

//static Boolean getwn(monst_t *mtmp) { return false; } // fake...
//static void initworm(monst_t *mtmp) { return; } // fake...

/*
 * called with [x,y] = coordinates;
 *	[0,0] means anyplace
 *	[u.ux,u.uy] means: call mnexto (if !in_mklev)
 *
 *	In case we make an Orc or k!ller bee, we make an entire horde (swarm);
 *	note that in this case we return only one of them (the one at [x,y]).
 */
extern Boolean in_mklev;
monst_t * makemon(permonst_t *ptr, Short x, Short y)
{
  monst_t *mtmp;
  Short tmp, ct;
  Boolean got_mon = false, anything = (ptr == NULL);

  if ((x != 0 || y != 0) && (mon_at(x,y)))
    return NULL;
  if (ptr){
    if (StrChr(fut_geno, ptr->mlet))
      return NULL;
  } else {
    ct = CMNUM - StrLen(fut_geno);
    if (StrChr(fut_geno, 'm')) ct++;  /* make only 1 minotaur */
    if (StrChr(fut_geno, '@')) ct++;
    if (ct <= 0) return NULL; 		  /* no more monsters! */
    tmp = rund(ct*dlevel/24 + 7);
    if (tmp < dlevel - 4) tmp = rund(ct*dlevel/24 + 12);
    if (tmp >= ct) tmp = rund(ct - ct/2) + ct/2;
    for (ct = 0; ct < CMNUM; ct++){
      ptr = &mons[ct];
      if (StrChr(fut_geno, ptr->mlet))
	continue;
      if (!tmp--) {
	got_mon = true;
	break;
      }
    }
    if (!got_mon) {
      message("BUG in makemon");
      return NULL; //panic("makemon?");
    }
  }
  // ok, we should have a valid 'ptr' now

  mtmp = (monst_t *) md_malloc(sizeof(monst_t));// should add ptr->ptr->pxlth !

  // for (ct = 0; ct < ptr->pxlth; ct++) ((Char *) &(mtmp->mextra[0]))[ct] = 0;
  mtmp->extra = NULL;
  mtmp->extra_len = 0;
  mtmp->name = NULL;
  // Put at head of list:
  mtmp->nmon = fmon;
  fmon = mtmp;
  // set other stuff...
  mtmp->m_id = flags.ident++;
  mtmp->data = ptr;
  //  mtmp->mxlth = ptr->pxlth; // XXX
  if (ptr->mlet == 'D')  mtmp->mhpmax = mtmp->mhp = 80;
  else if (!ptr->mlevel) mtmp->mhpmax = mtmp->mhp = rnd(4);
  else                   mtmp->mhpmax = mtmp->mhp = dice(ptr->mlevel, 8);
  mtmp->mx = x;
  mtmp->my = y;
  mtmp->mcansee_and_blinded |= M_CAN_SEE;
  if (ptr->mlet == 'M'){
    mtmp->bitflags |= M_IS_MIMIC;
    mtmp->mappearance = ']';
  }
  if (!in_mklev) {
    if (x == you.ux && y == you.uy && ptr->mlet != ' ')
      mnexto(mtmp);
    if (x == 0 && y == 0)
      rloc(mtmp);
  }
  if (ptr->mlet == 's' || ptr->mlet == 'S') {
    mtmp->bitflags |= M_IS_HIDER | M_IS_UNDETECTED;
    if (in_mklev)
      if (mtmp->mx && mtmp->my)
	mkobj_at(0, mtmp->mx, mtmp->my);
  }
  if (ptr->mlet == ':') {
    mtmp->bitflags |= M_IS_CHAMELEON;
    newcham(mtmp, &mons[dlevel+14+rund(CMNUM-14-dlevel)]);
  }
  if (ptr->mlet == 'I' || ptr->mlet == ';')
    mtmp->bitflags |= M_IS_INVISIBLE;
  if (ptr->mlet == 'L' || ptr->mlet == 'N' ||
      (in_mklev && StrChr("&w;", ptr->mlet) && rund(5)))
    mtmp->bitflags |= M_IS_ASLEEP;

#ifndef NOWORM
  if (ptr->mlet == 'w' && getwn(mtmp))
    initworm(mtmp);
#endif NOWORM

  if (anything && (ptr->mlet == 'O' || ptr->mlet == 'k')) {
    //    coord enexto();
    Short cnt = rnd(10);
    PointType mm;
    mm.x = x;
    mm.y = y;
    while (cnt--) {
      mm = enexto(mm.x, mm.y);
      makemon(ptr, mm.x, mm.y);
    }
  }

  return mtmp;
}




// Gosh.  Words fail me.
PointType enexto(Int8 xx, Int8 yy)
{
  Int8 x,y;
  PointType foo[15], *tfoo;
  Short range;

  tfoo = foo;
  range = 1;
  do {	/* full kludge action. */
    for (x = xx-range; x <= xx+range; x++)
      if (goodpos(x, yy-range)) {
	tfoo->x = x;
	tfoo++->y = yy-range;
	if (tfoo == &foo[15]) goto foofull;
      }
    for (x = xx-range; x <= xx+range; x++)
      if (goodpos(x,yy+range)) {
	tfoo->x = x;
	tfoo++->y = yy+range;
	if (tfoo == &foo[15]) goto foofull;
      }
    for (y = yy+1-range; y < yy+range; y++)
      if (goodpos(xx-range,y)) {
	tfoo->x = xx-range;
	tfoo++->y = y;
	if (tfoo == &foo[15]) goto foofull;
      }
    for (y = yy+1-range; y < yy+range; y++)
      if (goodpos(xx+range,y)) {
	tfoo->x = xx+range;
	tfoo++->y = y;
	if (tfoo == &foo[15]) goto foofull;
      }
    range++;
  } while (tfoo == foo);
 foofull:
  return( foo[rund(tfoo-foo)] );
}



Boolean goodpos(Short x, Short y)	/* used only in mnexto and rloc */
{
  if (x < 1 || x > DCOLS-2 || y < 1 || y > DROWS-2)
    return false; // out of range
  if (mon_at(x,y))
    return false; // monster is there
  if (!ACCESSIBLE(get_cell_type(floor_info[x][y])))
    return false; // solid rock or something
  if (x == you.ux && y == you.uy)
    return false; // you're there
  if (sobj_at(ENORMOUS_ROCK, x, y))
    return false; // a boulder is there

  return true;
}



void rloc(monst_t *mtmp)
{
  Short tx,ty;
  Char ch = mtmp->data->mlet;

#ifndef NOWORM
  if (ch == 'w' && mtmp->mx) return;	/* do not relocate worms */
#endif NOWORM

  do {
    tx = rund(DCOLS-3) + 2;
    ty = rund(DROWS);
  } while (!goodpos(tx,ty));

  mtmp->mx = tx;
  mtmp->my = ty;
  if (you.ustuck == mtmp){
    if (you.uswallow) {
      you.ux = tx;
      you.uy = ty;
      refresh(); //      docrt();
    } else
      you.ustuck = NULL;//someone goofed if ustuck != NULL and uswallow = false
  }
  pmon(mtmp);
}


monst_t * mkmon_at(Char let, Short x, Short y)
{
  Short ct;
  permonst_t *ptr;

  for (ct = 0; ct < CMNUM; ct++) {
    ptr = &mons[ct];
    if (ptr->mlet == let)
      return makemon(ptr,x,y);
  }
  return NULL;
}
