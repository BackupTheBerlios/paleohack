/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"

extern const Char *traps[TRAPNUM]; // in trap.c

Short findit()   /* returns number of things found */
{
  Short num;
  UChar zx,zy;
  trap_t *ttmp;
  monst_t *mtmp;
  UChar lx,hx,ly,hy;

  if (you.uswallow) return false;
  for (lx = you.ux;
       (num = get_cell_type(floor_info[lx-1][you.uy])) && num != CORR; lx--) ;
  for (hx = you.ux;
       (num = get_cell_type(floor_info[hx+1][you.uy])) && num != CORR; hx++) ;
  for (ly = you.uy;
       (num = get_cell_type(floor_info[you.ux][ly-1])) && num != CORR; ly--) ;
  for (hy = you.uy;
       (num = get_cell_type(floor_info[you.ux][hy+1])) && num != CORR; hy++) ;
  num = 0;
  for (zy = ly; zy <= hy; zy++) {
    for (zx = lx; zx <= hx; zx++) {
      if (get_cell_type(floor_info[zx][zy]) == SDOOR) {
	set_cell_type(floor_info[zx][zy], DOOR);
	print(zx, zy, DOOR_SYM);
	num++;
      } else if (get_cell_type(floor_info[zx][zy]) == SCORR) {
	set_cell_type(floor_info[zx][zy], CORR);
	print(zx, zy, CORR_SYM);
	num++;
      } else if ((ttmp = trap_at(zx, zy))) {
	if (get_trap_type(ttmp->trap_info) == PIERC) {
	  makemon(PM_PIERCER, zx, zy);
	  num++;
	  deltrap(ttmp);
	} else if (!get_trap_seen(ttmp->trap_info)) {
	  ttmp->trap_info |= SEEN_TRAP;
	  if (!vism_at(zx, zy))
	    print(zx,zy,'^');
	  num++;
	}
      } else if ((mtmp = mon_at(zx,zy)) && (mtmp->bitflags & M_IS_MIMIC)) {
	see_mimic(mtmp);
	num++;
      }
    }
  }
  return num;
}


// ok, the SDOOR/SCORR part works.  however, none of the rest is tested
// (traps, mimics, swallowed, etc)
void do_search() // was dosearch
{
  Int8 x,y;
  trap_t *trap;
  monst_t *mtmp;
  UChar floor_type, tmp_type;

  if (you.uswallow) {
    message("What are you looking for? The exit?");
    return;
  }
  for (x = you.ux - 1 ; x <= you.ux + 1 ; x++)
    for (y = you.uy - 1 ; y <= you.uy + 1 ; y++) {
      if (x == you.ux && y == you.uy) continue;
      floor_type = get_cell_type(floor_info[x][y]);
      if (floor_type == SDOOR || floor_type == SCORR) {
	if (rund(7)) continue;
	tmp_type = (floor_type == SDOOR) ? DOOR : CORR;
	set_cell_type(floor_info[x][y], tmp_type);
	floor_info[x][y] &= ~SEEN_CELL;	/* force prl */
	prl(x,y);
	nomul(0);
      } else {
	/* Be careful not to find anything in an SCORR or SDOOR */
	mtmp = mon_at(x,y);
	if (mtmp && (mtmp->bitflags & M_IS_MIMIC)) {
	  see_mimic(mtmp);
	  message("You find a mimic.");
	  return;
	}
	for (trap = ftrap ; trap ; trap = trap->ntrap)
	  if (trap->tx == x && trap->ty == y &&
	      !(get_trap_seen(trap->trap_info)) && !rund(8)) {
	    nomul(0);
	    tmp_type = get_trap_type(trap->trap_info);
	    StrPrintF(ScratchBuffer, "You find a%s.", traps[tmp_type]);
	    message(ScratchBuffer);
	    if (tmp_type == PIERC) {
	      deltrap(trap);
	      makemon(PM_PIERCER, x, y);
	      return;
	    }
	    trap->trap_info |= SEEN_TRAP;
	    if (!vism_at(x,y))
	      print(x, y, '^');
	  }
      }
    }
}

Boolean do_id_trap() // was doidtrap
{
  trap_t *trap;
  Short x,y;
  UChar tmp_type;
  x = you.ux + you.dx;
  y = you.uy + you.dy;
  for (trap = ftrap ; trap ; trap = trap->ntrap) {
    if (trap->tx == x && trap->ty == y && get_trap_seen(trap->trap_info)) {
      tmp_type = get_trap_type(trap->trap_info);
      if ((you.dz) && ((you.dz < 0) != (!xdnstair && (tmp_type == TRAPDOOR))))
	continue;
      StrPrintF(ScratchBuffer, "That is a%s.", traps[tmp_type]);
      message(ScratchBuffer);
      return false;
    }
  }
  message("I can't see a trap there.");
  return false;
}


void wakeup(monst_t *mtmp)
{
  mtmp->bitflags &= ~M_IS_ASLEEP;//msleep = 0;
  setmangry(mtmp);
  if (mtmp->bitflags & M_IS_MIMIC) see_mimic(mtmp);
}


/* NOTE: we must check if (mtmp->mimic) before calling this routine */
void see_mimic(monst_t *mtmp) // was seemimic
{
  mtmp->bitflags &= ~M_IS_MIMIC; // mtmp->mimic = 0;
  mtmp->mappearance = 0;  // hmmm
  unpmon(mtmp);
  pmon(mtmp);
}

