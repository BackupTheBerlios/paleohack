/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
static void walkfrom_iter(Short x, Short y) SEC_1;
static Boolean walkfrom_helper(Short *xp, Short *yp) SEC_1;
static void move(Short *x, Short *y, Short dir) SEC_1;
static Boolean okay(Short x, Short y, Short dir) SEC_1;


// currently in make_level.c ...

void makemaz()
{
  Short x,y;
  Short zx,zy;
  PointType mm;
  Boolean amul_level = (dlevel >= 30 && !flags.made_amulet);
  UChar typ;

  for (x = 2; x < DCOLS-1; x++)
    for (y = 2; y < DROWS-1; y++) {
      typ = ((x % 2) && (y % 2)) ? 0 : HWALL;
      set_cell_type(floor_info[x][y], typ);
    }

  if (amul_level) {
    monst_t *mtmp;

    zx = 2*(DCOLS/4) - 1;
    zy = 2*(DROWS/4) - 1;
    for (x = zx-2; x < zx+4; x++)
      for (y = zy-2; y <= zy+2; y++) {
	typ = ((y == zy-2 || y == zy+2 || x == zx-2 || x == zx+3) ? POOL :
	       ((y == zy-1 || y == zy+1 || x == zx-1 || x == zx+2) ? HWALL:
		ROOM));
	set_cell_type(floor_info[x][y], typ);
      }

    mkobj_at(AMULET_SYM, zx, zy);

    flags.made_amulet = true;

    walkfrom_iter(zx+4, zy);
    if ((mtmp = makemon(PM_HELLHOUND, zx, zy)))
      mtmp->bitflags |= M_IS_ASLEEP;
    if ((mtmp = makemon(PM_WIZARD, zx+1, zy))) {
      mtmp->bitflags |= M_IS_ASLEEP;
      flags.no_of_wizards = 1;
    }
  } else {
    mm = mazexy();
    zx = mm.x;
    zy = mm.y;
    walkfrom_iter(zx,zy);
    mksobj_at(WAN_WISHING, zx, zy);
    mkobj_at(ROCK_SYM, zx, zy);	/* put a rock on top of it */
  }

  for (x = 2; x < DCOLS-1; x++)
    for (y = 2; y < DROWS-1; y++) {
      typ = get_cell_type(floor_info[x][y]);
      switch(typ) {
      case HWALL:
	floor_symbol[x][y] = HWALL_SYM;
	break;
      case ROOM:
	floor_symbol[x][y] = ROOM_SYM;
	break;
      }
      // What, no VWALLs?
    }

  // make 11-18.. um.. things.
  for (x = rund(8)+11; x>0; x--) {
    mm = mazexy();
    mkobj_at((rund(2) ? GEM_SYM : 0), mm.x, mm.y);
  }
  // make 2-11 rocks
  for (x = rund(10)+2; x>0; x--) {
    mm = mazexy();
    mkobj_at(ROCK_SYM, mm.x, mm.y);
  }
  // make minotaur and 7-11 other dudes
  mm = mazexy();
  makemon(PM_MINOTAUR, mm.x, mm.y);
  for (x = rund(5)+7; x>0; x--) {
    mm = mazexy();
    makemon(NULL, mm.x, mm.y);
  }
  // make 7-12 golds
  for (x = rund(6)+7; x>0; x--) {
    mm = mazexy();
    mkgold(0L,mm.x,mm.y);
  }
  // make 7-12 traps
  for (x = rund(6)+7; x; x--)
    mktrap(0,1,NULL); // this is actually in make_level.c and not trap.c

  mm = mazexy();
  xupstair = mm.x;
  yupstair = mm.y;
  floor_symbol[xupstair][yupstair] = UPSTAIR_SYM;
  set_cell_type(floor_info[xupstair][yupstair], STAIRS);
  xdnstair = ydnstair = 0;
}


// Aiee!  Recursion!
// Ok, I *think* this will work, but not totally sure.
// also, TEST it and see if BIGNUM is big enough.
// 200 was too small, once or twice.  I think 300 will be ok.
#define BIGNUM 300
UChar my_stack_ptr; // points to the first "empty" space on stack.
UChar my_stack_x[BIGNUM];
UChar my_stack_y[BIGNUM];
static void walkfrom_iter(Short x, Short y)
{
  my_stack_ptr = 0;
  set_cell_type(floor_info[x][y], ROOM);
  while (true) {
    if (walkfrom_helper(&x, &y)) {
      // "recurse"
      my_stack_x[my_stack_ptr] = x;
      my_stack_y[my_stack_ptr] = y;
      my_stack_ptr++;
      /*
      { // Here's how I estimate whether BIGNUM is big enough:
	Char buf[20];
	StrPrintF(buf, "%d_____", my_stack_ptr);
	WinDrawChars(buf, StrLen(buf), 30, 30);
      }
      */
      if (my_stack_ptr >= BIGNUM) {
	message("DEBUG: bignum is too small!");
	return;
      }
      set_cell_type(floor_info[x][y], ROOM);
    } else {
      // this branch hit bottom; pop "call" from stack (if any remains)
      if (my_stack_ptr <= 0)
	return; // we hit bottom!  yay.
      my_stack_ptr--;
      x = my_stack_x[my_stack_ptr];
      y = my_stack_y[my_stack_ptr];
    }
  }
}

static Boolean walkfrom_helper(Short *xp, Short *yp)
{
  Short q=0, a, dir, dirs[4];

  // first, find all of the directions that are "ok" to move two hops in.
  for (a = 0, q = 0 ; a < 4 ; a++)
    if (okay(*xp, *yp, a))
      dirs[q++]= a;
  if (q == 0)
    return false;

  // select one of the valid directions, at random.  do some walking.
  dir = dirs[rund(q)];
  move(xp, yp, dir);
  set_cell_type(floor_info[*xp][*yp], ROOM);
  move(xp, yp, dir);
  return true;
}

/*  This is the (more or less) original recursive version.
    PalmOS stack is VERY SMALL so we must rewrite.  see above.
static void walkfrom(Short x, Short y)
{
  Short q,a,dir;
  Short dirs[4];
  set_cell_type(floor_info[x][y], ROOM);
  while (true) {
    q = 0;
    // first, find all of the directions that are "ok" to move two hops in.
    for (a = 0; a < 4; a++)
      if (okay(x,y,a)) dirs[q++]= a;
    if (!q) return; // we terminate this call if no directions are ok
    // select one of the valid directions, at random.
    dir = dirs[rund(q)];
    move(&x,&y,dir);
    set_cell_type(floor_info[x][y], ROOM);
    move(&x,&y,dir);
    walkfrom(x,y);
  }
}
*/

static void move(Short *x, Short *y, Short dir)
{
  switch(dir){
  case 0: --(*y); break; // north
  case 1: (*x)++; break; // east
  case 2: (*y)++; break; // south
  case 3: --(*x); break; // west
  }
}

// make sure the square two spaces in 'dir' direction is:
//   not out of bounds,
//   not already spoken for.
static Boolean okay(Short x, Short y, Short dir)
{
  move(&x,&y,dir);
  move(&x,&y,dir);
  if (x<3 || y<3 || x>DCOLS-3 || y>DROWS-3 ||
      get_cell_type(floor_info[x][y]) != 0)
    return false;
  else
    return true;
}

// return a fairly random point..
PointType mazexy()
{
  PointType mm;
  mm.x = 3 + 2*rund(DCOLS/2 - 2);
  mm.y = 3 + 2*rund(DROWS/2 - 2);
  return mm;
}
