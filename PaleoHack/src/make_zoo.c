/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
// this was called mkshop.c
//#ifndef QUEST

#define  PL_NSIZ 32      /* name of player, ghost, shopkeeper */  /* XXXXXXX*/

#include "shk.h"
extern Char shtypes[]; /* = "=/)%?!["; 8 types: 7 specialized, 1 mixed */
Int8 shprobs[] = { 3,3,5,5,10,10,14,50 };	/* shop probabilities */
#define ESHK        ( (eshk_t *)((shk)->extra) )

extern room_t rooms[MAX_ROOMS]; // in make_level.c .... SIGH...
extern Short nroom; // ditto
extern PointType doors[MAX_DOORS]; // ditto

static permonst_t * morguemon() SEC_3;

static void findname(Char *nampt, Char let) SEC_5; // was in shknam.c
static Boolean nexttodoor(Short sx, Short sy) SEC_3;
static Boolean has_dnstairs(room_t *sroom) SEC_3;
static Boolean has_upstairs(room_t *sroom) SEC_3;
static Boolean isbig(room_t *sroom) SEC_3;
static Short dist2_squared(Short x0, Short y0, Short x1, Short y1) SEC_3;



#define SHOP_TYPES 8
extern Short *shknam_start;
Char shoptype_let[SHOP_TYPES] = {
  POTION_SYM, SCROLL_SYM, ARMOR_SYM, WAND_SYM, RING_SYM, FOOD_SYM, WEAPON_SYM,
  0
};

/*
void test_findname()
{
  Char name[PL_NSIZ];
  static Short t = SHOP_TYPES-1;
  findname(name, shoptype_let[t]);
  StrPrintF(ScratchBuffer, "\"%s\"", name);
  message(ScratchBuffer);
  //  t = (t+1) % SHOP_TYPES;
  dlevel++;
}
*/

static void findname(Char *nampt, Char let) // was in shknam.c
{
  Short i, shop, shop_types, *offsets = shknam_start;
  Short skip_names = 0, avail_names, len;
  Char *p;
  // first Short in offsets is number of shop types (8)
  // next 8 Shorts in offsets are the number of names for each shop type
  shop_types = max(shknam_start[0], SHOP_TYPES);
  shop = shop_types - 1; // default shop type is general
  for (i = 0 ; i < shop_types ; i++)
    if (shoptype_let[i] == let) {
      shop = i;
      break;
    }
  // find the range of names belonging to this shop type
  for (i = 0 ; i < shop ; i++)
    skip_names += shknam_start[i+1];
  avail_names = shknam_start[shop+1];
  if (dlevel >= avail_names) {
    StrCopy(nampt, "Dirk");
    return;
  }
  // find the offset for a name in this range
  offsets = shknam_start + 1 + shknam_start[0];
  i = skip_names + dlevel;
  p = ((Char *) shknam_start) + offsets[i];
  len = min(offsets[i+1] - offsets[i], PL_NSIZ);
  StrNCopy(nampt, p, len); // not null terminated..
  nampt[len] = '\0';
}


void make_shop() // was mkshop // xxx not tested yet
{
  room_t *sroom;
  Short sh,sx,sy,i = -1;
  Char let;
  Short roomno;
  monst_t *shk;
  /*
#ifdef WIZARD
  // first determine shoptype
  if (wizard){
    Char *ep = getenv("SHOPTYPE");
    if (ep) {
      if (*ep == 'z' || *ep == 'Z') { mkzoo(ZOO); return; }
      if (*ep == 'm' || *ep == 'M') { mkzoo(MORGUE); return; }
      if (*ep == 'b' || *ep == 'B') { mkzoo(BEEHIVE); return; }
      if (*ep == 's' || *ep == 'S') { mkswamp(); return; }
      for (i=0 ; shtypes[i] ; i++) if (*ep == shtypes[i]) break;
      goto gottype;
    }
  }
 gottype:
#endif WIZARD
  */
  for (sroom = &rooms[0], roomno = 0 ; ; sroom++, roomno++) {
    if (sroom->hx < 0) return;
    if (sroom - rooms >= nroom) {
      message("BUG: rooms not closed by -1?");
      return;
    }
    if (sroom->rtype) continue;
    if (!sroom->rlit || has_dnstairs(sroom) || has_upstairs(sroom))
      continue;
    if (
	/*
#ifdef WIZARD
       (wizard && getenv("SHOPTYPE") && sroom->doorct != 0) ||
#endif WIZARD
	*/
       (sroom->door_ctr <= 2 && sroom->door_ctr > 0)) break;
  }

  if (i < 0) {			/* shoptype not yet determined */
    Short j;

    for (j = rund(100), i = 0; (j -= shprobs[i])>= 0; i++)
      if (!shtypes[i]) break;			/* superfluous */
    if (isbig(sroom) && i + SHOPBASE == WANDSHOP)
      i = GENERAL-SHOPBASE;
  }
  sroom->rtype = i + SHOPBASE;
  let = shtypes[i];
  sh = sroom->fdoor;
  sx = doors[sh].x;
  sy = doors[sh].y;
  if (sx == sroom->lx-1) sx++; else
    if (sx == sroom->hx+1) sx--; else
      if (sy == sroom->ly-1) sy++; else
	if (sy == sroom->hy+1) sy--; else {
	/*
#ifdef WIZARD
	  // "This is said to happen sometimes, but I've never seen it."
	  if (wizard) {
	    Short j = sroom->door_ctr;

	    message("Where is shopdoor?");
	    StrPrintF(ScratchBuffer, "Room at (%d,%d),(%d,%d).",
		      sroom->lx, sroom->ly, sroom->hx, sroom->hy);
	    message(ScratchBuffer);
	    StrPrintF(ScratchBuffer, "doormax=%d door_ctr=%d fdoor=%d",
		      doorindex, sroom->door_ctr, sh);
	    message(ScratchBuffer);
	    while (j--) {
	      StrPrintF(ScratchBuffer, "door [%d,%d]",
			doors[sh].x, doors[sh].y);
	      message(ScratchBuffer);
	      sh++;
	    }
	    more();
	  }
#endif WIZARD
	  */
	  return;
	}
  if (!(shk = makemon(PM_SHK,sx,sy))) return;
  shk->extra = (void *) md_malloc(sizeof(eshk_t));  // whee!
  shk->extra_len = sizeof(eshk_t);
  shk->bitflags |= M_IS_SHOPKEEPER | M_IS_PEACEFUL;
  shk->bitflags &= ~M_IS_ASLEEP;
  shk->mtraps_seen = ~0;	/* we know all the traps already */
  ESHK->shoproom = roomno;
  ESHK->shoplevel = dlevel; // XXX
  ESHK->shd = doors[sh];
  ESHK->shk.x = sx;
  ESHK->shk.y = sy;
  ESHK->robbed = 0;
  ESHK->visitct = 0;
  ESHK->following = false;
  shk->mgold = 1000 + 30*rnd(100);	/* initial capital */
  ESHK->billct = 0;
  findname(ESHK->shknam, let);
  for (sx = sroom->lx; sx <= sroom->hx; sx++)
    for (sy = sroom->ly; sy <= sroom->hy; sy++){
      monst_t *mtmp;
      if ((sx == sroom->lx && doors[sh].x == sx-1) ||
	 (sx == sroom->hx && doors[sh].x == sx+1) ||
	 (sy == sroom->ly && doors[sh].y == sy-1) ||
	 (sy == sroom->hy && doors[sh].y == sy+1)) continue;
      if (rund(100) < dlevel && !mon_at(sx,sy) &&
	 (mtmp = makemon(PM_MIMIC, sx, sy))) {
	mtmp->bitflags |= M_IS_MIMIC;
	mtmp->mappearance =
	  (let && rund(10) < dlevel) ? let : ']';
	continue;
      }
      mkobj_at(let, sx, sy);
    }
}




void make_zoo(Short zoo_type) // was mkzoo
{
  room_t *sroom;
  monst_t *mon;
  Short sh,sx,sy,i;
  Short goldlim = 500 * dlevel;
  Short moct = 0;

  i = nroom;
  for (sroom = &rooms[rund(nroom)]; ; sroom++) {
    if (sroom == &rooms[nroom])
      sroom = &rooms[0];
    if (!i-- || sroom->hx < 0)
      return;
    if (sroom->rtype)
      continue;
    if (zoo_type == MORGUE && sroom->rlit)
      continue;
    if (has_upstairs(sroom) || (has_dnstairs(sroom) && rund(3)))
      continue;
    if (sroom->door_ctr == 1 || !rund(5))
      break;
  }
  sroom->rtype = zoo_type;
  sh = sroom->fdoor;
  for (sx = sroom->lx; sx <= sroom->hx; sx++)
    for (sy = sroom->ly; sy <= sroom->hy; sy++){
      if ((sx == sroom->lx && doors[sh].x == sx-1) ||
	  (sx == sroom->hx && doors[sh].x == sx+1) ||
	  (sy == sroom->ly && doors[sh].y == sy-1) ||
	  (sy == sroom->hy && doors[sh].y == sy+1)) continue;
      mon = makemon(
		    (zoo_type == MORGUE) ? morguemon() :
		    (zoo_type == BEEHIVE) ? PM_KILLER_BEE : NULL,
		    sx, sy);
      if (mon) mon->bitflags |= M_IS_ASLEEP;
      switch(zoo_type) {
      case ZOO:
	i = dist2_squared(sx,sy,doors[sh].x,doors[sh].y);
	if (i >= goldlim) i = 5*dlevel;
	goldlim -= i;
	mkgold((Long)(10 + rund(i)), sx, sy);
	break;
      case MORGUE:
	/* Usually there is one dead body in the morgue */
	if (!moct && rund(3)) {
	  mksobj_at(CORPSE, sx, sy);
	  moct++;
	}
	break;
      case BEEHIVE:
	if (!rund(3)) mksobj_at(LUMP_OF_ROYAL_JELLY, sx, sy);
	break;
      }
    }
}



static permonst_t * morguemon()
{
  Short i = rund(100), hd = rund(dlevel);

  if (hd > 10 && i < 10) return(PM_DEMON);
  if (hd > 8 && i > 85) return(PM_VAMPIRE);
  return((i < 40) ? PM_GHOST : (i < 60) ? PM_WRAITH : PM_ZOMBIE);
}

void make_swamp() // was mkswamp   /* Michiel Huisjes & Fred de Wilde */
{
  room_t *sroom;
  Short sx,sy,i,eelct = 0;

  for (i = 0 ; i < 5 ; i++) {		/* 5 tries */
    sroom = &rooms[rund(nroom)];
    if (sroom->hx < 0 || sroom->rtype ||
	has_upstairs(sroom) || has_dnstairs(sroom))
      continue;

    /* satisfied; make a swamp */
    sroom->rtype = SWAMP;
    for (sx = sroom->lx ; sx <= sroom->hx ; sx++)
      for (sy = sroom->ly ; sy <= sroom->hy ; sy++)
	if ((sx+sy)%2 && !obj_at(sx,sy) && !trap_at(sx,sy)
	    && !mon_at(sx,sy) && !nexttodoor(sx,sy)) {
	  set_cell_type(floor_info[sx][sy], POOL);
	  floor_symbol[sx][sy] = POOL_SYM;
	  if (!eelct || !rund(4)) {
	    makemon(PM_EEL, sx, sy);
	    eelct++;
	  }
	}
  }
}



static Boolean nexttodoor(Short sx, Short sy)
{
  Short dx,dy;
  UChar tile;
  for (dx = -1 ; dx <= 1 ; dx++)
    for (dy = -1 ; dy <= 1 ; dy++) {
      tile = get_cell_type(floor_info[sx+dx][sy+dy]);
      if (tile == DOOR || tile == SDOOR || tile == LDOOR) return true;
    }
  return false;
}

static Boolean has_dnstairs(room_t *sroom)
{
  return(sroom->lx <= xdnstair && xdnstair <= sroom->hx &&
	 sroom->ly <= ydnstair && ydnstair <= sroom->hy);
}

static Boolean has_upstairs(room_t *sroom)
{
  return(sroom->lx <= xupstair && xupstair <= sroom->hx &&
	 sroom->ly <= yupstair && yupstair <= sroom->hy);
}

static Boolean isbig(room_t *sroom)
{
  Short area = (sroom->hx - sroom->lx) * (sroom->hy - sroom->ly);
  return (area > 20);
}

static Short dist2_squared(Short x0, Short y0, Short x1, Short y1)
{
  Short s = ((x0-x1)*(x0-x1) + (y0-y1)*(y0-y1));
  return s*s;
}

//#endif // !QUEST
