/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
#include "paleohackRsc.h"

extern room_t rooms[MAX_ROOMS]; // in make_level.c .... SIGH...
extern Short engrave_or_what;

static monst_t *guard;
static UInt gdlevel;

#define	FCSIZ	(DROWS + DCOLS)
//struct fakecorridor {
//  UChar fx, fy, ftyp;
//};
typedef struct eguard {
  Short fcbeg, fcend;	/* fcend: first unused pos of fake corridor */
  UChar gdx, gdy;	/* goal of guard's walk */
  Boolean gddone;
  //struct fakecorridor fakecorr[FCSIZ]; // shee-it.  that's (80+22)*4
  // it should have less padding this way, (80+22)*3 = 306.  still a lot..
  UChar fx[FCSIZ], fy[FCSIZ], ftyp[FCSIZ];   // Fake Corridor.
} eguard_t;
#define EGUARD        ( (eguard_t *)((guard)->extra) )


static void restfakecorr() SEC_4;
static Boolean goldincorridor() SEC_4;
static Boolean find_guard_goal(Short *gx, Short *gy) SEC_4;
static Short gd_proceed(Short nx, Short ny, UChar typ) SEC_4;
static Short gd_newpos(Short nx, Short ny) SEC_4;

static void restfakecorr()
{
  Short fcx, fcy, fcbeg;
  //  struct rm *crm;

  while ((fcbeg = EGUARD->fcbeg) < EGUARD->fcend) {
    fcx = EGUARD->fx[fcbeg];
    fcy = EGUARD->fy[fcbeg];
    if ((you.ux == fcx && you.uy == fcy) || cansee(fcx,fcy) || mon_at(fcx,fcy))
      return;
    // crm = &levl[fcx][fcy];
    set_cell_type(floor_info[fcx][fcy], EGUARD->ftyp[fcbeg]);
    if (!EGUARD->ftyp[fcbeg]) floor_info[fcx][fcy] &= ~SEEN_CELL;//crm->seen=0;
    newsym(fcx,fcy);
    EGUARD->fcbeg++;
  }
  /* it seems he left the corridor - let the guard disappear */
  mondead(guard);
  guard = NULL;
}

static Boolean goldincorridor()
{
  Short fci;

  for (fci = EGUARD->fcbeg ; fci < EGUARD->fcend ; fci++)
    if (gold_at(EGUARD->fx[fci], EGUARD->fy[fci]))
      return true;
  return false;
}

void setgd()
{
  monst_t *mtmp;
  for (mtmp = fmon ; mtmp ; mtmp = mtmp->nmon)
    if (mtmp->bitflags & M_IS_GUARD) {
      guard = mtmp;
      gdlevel = dlevel;
      return;
    }
  guard = NULL;
}


static Boolean find_guard_goal(Short *gx, Short *gy)
{
  Short x, y, dd;
  for (dd = 1 ; (dd < DROWS || dd < DCOLS) ; dd++) {
    for (y = you.uy - dd ; y <= you.uy + dd ; y++) {
      if (y < 0 || y > DROWS-1) continue;
      for (x = you.ux - dd ; x <= you.ux + dd ; x++) {
	if (y != you.uy - dd && y != you.uy + dd && x != you.ux - dd)
	  x = you.ux + dd; // check only the (expanding) edges of the square.
	if (x < 0 || x > DCOLS-1) continue;
	if (get_cell_type(floor_info[x][y]) == CORR) {
	  *gx = x; *gy = y; return true;
	}

      }
    }
  }
  return false;
}

void invault()
{
  Short tmp = inroom(you.ux, you.uy);
  if (tmp < 0 || rooms[tmp].rtype != VAULT) {
    you.uinvault = 0;
    return;
  }
  // every 50 consecutive turns that you spend in a vault, make a guard.
  if ((++you.uinvault % 50 == 0) && (!guard || gdlevel != dlevel)) {
    //    Char buf[BUFSZ];
    Short x, y, gx, gy;

    /* first find the goal for the guard */
    // search edges of a square around you; expand, repeat until corridor found
    if (!find_guard_goal(&gx, &gy)) {
      message("BUG: Not a single corridor on this level??");
      tele();
      return;
    }

    /* next find a good place for a door in the wall */
    x = you.ux; y = you.uy;
    while (get_cell_type(floor_info[x][y]) == ROOM) {
      Short dx,dy;

      dx = (gx > x) ? 1 : (gx < x) ? -1 : 0;
      dy = (gy > y) ? 1 : (gy < y) ? -1 : 0;
      if (abs(gx-x) >= abs(gy-y))
	x += dx;
      else
	y += dy;
    }

    /* make something interesting happen */
    if (!(guard = makemon(PM_GUARD, x, y))) return;
    guard->bitflags |= (M_IS_GUARD | M_IS_PEACEFUL);
    // allocate an "eguard" and point "extra" to it.
    guard->extra = (void *) md_malloc(sizeof(eguard_t));  // Dangerous!
    guard->extra_len = sizeof(eguard_t);
    // When monsters are freed, we must make sure to free 'extra' also
    // iff it is not NULL!
    EGUARD->gddone = false;
    gdlevel = dlevel;
    if (!cansee(guard->mx, guard->my)) {
      mondead(guard);
      guard = NULL;
      return; // well... so much for that.
    }

    message("Suddenly one of the Vault's guards enters!");
    pmon(guard);
    engrave_or_what = GET_VAULT;
    FrmPopupForm(EngraveForm);
    // TO be continued in do_vault ... stay tuned
    // oh maybe I can do this here first.....
    EGUARD->gdx = gx;
    EGUARD->gdy = gy;
    EGUARD->fcbeg = 0;
    EGUARD->fx[0] = x;
    EGUARD->fy[0] = y;
    EGUARD->ftyp[0] = get_cell_type(floor_info[x][y]);
    //    set_cell_type(floor_info[x][y], DOOR);
    EGUARD->fcend = 1;
  }
}

void do_vault(Char *buf)
{
  if (buf && (!StrCompare(buf, "Croesus") || !StrCompare(buf, "Kroisos"))) {
    message("\"Oh, yes - of course. Sorry to have disturbed you.\"");
    mondead(guard); // conveniently, guard was a global already.
    guard = NULL;
    return;
  }
  //    clrlin();
  message("\"I don't know you.\"");
  if (!you.ugold)
    message("\"Please follow me.\"");
  else {
    message("\"Most likely all that gold was stolen from this vault.\"");
    message("\"Please drop your gold (say D$ ) and follow me.\"");
  }
  set_cell_type(floor_info[EGUARD->fx[0]][EGUARD->fy[0]], DOOR);
}


Short gd_move()
{
  Short x,y,dx,dy,gx,gy,nx,ny;
  UChar typ, fc_cell;
  //  struct fakecorridor *fcp;
  //  struct rm *crm;
  if (!guard || gdlevel != dlevel) {
    message("BUG: Where is the guard?");
    return 2;	/* died */
  }
  if (you.ugold || goldincorridor())
    return 0;	/* didnt move */
  if (dist(guard->mx, guard->my) > 1 || EGUARD->gddone) {
    restfakecorr();
    return 0;	/* didnt move */
  }
  x = guard->mx;
  y = guard->my;
  /* look around (hor & vert only) for accessible places */
  for (nx = x-1 ; nx <= x+1 ; nx++)
    for (ny = y-1 ; ny <= y+1 ; ny++) {
      if (OUT_OF_BOUNDS(nx,ny)) continue;
      if ((nx == x && ny == y) || (nx != x && ny != y)) continue;
      typ = get_cell_type(floor_info[nx][ny]);
      if (!IS_WALL(typ) && typ != POOL) {
	Short i;
	for (i = EGUARD->fcbeg ; i < EGUARD->fcend ; i++)
	  if (EGUARD->fx[i] == nx && EGUARD->fy[i] == ny) 
	    goto next_fc_cell; // can't "continue" an outer loop... sigh
	if ((i = inroom(nx,ny)) >= 0 && rooms[i].rtype == VAULT) continue;
	/* seems we found a good place to leave him alone */
	EGUARD->gddone = true;
	if (ACCESSIBLE(typ)) {
	  return gd_newpos(nx, ny);
	}
	set_cell_type(floor_info[nx][ny], ((typ == SCORR) ? CORR : DOOR));
	return gd_proceed(nx, ny, typ);
      }
    next_fc_cell:
      ;
    }
  nx = x;
  ny = y;
  gx = EGUARD->gdx;
  gy = EGUARD->gdy;
  dx = (gx > x) ? 1 : (gx < x) ? -1 : 0;
  dy = (gy > y) ? 1 : (gy < y) ? -1 : 0;
  if (abs(gx-x) >= abs(gy-y)) nx += dx;
  else ny += dy;

  while (0 != (typ = get_cell_type( (fc_cell=floor_info[nx][ny]) ))) {
    /* in view of the above we must have IS_WALL(typ) or typ == POOL */
    /* must be a wall here */
    if (!OUT_OF_BOUNDS(nx+nx-x, ny+ny-y) && typ != POOL &&
	ZAP_POS(get_cell_type(floor_info[nx+nx-x][ny+ny-y]))) {
      set_cell_type(fc_cell, DOOR);
      return gd_proceed(nx, ny, typ);
    }
    if (dy && nx != x) {
      nx = x; ny = y+dy;
      continue;
    }
    if (dx && ny != y) {
      ny = y; nx = x+dx; dy = 0;
      continue;
    }
    /* I don't like this, but ... */ // and if HE doesn't like it, must be bad.
    set_cell_type(fc_cell, DOOR);
    return gd_proceed(nx, ny, typ);
  }
  set_cell_type(fc_cell, CORR);

  return gd_proceed(nx, ny, typ);
}
static Short gd_proceed(Short nx, Short ny, UChar typ)
{
  Short i;
  if (cansee(nx,ny)) {
    mnewsym(nx,ny);
    prl(nx,ny);
  }
  i = EGUARD->fcend;
  if (EGUARD->fcend++ == FCSIZ) {
    message("BUG: fakecorr overflow"); return 1; // sigh.  was panic.
  }
  EGUARD->fx[i] = nx;
  EGUARD->fy[i] = ny;
  EGUARD->ftyp[i] = typ;
  return gd_newpos(nx, ny);
}
static Short gd_newpos(Short nx, Short ny)
{
  if (EGUARD->gddone) nx = ny = 0;
  guard->mx = nx;
  guard->my = ny;
  pmon(guard);
  restfakecorr();
  return 1;
}


void gddead()
{
  guard = NULL;
}

void replgd(monst_t *mtmp, monst_t *mtmp2)
{
  if (mtmp == guard)
    guard = mtmp2;
}
