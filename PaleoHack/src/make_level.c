/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"


// use the RectangleType we have already.
// it is two PointTypes: topLeft, extent.
// a PointType has x,y.  (these are Coords, formerly SWords.)

// got all kinds of good stuff:
// Rct{ Set, Copy, Inset, Offset, PtIn. }Rectangle,
// RctGetIntersection.
#define	XLIM	4	/* define minimum required space around a room */
#define	YLIM	3
Boolean secret;		/* true while making a vault: increases [XY]LIM */

//#define MAX_ROOMS 16 // moved to constant.c
room_t rooms[MAX_ROOMS];
Short smeq[MAX_ROOMS]; // not sure how to expand this name!!!

//#define MAX_DOORS 100 // moved to constant.h
PointType doors[MAX_DOORS];

Short doorindex = 0;
Short n_intersection = 0; // was nxcor
Short nroom;

// [0..rects_ctr) not looked at yet, [rects_ctr..rects_max] are discarded.
#define MAX_RECTS 51
RectangleType rects[MAX_RECTS];
Short rects_ctr, rects_max;

Boolean gold_seen;

Short xdnstair;
Short xupstair;
Short ydnstair;
Short yupstair;


// These all should be moved to some other location.  They were in Decl.c.
Int8 dlevel = 1;
Char fut_geno[MAX_GENOCIDE]; // things that have been genocided??
// floor_symbol actually needs to be UChar so it can have EMDASH in-range!
UChar floor_symbol[DCOLS][DROWS]; // scrsym from 'levl'
UChar floor_info[DCOLS][DROWS];
// floor_info bits are "NSLttttt": New, Seen, Lit, + 5 bits of type.


static Boolean makerooms() SEC_1;
static void add_rs(Short lowx, Short lowy, Short hix, Short hiy) SEC_1;
static void add_rs_x(Short lx, Short ly, Short hx, Short hy, Boolean discarded) SEC_1;
static Int qsort_compare_rooms(VoidPtr a, VoidPtr b, Long other) SEC_1;
static PointType find_door_pos(Short xl, Short yl, Short xhi, Short yhi) SEC_1;
static Boolean ok_for_door(Short x, Short y) SEC_1;
static void do_door(Short x, Short y, Short room_ix) SEC_1;
static void do_sdoor(Short x, Short y, Short room_ix, Short type) SEC_1;
static Boolean make_r(Char lowx, Char ddx, Char lowy, Char ddy) SEC_1;
static void makecorridors() SEC_1;
static void join(Short a, Short b) SEC_1;
static void make_niches() SEC_1;
static void make_v_tele() SEC_1;
static void makeniche(Boolean with_trap) SEC_1;


static Short somex(room_t *croom)
{
  return (rund(croom->hx - croom->lx + 1) + croom->lx);
}
static Short somey(room_t *croom)
{
  return (rund(croom->hy - croom->ly + 1) + croom->ly);
}



void makelevel()
{
  room_t *croom, *troom;
  UInt try_ctr;
  Short x,y;

  nroom = 0;
  doorindex = 0;
  rooms[0].hx = -1;	/* in case we are in a maze */

  for (x = 0 ; x < DCOLS ; x++)
    for (y = 0 ; y < DROWS ; y++)
      floor_symbol[x][y] = floor_info[x][y] = 0;

  oinit();	/* assign level dependent obj probabilities */

  if (dlevel >= rund(3)+26) {	/* there might be several mazes */
    makemaz();
    return;
  }

  /* construct the rooms */
  nroom = 0;
  secret = false;
  makerooms();

  /* construct stairs (up and down in different rooms if possible) */
  croom = &rooms[rund(nroom)];
  xdnstair = somex(croom);
  ydnstair = somey(croom);
  floor_symbol[xdnstair][ydnstair] = DOWNSTAIR_SYM;
  set_cell_type(floor_info[xdnstair][ydnstair], STAIRS);
  if (nroom > 1) {
    troom = croom;
    croom = &rooms[rund(nroom-1)];
    if (croom >= troom) croom++;
  }
  xupstair = somex(croom);	/* %% < and > might be in the same place */
  yupstair = somey(croom);
  floor_symbol[xupstair][yupstair] = UPSTAIR_SYM;
  set_cell_type(floor_info[xupstair][yupstair], STAIRS);

  /* for each room: put things inside */
  for (croom = rooms; croom->hx > 0; croom++) {

    /* put a sleeping monster inside */
    /* Note: monster may be on the stairs. This cannot be
       avoided: maybe the player fell through a trapdoor
       while a monster was on the stairs. Conclusion:
       we have to check for monsters on the stairs anyway. */
    if (!rund(3))
      makemon(NULL, somex(croom), somey(croom));

    /* put traps and mimics inside */
    gold_seen = false;
    while (rund(8-(dlevel/6)) == 0)
      mktrap(0, 0, croom);
    if (!gold_seen && !rund(3))
      mkgold(0L, somex(croom), somey(croom));
    if (!rund(3)) {
      mkobj_at(0, somex(croom), somey(croom));
      try_ctr = 0;
      while (rund(5) == 0) {
	if (++try_ctr > 100){
	  alert_message("try_ctr overflow4\n");
	  break;
	}
	mkobj_at(0, somex(croom), somey(croom));
      }
    }
  } // end for room.

  // Gosh, hope this works.
  SysQSort((void *) rooms, nroom, sizeof(room_t), qsort_compare_rooms, NULL);
  makecorridors();
  make_niches();

  /* make a secret treasure vault, not connected to the rest */
  if (nroom <= (2*(MAX_ROOMS-1)/3))
    if (rund(3)) {
      troom = &rooms[nroom];
      secret = true;
      if (makerooms()) {
	troom->rtype = VAULT;		/* treasure vault */
	for (x = troom->lx; x <= troom->hx; x++)
	  for (y = troom->ly; y <= troom->hy; y++)
	    mkgold((Long)(rnd(dlevel*100) + 50), x, y);
	if (rund(3) == 0)
	  make_v_tele();
      }
    }

  if (dlevel > 1 && dlevel < 20 && rund(dlevel) < 3) make_shop();
  else if (dlevel > 6 && !rund(7)) make_zoo(ZOO);
  else if (dlevel > 9 && !rund(5)) make_zoo(BEEHIVE);
  else if (dlevel > 11 && !rund(6)) make_zoo(MORGUE);
  else if (dlevel > 18 && !rund(6)) make_swamp();
}



static Boolean makerooms()
{
  RectanglePtr rsp;
  Short lx, ly, lowx, lowy, hix, hiy, dx, dy;
  Short extent_x, extent_y;
  Short try_ctr = 0, xlim, ylim;

  /* init */
  xlim = XLIM + secret;
  ylim = YLIM + secret;
  if (nroom == 0) {
    rsp = &rects[0];
    RctSetRectangle(rsp, 0, 0, DCOLS-1, DROWS-1);
    rects_max = 1;
  }
  rects_ctr = rects_max;

  /* make rooms until satisfied */
  while (rects_ctr > 0 && nroom < MAX_ROOMS-2) {
    if (!secret &&
	(nroom > (MAX_ROOMS-1)/3) &&
	0 == rund( ((MAX_ROOMS-1)-nroom)*((MAX_ROOMS-1)-nroom) ))
      return false;

    /* pick a rectangle */
    rsp = &rects[rund(rects_ctr)];
    lx = rsp->topLeft.x;
    ly = rsp->topLeft.y;
    extent_x = rsp->extent.x;
    extent_y = rsp->extent.y;

    /* find size of room */
    if (secret)
      dx = dy = 1;
    else {
      dx = 2 + rund( (extent_x - 8 > 20) ? 12 : 8);
      dy = 2 + rund(4);
      if (dx*dy > 50)
	dy = 50/dx;
    }

    /* look whether our room will fit */
    if (extent_x < dx + dx/2 + 2*xlim ||
	extent_y < dy + dy/3 + 2*ylim) {
      /* no, too small */
      /* maybe we throw this area out */
      if (secret || (0 == rund(MAX_ROOMS - nroom - try_ctr))) {
	rects_ctr--;
	// Swap the values of [ctr] and rsp (using [max] as a temporary space).
	RctCopyRectangle(rsp, &rects[rects_max]);
	RctCopyRectangle(&rects[rects_ctr], rsp);
	RctCopyRectangle(&rects[rects_max], &rects[rects_ctr]);
	try_ctr = 0;
      } else
	try_ctr++;
      continue;
    }

    lowx = lx + xlim + rund(extent_x - dx - 2*xlim + 1);
    lowy = ly + ylim + rund(extent_y - dy - 2*ylim + 1);
    hix = lowx + dx;
    hiy = lowy + dy;

    if (make_r(lowx, dx, lowy, dy)) {
      if (secret)
	return true;
      add_rs(lowx-1, lowy-1, hix+1, hiy+1);
      try_ctr = 0;
    } else
      if (try_ctr++ > 100)
	break;
  }
  return false;	/* failed to make vault - very strange */
}




// I have NO IDEA what this does.
static void add_rs(Short lowx, Short lowy, Short hix, Short hiy)
{
  RectanglePtr rsp;
  Short lx,ly,hx,hy,xlim,ylim;
  Boolean discarded;

  xlim = XLIM + secret;
  ylim = YLIM + secret;

  /* walk down since rects_ctr and rects_max change */
  for (rsp = &rects[rects_max-1]; rsp >= &rects[0]; rsp--) {
		
    discarded = (rsp >= &rects[rects_ctr]);
    lx = rsp->topLeft.x;
    ly = rsp->topLeft.y;
    hx = rsp->extent.x  + lx;
    hy = rsp->extent.y  + ly;
    if (lx > hix || ly > hiy ||	hx < lowx || hy < lowy)
      continue;
    if (discarded) {
      // decrement rects_max, then copy the one at (new) rects_max to rsp
      rects_max--;
      RctCopyRectangle(&rects[rects_max], rsp);
    } else {
      rects_max--;
      rects_ctr--;
      // copy the one at rects_ctr to rsp
      RctCopyRectangle(&rects[rects_ctr], rsp);
      if (rects_ctr != rects_max)
	// copy the one at rects_max to rects_ctr
	RctCopyRectangle(&rects[rects_max], &rects[rects_ctr]);
    }
    if (lowy - ly > 2*ylim + 4) add_rs_x(lx,   ly,   hx,    lowy-2,discarded);
    if (lowx - lx > 2*xlim + 4) add_rs_x(lx,   ly,   lowx-2,hy,    discarded);
    if (hy - hiy > 2*ylim + 4)  add_rs_x(lx,   hiy+2,hx,    hy,    discarded);
    if (hx - hix > 2*xlim + 4)  add_rs_x(hix+2,ly,   hx,    hy,    discarded);
  }
}


// This, um, decides whether to add a given new rectangle to rectangle array.
// It will be added if it does not fall wholly within an existing rectangle
// (where within includes having a point on the existing rect's boundary).
// discarded says whether this is a piece of a discarded area (?)
static void add_rs_x(Short lx, Short ly, Short hx, Short hy, Boolean discarded)
{
  RectanglePtr rsp;
  Short i;

  // if proposed new rectangle falls entirely within an existing rct, skip.
  for (i = 0 ; i < rects_max ; i++)
    // Hey!  Does this include equality?  because, I need it to! better test..
    if (RctPtInRectangle(lx, ly, &rects[i]) &&
	RctPtInRectangle(hx, hy, &rects[i]))
      return;

  if (rects_max >= MAX_RECTS)
    return;

  /* make a new entry */
  // this might be screwy now
  rsp = &rects[rects_max];
  rects_max++;
  if (!discarded) {
    RctCopyRectangle(&rects[rects_ctr], &rects[rects_max]); // source, dest!
    rsp = &rects[rects_ctr];
    rects_ctr++;
  }
  RctSetRectangle(rsp, lx, ly, hx-lx, hy-ly);
}


// Help us to sort rooms according to the value of "lx".
// must match CmpFuncPtr.  wonder if this will work ok.
static Int qsort_compare_rooms(VoidPtr a, VoidPtr b, Long other)
{
  room_t *room_a, *room_b;
  room_a = (room_t *) a;
  room_b = (room_t *) b;
  if (room_a->lx < room_b->lx) return -1;
  if (room_a->lx == room_b->lx) return 0;
  return 1;
}
/*
  Then, use:
  SysQSort((void *) rooms, nroom, sizeof(room_t), qsort_compare_rooms, NULL);
*/


// Find a possible door location (anywhere within a rectangular area?)
static PointType find_door_pos(Short xlo, Short ylo, Short xhi, Short yhi)
{
  Boolean found = false;
  PointType ff;
  Short x,y;
  UChar here;

  x = (xlo == xhi) ? xlo : (xlo + rund(xhi-xlo+1));
  y = (ylo == yhi) ? ylo : (ylo + rund(yhi-ylo+1));

  if (ok_for_door(x, y))
    found = true;

  if (!found)
    for (x = xlo; x <= xhi && !found; x++)
      for (y = ylo; y <= yhi && !found; y++)
	if (ok_for_door(x, y))
	  found = true;
  
  if (!found)
    for (x = xlo; x <= xhi && !found; x++)
      for (y = ylo; y <= yhi && !found; y++) {
	here = get_cell_type(floor_info[x][y]);
	if (here == DOOR || here == SDOOR)
	  found = true;
      }

  if (!found) {
    // cannot find something reasonable -- strange
    x = xlo;
    y = yhi;
  }

  ff.x = x;
  ff.y = y;
  return ff;
}


// see whether it is allowable to create a door at [x,y]
static Boolean ok_for_door(Short x, Short y)
{
  UChar here, north, south, east, west;
  here  = get_cell_type(floor_info[x][y]);
  north = get_cell_type(floor_info[x][y-1]);
  south = get_cell_type(floor_info[x][y+1]);
  east  = get_cell_type(floor_info[x+1][y]);
  west  = get_cell_type(floor_info[x-1][y]);
  if (north == DOOR || south == DOOR || east == DOOR || west == DOOR ||
      north == SDOOR || south == SDOOR || east == SDOOR || west == SDOOR ||
      (here != HWALL && here != VWALL) ||
      doorindex >= MAX_DOORS)
    return false;
  return true;
}

// Create a door if it is permissible to do so.
static void do_door(Short x, Short y, Short room_ix)
{
  if (doorindex >= MAX_DOORS) {
    alert_message("Error: Too many doors!"); // should not happen, I'd hope
    return;
  }
  if (!ok_for_door(x, y) && (n_intersection != 0))
    return;
  do_sdoor(x, y, room_ix, (rund(8) != 0) ? DOOR : SDOOR);
}

// Create a door (caller must check whether it's actually ok to).
static void do_sdoor(Short x, Short y, Short room_ix, Short type)
{
  Short tmp;

  if (!IS_WALL(get_cell_type(floor_info[x][y])))
    type = DOOR;
  set_cell_type(floor_info[x][y], type);
  if (type == DOOR)
    floor_symbol[x][y] = DOOR_SYM;
  rooms[room_ix].door_ctr++;
  
  room_ix++; // hey, what if it exceeds the max?

  // find a good spot to store this door
  tmp = doorindex;
  if (rooms[room_ix].hx >= 0)
    for ( ; tmp < rooms[room_ix].fdoor ; tmp--)
      doors[tmp] = doors[tmp-1];
  // fix all the other rooms' pointers to doors
  for ( ; rooms[room_ix].hx >= 0 ; room_ix++)
    rooms[room_ix].fdoor++;
  // write the door
  doors[tmp].x = x;
  doors[tmp].y = y;
  doorindex++;
}



/* Only called from makerooms() */
static Boolean make_r(Char lowx, Char ddx, Char lowy, Char ddy)
{
  room_t *croom;
  Short x, y, hix, hiy, xlim, ylim;
  Boolean area_ok = false;
  xlim = XLIM + secret;
  ylim = YLIM + secret;
  hix = lowx+ddx;
  hiy = lowy+ddy;

  if (nroom >= MAX_ROOMS-1) return false;
  // some bounds-coercing:
  lowx = max(lowx, XLIM);
  lowy = max(lowy, YLIM);
  hix = min(hix, DCOLS-XLIM-1);
  hiy = min(hiy, DROWS-YLIM-1);

  while (!area_ok) {
    if (hix <= lowx || hiy <= lowy) return false;

    area_ok = true;
    /* check area around room (and make room smaller if necessary) */
    for (x = lowx - xlim; area_ok && x <= hix + xlim; x++) {
      for (y = lowy - ylim; area_ok && y <= hiy + ylim; y++) {
	if (get_cell_type(floor_info[x][y]) != 0) {
	  if (rund(3) == 0) return false; // 1 in 3 chance of aborting
	  // make room smaller in each dimension
	  if (x < lowx)  lowx = x+xlim+1;
	  else           hix  = x-xlim-1;
	  if (y < lowy)  lowy = y+ylim+1;
	  else           hiy  = y-ylim-1;
	  area_ok = false; // gets us out of the for loop; retry the while.
	}
      }
    }
  } // (we only exit the while loop if the for loop was ok)
  // Hey, this is a good spot for a room.  So, initialize the room.

  croom = &rooms[nroom];

  /* on low levels the room is lit (usually) */
  /* secret vaults are always lit */
  if ((rnd(dlevel) < 10 && rund(77) != 0) || (ddx == 1 && ddy == 1)) {
    for (x = lowx-1; x <= hix+1; x++)
      for (y = lowy-1; y <= hiy+1; y++)
	floor_info[x][y] |= LIT_CELL;
    croom->rlit = true;
  } else
    croom->rlit = false;
  croom->lx = lowx;
  croom->hx = hix;
  croom->ly = lowy;
  croom->hy = hiy;
  croom->rtype = croom->door_ctr = croom->fdoor = 0;

  for (x = lowx-1; x <= hix+1; x++)
    for (y = lowy-1; y <= hiy+1; y += (hiy-lowy+2)) {
      floor_symbol[x][y] = HWALL_SYM;
      set_cell_type(floor_info[x][y], HWALL);
    }
  for (x = lowx-1; x <= hix+1; x += (hix-lowx+2))
    for (y = lowy; y <= hiy; y++) {
      floor_symbol[x][y] = VWALL_SYM;
      set_cell_type(floor_info[x][y], VWALL);
    }
  for (x = lowx; x <= hix; x++)
    for (y = lowy; y <= hiy; y++) {
      floor_symbol[x][y] = ROOM_SYM;
      set_cell_type(floor_info[x][y], ROOM);
    }

  smeq[nroom] = nroom; // whatever....
  nroom++;
  rooms[nroom].hx = -1; // whatever....
  return true;
}


// This is used when you want to know what room something's in....
// it's unchanged, will it work?
// returns 'roomnumber' or -1
Short inroom(Short x, Short y)
{
  room_t *croom = &rooms[0];
  while (croom->hx >= 0) {
    if (croom->hx >= x-1 && croom->lx <= x+1 &&
	croom->hy >= y-1 && croom->ly <= y+1)
      return(croom - rooms);
    croom++;
  }
  return(-1);	/* not in room or on door */
}




// mysterious and unchanged.
static void makecorridors()
{
  Short a,b;

  n_intersection = 0;
  for (a = 0; a < nroom-1; a++)
    join(a, a+1);
  for (a = 0; a < nroom-2; a++)
    if (smeq[a] != smeq[a+2])
      join(a, a+2);
  for (a = 0; a < nroom; a++)
    for (b = 0; b < nroom; b++)
      if (smeq[a] != smeq[b])
	join(a, b);
  if (nroom > 2)
    for (n_intersection = rund(nroom)+4; n_intersection>0; n_intersection--) {
      a = rund(nroom);
      b = rund(nroom-2);
      if (b >= a) b += 2;
      join(a, b);
    }
}


// Attempt to join room A and room B with a corridor...
static void join(Short a, Short b)
{
  PointType cc,tt;
  Short tx, ty, xx, yy;
  room_t *croom, *troom;
  Short dx, dy, dix, diy, cct;
  UChar floor_type;

  croom = &rooms[a]; // come_from?
  troom = &rooms[b]; // to?

  /* find positions cc and tt for doors in croom and troom
     and direction for a corridor between them */

  if (troom->hx < 0 || croom->hx < 0 || doorindex >= MAX_DOORS) return;
  if (troom->lx > croom->hx) {
    dx = 1;
    dy = 0;
    xx = croom->hx+1;
    tx = troom->lx-1;
    cc = find_door_pos(xx,croom->ly,xx,croom->hy);
    tt = find_door_pos(tx,troom->ly,tx,troom->hy);
  } else if (troom->hy < croom->ly) {
    dx = 0;
    dy = -1;
    yy = croom->ly-1;
    cc = find_door_pos(croom->lx,yy,croom->hx,yy);
    ty = troom->hy+1;
    tt = find_door_pos(troom->lx,ty,troom->hx,ty);
  } else if (troom->hx < croom->lx) {
    dx = -1;
    dy = 0;
    xx = croom->lx-1;
    tx = troom->hx+1;
    cc = find_door_pos(xx,croom->ly,xx,croom->hy);
    tt = find_door_pos(tx,troom->ly,tx,troom->hy);
  } else {
    dx = 0;
    dy = 1;
    yy = croom->hy+1;
    ty = troom->ly-1;
    cc = find_door_pos(croom->lx,yy,croom->hx,yy);
    tt = find_door_pos(troom->lx,ty,troom->hx,ty);
  }
  xx = cc.x;
  yy = cc.y;
  tx = tt.x - dx;
  ty = tt.y - dy;
  floor_type = get_cell_type(floor_info[xx+dx][yy+dy]);
  if (n_intersection && floor_type != 0)
    return;
  do_door(xx, yy, a); // a == index of croom

  cct = 0;
  while (xx != tx || yy != ty) {
    xx += dx;
    yy += dy;

    /* loop: dig corridor at [xx,yy] and find new [xx,yy] */
    if (cct++ > 500 || (n_intersection && rund(35)==0))
      return;

    if (xx == DCOLS-1 || xx == 0 || yy == 0 || yy == DROWS-1)
      return;		/* impossible */

    floor_type = get_cell_type(floor_info[xx][yy]);
    //    crm = &levl[xx][yy];
    if (floor_type == 0) {
      if (rund(100) != 0) {
	set_cell_type(floor_info[xx][yy], CORR);
	floor_symbol[xx][yy] = CORR_SYM;
	if (n_intersection && rund(50) == 0)
	  mkobj_at(ROCK_SYM, xx, yy);
      } else {
	set_cell_type(floor_info[xx][yy], SCORR);
	floor_symbol[xx][yy] = SCORR_SYM;
      }
    } else if (floor_type != CORR && floor_type != SCORR) {
      /* strange ... */
      return;
    }

    /* find next corridor position */
    dix = abs(xx-tx);
    diy = abs(yy-ty);

    /* do we have to change direction ? */
    if (dy && dix > diy) {
      Short ddx = (xx > tx) ? -1 : 1;

      floor_type = get_cell_type(floor_info[xx+ddx][yy]);
      //      crm = &levl[xx+ddx][yy];
      if (floor_type == 0 || floor_type == CORR || floor_type == SCORR) {
	dx = ddx;
	dy = 0;
	continue;
      }
    } else if (dx && diy > dix) {
      Short ddy = (yy > ty) ? -1 : 1;

      floor_type = get_cell_type(floor_info[xx][yy+ddy]);
      //      crm = &levl[xx][yy+ddy];
      if (floor_type == 0 || floor_type == CORR || floor_type == SCORR) {
	dy = ddy;
	dx = 0;
	continue;
      }
    }

    /* continue straight on? */
    floor_type = get_cell_type(floor_info[xx+dx][yy+dy]);
    //    crm = &levl[xx+dx][yy+dy];
    if (floor_type == 0 || floor_type == CORR || floor_type == SCORR)
      continue;

    /* no, what must we do now?? */
    if (dx) {
      dx = 0;
      dy = (ty < yy) ? -1 : 1;
      floor_type = get_cell_type(floor_info[xx+dx][yy+dy]);
      //      crm = &levl[xx+dx][yy+dy];
      if (floor_type == 0 || floor_type == CORR || floor_type == SCORR)
	continue;
      dy = -dy;
      continue;
    } else {
      dy = 0;
      dx = (tx < xx) ? -1 : 1;
      floor_type = get_cell_type(floor_info[xx+dx][yy+dy]);
      //      crm = &levl[xx+dx][yy+dy];
      if (floor_type == 0 || floor_type == CORR || floor_type == SCORR)
	continue;
      dx = -dx;
      continue;
    }
  }

  /* we succeeded in digging the corridor */
  do_door(tt.x, tt.y, b); // b == index of troom

  if (smeq[a] < smeq[b])
    smeq[b] = smeq[a];
  else
    smeq[a] = smeq[b];
}

// make some niches-with-no-traps
static void make_niches()
{
  Short ct = rnd(nroom/2 + 1);
  while (ct--)
    makeniche(false);
}

// make a niche-with-trap
static void make_v_tele()
{
  makeniche(true);
}

static void makeniche(Boolean with_trap)
{
  room_t *aroom;
  Short aroom_ix;
  //  struct rm *rm;
  UChar here;
  Short vct = 8;
  PointType dd;
  Short dy,xx,yy;
  trap_t *ttmp; // hmmm.

  if (doorindex < MAX_DOORS)
    while (vct--) {
      aroom_ix = rund(nroom-1);
      aroom = &rooms[aroom_ix];
      if (aroom->rtype != 0) continue;	/* not an ordinary room */
      if (aroom->door_ctr == 1 && rund(5) != 0) continue;
      if (rund(2) != 0) {
	dy = 1;
	dd = find_door_pos(aroom->lx,aroom->hy+1,aroom->hx,aroom->hy+1);
      } else {
	dy = -1;
	dd = find_door_pos(aroom->lx,aroom->ly-1,aroom->hx,aroom->ly-1);
      }

      xx = dd.x;
      yy = dd.y;
      here = get_cell_type(floor_info[xx][yy+dy]);
      if (here) continue;

      if (with_trap || rund(4)==0) {
	set_cell_type(floor_info[xx][yy+dy], SCORR);
	floor_symbol[xx][yy+dy] = SCORR_SYM;
	if (with_trap) {
	  ttmp = maketrap(xx, yy+dy, TELEP_TRAP);
	  ttmp->trap_info |= ONCE_TRAP;
	  make_engr_at(xx, yy-dy, "ad ae?ar um");
	}
	do_sdoor(xx, yy, aroom_ix, SDOOR);
      } else {
	set_cell_type(floor_info[xx][yy+dy], CORR);
	floor_symbol[xx][yy+dy] = CORR_SYM;
	if (rund(7) != 0)
	  do_sdoor(xx, yy, aroom_ix, (rund(5)!=0) ? SDOOR : DOOR);
	else {
	  mksobj_at(SCR_TELEPORTATION, xx, yy+dy);
	  if (rund(3)==0) mkobj_at(0, xx, yy+dy);
	}
      }
      return;
    }
}

/* make a trap somewhere (in croom if mazeflag = 0) */
void mktrap(Short num, Short mazeflag, room_t *croom)
{
  trap_t *ttmp;
  Short kind, nopierc, nomimic, try_ctr = 0;
  Boolean fakedoor, fakegold;
  Short mx, my;//  Char mx,my;

  // First, figure out what kind of trap to make.
  if (!num || num >= TRAPNUM) {
    nopierc = (dlevel < 4) ? 1 : 0;
    nomimic = (dlevel < 9 || gold_seen ) ? 1 : 0;
    if (StrChr(fut_geno, 'M') != NULL) nomimic = 1;
    kind = rund(TRAPNUM - nopierc - nomimic);
    /* note: PIERC = 7, MIMIC = 8, TRAPNUM = 9 */
  } else
    kind = num;


  if (kind == MIMIC) {
    struct monst *mtmp;

    // what to make the mimic look like
    fakedoor = (!mazeflag && rund(3) == 0);
    fakegold = (!fakedoor && rund(2) == 0);
    if (fakegold) gold_seen = true;
    // find a good place to put the mimic
    do {
      if (++try_ctr > 200)
	return;
      if (fakedoor) {  /* note: fakedoor may be on an actual door */
	if (rund(2)) {
	  mx = (rund(2)) ? croom->hx+1 : croom->lx-1;
	  my = somey(croom);
	} else {
	  mx = somex(croom);
	  my = (rund(2)) ? croom->hy+1 : croom->ly-1;
	}
      } else if (mazeflag) {
	PointType mm;
	mm = mazexy();
	mx = mm.x;
	my = mm.y;
      } else {
	mx = somex(croom);
	my = somey(croom);
      }
    } while (mon_at(mx,my) || get_cell_type(floor_info[mx][my]) == STAIRS);

    // create the mimic in that place, and set its appearance.
    mtmp = makemon(PM_MIMIC,mx,my);
    if (mtmp != NULL) {
      mtmp->bitflags |= M_IS_MIMIC;
      mtmp->mappearance =
	(fakegold ? GOLD_SYM :
	 (fakedoor ? DOOR_SYM :
	  ((mazeflag && rund(2)) ? AMULET_SYM :
	   "=/)%?![<>" [ rund(9) ])));
    }
    return;
  }
  // If we're still here, kind != MIMIC.

  do {
    if (++try_ctr > 200)
      return;
    if (mazeflag){
      PointType mm;
      mm = mazexy();
      mx = mm.x;
      my = mm.y;
    } else {
      mx = somex(croom);
      my = somey(croom);
    }
  } while (trap_at(mx, my) || get_cell_type(floor_info[mx][my]) == STAIRS);

  ttmp = maketrap(mx, my, kind);
  // if maze, you have a 10% chance of a regular trap being already visible.
  if (mazeflag && (rund(10)==0) && (get_trap_type(ttmp->trap_info) < PIERC))
    ttmp->trap_info |= SEEN_TRAP;
}











void clear_all_level()
{
  Short i, j;
  room_t zero_room = {0,0,0,0,0,0,0,0};

  for (i = 0 ; i < DCOLS ; i++)
    for (j = 0 ; j < DROWS ; j++)
      floor_symbol[i][j] = floor_info[i][j] = 0;

  for (i = 0 ; i < MAX_DOORS ; i++)
    doors[i].x = doors[i].y = 0;

  for (i = 0 ; i < MAX_ROOMS ; i++) {
    rooms[i] = zero_room;
    smeq[i] = 0;
  }

  // I should also clear the rects.

}
