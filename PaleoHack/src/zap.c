/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
#include "paleohackRsc.h"
#include "bit.h"

extern previous_state curr_state;

#define MAX_FLTXT 6
#define GENERIC_FL 5
Char *fl[MAX_FLTXT] = {
  "magic missile",
  "bolt of fire",
  "sleep ray",
  "bolt of cold",
  "death ray",
  "blaze of fire" // this is the generic one
};


// move these to prototypes.h:
monst_t * bhit(Short ddx, Short ddy, Short range, Char sym,
	       Boolean call_fhit, obj_t *obj);
void buzz(Short type, Short sx, Short sy, Short dx, Short dy);


static void beam_hit_mon(monst_t *mtmp, obj_t *otmp) SEC_3; // was bhitm
static Boolean beam_hit_obj(obj_t *obj, obj_t *otmp) SEC_3; // was bhito
static Char dirlet(Short dx, Short dy) SEC_3;

static Short zap_damage(monst_t *mon, Short type) SEC_3; // was 'zhit'
static Boolean revive(obj_t *obj) SEC_3;
static void relocate(obj_t *obj) SEC_3; // was called 'rloco'
static void burn_scrolls() SEC_3;



Boolean do_zap(obj_t *obj)
{
  if (!obj) return false;
  if (obj->spe < 0 || (obj->spe == 0 && rund(121))) {
    message("Nothing Happens.");
    return true;
  }

  if (obj->spe == 0)
    message("You wrest one more spell from the worn-out wand.");
  // hm, but it won't show yet

  // now we need to get a direction!  we should set you.dx and you.dy...
  if (objects[obj->otype].bits & NODIR) {
    you.dx = you.dy = you.dz = 0;
    curr_state.item = obj;
    return do_zap_helper(); // return true if form_inv should 'tick'.
  } else {
    curr_state.cmd = 'z';
    curr_state.item = obj;
    curr_state.mode = MODE_DIRECTIONAL;
    draw_directional();
    return false; // do not 'tick'
  }
}

// Caller should have [arranged to have] set you.dx and you.dy.
extern Short engrave_or_what;
Boolean do_zap_helper()
{
  obj_t *obj = curr_state.item;
  Short zx,zy;

  if (!obj) return false;// should not happen!

  obj->spe--;
  if (objects[obj->otype].bits & IMMEDIATE) {
    if (you.uswallow)
      beam_hit_mon(you.ustuck, obj);
    else if (you.dz) {
      if (you.dz > 0) {
	obj_t *otmp = obj_at(you.ux, you.uy);
	if (otmp)
	  beam_hit_obj(otmp, obj);
      }
    } else
      bhit(you.dx, you.dy, rund(8)+6, 0, true, obj);
  } else {
    switch(obj->otype) {
    case WAN_LIGHT:
      litroom(true);
      break;
    case WAN_SECRET_DOOR_DETECTION:
      if (!findit()) return true; // we used a turn but didn't i.d. the wand
      break;
    case WAN_CREATE_MONSTER:
      {
	Short cnt = 1;
	if (!rund(23)) cnt += rund(7) + 1;
	while (cnt--)
	  makemon(NULL, you.ux, you.uy);
      }
      break;
    case WAN_WISHING:
      {
	if (you.uluck + rund(5) < 0) {
	  message("Unfortunately, nothing happens.");
	  break;
	}
	engrave_or_what = GET_WISH;
	FrmPopupForm(EngraveForm);
	// xxx Hey, do I need to do something about taking a turn here?
	break;
      }
    case WAN_DIGGING:
      /* Original effect (approximately):
       * from CORR: dig until we pierce a wall
       * from ROOM: piece wall and dig until we reach
       * an ACCESSIBLE place.
       * Currently: dig for digdepth positions;
       * also down on request of Lennart Augustsson.
       */
      {
	//	struct rm *room;
	UChar tile_type;
	Short digdepth;
	if (you.uswallow) {
	  monst_t *mtmp = you.ustuck;

	  StrPrintF(ScratchBuffer, "You pierce %s's stomach wall!",
		monnam(mtmp));
	  message(ScratchBuffer);
	  mtmp->mhp = 1;	/* almost dead */
	  unstuck(mtmp);
	  mnexto(mtmp);
	  break;
	}
	if (you.dz) {
	  if (you.dz < 0) {
	    message("You loosen a rock from the ceiling.");
	    message("It falls on your head!");
	    losehp(1, "falling rock");
	    mksobj_at(ROCK, you.ux, you.uy);
	    fobj->quantity = 1;
	    stackobj(fobj);
	    if (Invisible) newsym(you.ux, you.uy);
	  } else {
	    dighole();
	  }
	  break;
	}
	zx = you.ux+you.dx;
	zy = you.uy+you.dy;
	digdepth = 8 + rund(18);
	Tmp_at_init('*');
	while (--digdepth >= 0) {
	  if (OUT_OF_BOUNDS(zx,zy)) break;
	  tile_type = get_cell_type(floor_info[zx][zy]);//room = &levl[zx][zy];
	  Tmp_at(zx,zy);
	  if (!xdnstair) {
	    if (zx < 3 || zx > DCOLS-3 ||
	       zy < 3 || zy > DROWS-3)
	      break;
	    if (tile_type == HWALL ||
	       tile_type == VWALL) {
	      set_cell_type(floor_info[zx][zy], ROOM);
	      break;
	    }
	  } else
	    if (tile_type == HWALL || tile_type == VWALL ||
	       tile_type == SDOOR || tile_type == LDOOR) {
	      set_cell_type(floor_info[zx][zy], DOOR);
	      digdepth -= 2;
	    } else
	      if (tile_type == SCORR || !tile_type) {
		set_cell_type(floor_info[zx][zy], CORR);
		digdepth--;
	      }
	  mnewsym(zx,zy);
	  zx += you.dx;
	  zy += you.dy;
	}
	mnewsym(zx,zy);	/* not always necessary */
	Tmp_at_cleanup();
	break;
      }
    default:
      buzz((Short) obj->otype - WAN_MAGIC_MISSILE,
	   you.ux, you.uy, you.dx, you.dy);
      break;
    }
    if (!BITTEST(oc_name_known, obj->otype)) {
      BITSET(oc_name_known, obj->otype);
      more_experienced(0,10);
    }
  }
  return true;
}


Char exclaim(Short force) // hm, this is used in fight.c too. was '* exclam'
{
  /* force == 0 occurs e.g. with sleep ray */
  /* note that large force is usual with wands so that !! would
     require information about hand/weapon/wand */
  return( (force < 0) ? '?' : (force <= 4) ? '.' : '!' );
}

// Used in fight.c (one place) and zap.c (three places)
void hit_message(Char *str, monst_t *mtmp, Char punct) // This was 'hit(...)'
{
  /* 'punct' is usually either "." or "!" */
  if (!cansee(mtmp->mx, mtmp->my))
    StrPrintF(ScratchBuffer, "The %s hits it.", str);
  else
    StrPrintF(ScratchBuffer, "The %s hits %s%c", str, monnam(mtmp), punct);
  message(ScratchBuffer);
}

// used here and, for some reason, in do.c
void miss_message(Char *str, monst_t *mtmp) // This was 'miss(...)'
{
  if (!cansee(mtmp->mx, mtmp->my))
    StrPrintF(ScratchBuffer, "The %s misses it.", str);
  else
    StrPrintF(ScratchBuffer, "The %s misses %s.", str, monnam(mtmp));
  message(ScratchBuffer);
}








/* Routines for IMMEDIATE wands. */
/* bhitm: monster mtmp was hit by the effect of wand owand */
static void beam_hit_mon(monst_t *mtmp, obj_t *owand) // was bhitm
{
  wakeup(mtmp); // little Suzie..
  switch(owand->otype) {
  case WAN_STRIKING:
    if (you.uswallow || rnd(20) < 10+mtmp->data->ac) {
      Short tmp = dice(2,12);
      hit_message("wand", mtmp, exclaim(tmp));
      mtmp->mhp -= tmp;
      if (mtmp->mhp < 1) killed(mtmp);
    } else miss_message("wand", mtmp);
    break;
  case WAN_SLOW_MONSTER:
    mtmp->mspeed = MSLOW;
    break;
  case WAN_SPEED_MONSTER:
    mtmp->mspeed = MFAST;
    break;
  case WAN_UNDEAD_TURNING:
    if (StrChr(UNDEAD, mtmp->data->mlet)) {
      mtmp->mhp -= rnd(8);
      if (mtmp->mhp < 1) killed(mtmp);
      else mtmp->mflee_and_time |= M_FLEEING;
    }
    break;
  case WAN_POLYMORPH:
    if ( newcham(mtmp,&mons[rund(CMNUM)]) )
      BITSET(oc_name_known, owand->otype);
    break;
  case WAN_CANCELLATION:
    mtmp->bitflags |= M_IS_CANCELLED;
    break;
  case WAN_TELEPORTATION:
    rloc(mtmp);
    break;
  case WAN_MAKE_INVISIBLE:
    mtmp->bitflags |= M_IS_INVISIBLE;
    break;
#ifdef WAN_PROBING
  case WAN_PROBING:
    mstatusline(mtmp);
    break;
#endif WAN_PROBING
  default:
    StrPrintF(ScratchBuffer,
	      "What an interesting wand (%u)", owand->otype);
    message(ScratchBuffer);
  }
}


/* object obj was hit by the effect of wand owand */
/* returns TRUE if something was done */
static Boolean beam_hit_obj(obj_t *obj, obj_t *owand) // was bhito
{
  Boolean res = true;
  obj_t *otmp;

  if (obj == uball || obj == uchain)
    res = false;
  else
    switch(owand->otype) {
    case WAN_POLYMORPH:
      /* preserve symbol and quantity, but turn rocks into gems */
      otmp = mkobj_at( ((obj->otype == ROCK ||
			 obj->otype == ENORMOUS_ROCK) ? GEM_SYM : obj->olet),
		       obj->ox, obj->oy);
      otmp->quantity = obj->quantity;
      delobj(obj);
      break;
    case WAN_STRIKING:
      if (obj->otype == ENORMOUS_ROCK)
	fracture_rock(obj);
      else
	res = false;
      break;
    case WAN_CANCELLATION:
      if (obj->spe && obj->olet != AMULET_SYM) {
	obj->bitflags &= ~O_IS_KNOWN;
	obj->spe = 0;
      }
      break;
    case WAN_TELEPORTATION:
      relocate(obj);
      break;
    case WAN_MAKE_INVISIBLE:
      obj->bitflags |= O_IS_INVISIBLE;
      break;
    case WAN_UNDEAD_TURNING:
      res = revive(obj);
      break;
    case WAN_SLOW_MONSTER:		/* no effect on objects */
    case WAN_SPEED_MONSTER:
#ifdef WAN_PROBING
    case WAN_PROBING:
#endif WAN_PROBING
      res = false;
      break;
    default:
      StrPrintF(ScratchBuffer,
		"What an interesting wand (%u)", owand->otype);
      message(ScratchBuffer);
    }
  return res;
}


/* bhit: called when a weapon is thrown (sym = obj->olet) or when an
   IMMEDIATE wand is zapped (sym = 0); the weapon falls down at end of
   range or when a monster is hit; the monster is returned, and bhitpos
   is set to the final position of the weapon thrown; the ray of a wand
   may affect several objects and monsters on its path - for each of
   these an argument function is called. */
/* check !u.uswallow before calling bhit() */
// Since fhitm and fhito are only ever either bhitm+bhito or NULL+NULL,
// I have replaced them with Folger's Crystals.
// Args:
/* direction and range */
/* symbol displayed on path */
/* fns called when mon/obj hit */
/* 2nd arg to fhitm/fhito */
PointType bhitpos;
monst_t * bhit(Short ddx, Short ddy, Short range, Char sym,
	       Boolean call_fhit, obj_t *obj)
{
  monst_t *mtmp;
  obj_t *otmp;
  UChar tile_type;

  bhitpos.x = you.ux;
  bhitpos.y = you.uy;

  if (sym) tmp_at(-1, sym);	/* opening call */ // XXX not impl yet
  while (range-- > 0) {
    bhitpos.x += ddx;
    bhitpos.y += ddy;
    tile_type = get_cell_type(floor_info[bhitpos.x][bhitpos.y]);
    if ((mtmp = mon_at(bhitpos.x,bhitpos.y))) {
      if (sym) {
	tmp_at(-1, -1);	/* closing call */ // XXX not impl yet
	return mtmp;
      }
      if (call_fhit) beam_hit_mon(mtmp, obj);
      range -= 3;
    }
    if (call_fhit && (otmp = obj_at(bhitpos.x,bhitpos.y))) {
      if (beam_hit_obj(otmp, obj))
	range--;
    }
    if (!ZAP_POS(tile_type)) {
      bhitpos.x -= ddx;
      bhitpos.y -= ddy;
      break;
    }
    if (sym) tmp_at(bhitpos.x, bhitpos.y); // XXX not impl yet
  }

  /* leave last symbol unless in a pool */
  tile_type = get_cell_type(floor_info[bhitpos.x][bhitpos.y]);
  if (sym)
    tmp_at(-1, (tile_type == POOL) ? -1 : 0);
  return NULL;
}


// boomerang...
extern const Int8 xdir[8], ydir[8];
// I got rid of youmonst; instead, set caught to true if you catch it.
monst_t * boomhit(Short dx, Short dy, Boolean *caught)
{
  Short i, ct;
  monst_t *mtmp;
  Char sym = ')';
  UChar tile_type;

  *caught = false;
  bhitpos.x = you.ux;
  bhitpos.y = you.uy;

  for (i = 0 ; i < 8 ; i++) if (xdir[i] == dx && ydir[i] == dy) break;
  tmp_at(-1, sym);	/* open call */
  for (ct = 0 ; ct < 10 ; ct++) {
    if (i == 8) i = 0;
    sym = ')' + '(' - sym;
    tmp_at(-2, sym);	/* change let call */
    dx = xdir[i];
    dy = ydir[i];
    bhitpos.x += dx;
    bhitpos.y += dy;
    if ((mtmp = mon_at(bhitpos.x, bhitpos.y))) {
      tmp_at(-1,-1);
      return mtmp;
    }
    tile_type = get_cell_type(floor_info[bhitpos.x][bhitpos.y]);
    if (!ZAP_POS(tile_type)) {
      bhitpos.x -= dx;
      bhitpos.y -= dy;
      break;
    }
    if (bhitpos.x == you.ux && bhitpos.y == you.uy) { /* ct == 9 */
      if (rund(20) >= 10+you.ulevel){	/* we hit ourselves */
	thing_hit_you(10, rnd(10), "boomerang");
	break;
      } else {	/* we catch it */
	tmp_at(-1,-1);
	message("Skillfully, you catch the boomerang.");
	*caught = true;
	return NULL;
      }
    }
    tmp_at(bhitpos.x, bhitpos.y);
    if (ct % 5 != 0) i++;
  }
  tmp_at(-1, -1);	/* do not leave last symbol */
  return NULL;
}




static Char dirlet(Short dx, Short dy)
{
  return
    (dx == dy) ? '\\' : (dx && dy) ? '/' : dx ? '-' : '|';
}




extern Boolean stop_occupation_now;
// note: buzz has not been totally proofread.....
/* type == -1: monster spitting fire at you */
/* type == -1,-2,-3: bolts sent out by wizard */
/* called with dx = dy = 0 with vertical bolts */
void buzz(Short type, Short sx, Short sy, Short dx, Short dy)
{
  Short abstype = abs(type);
  Char *fltxt = (type == -1) ? fl[GENERIC_FL] : fl[abstype];
  //  struct rm *lev;
  UChar *levp;
  Short range;
  monst_t *mon;

  if (you.uswallow) {
    Short tmp;

    if (type < 0) return;
    tmp = zap_damage(you.ustuck, type);
    StrPrintF(ScratchBuffer, "The %s rips into %s%c",
	  fltxt, monnam(you.ustuck), exclaim(tmp));
    message(ScratchBuffer);
    return;
  }
  if (type < 0) pru();
  range = rund(7) + 7;
  Tmp_at_init(dirlet(dx,dy));
  while (range-- > 0) {
    sx += dx;
    sy += dy;
    //  if ((lev = &levl[sx][sy])->typ) Tmp_at(sx,sy);
    levp = &(floor_info[sx][sy]);
    if (levp && get_cell_type(*levp)) Tmp_at(sx,sy);
    else {
      Int8 bounce = 0;
      if (cansee(sx-dx,sy-dy)) {
	StrPrintF(ScratchBuffer, "The %s bounces!", fltxt);
	message(ScratchBuffer);
      }
      if (ZAP_POS(get_cell_type(floor_info[sx][sy-dy])))
	bounce = 1;
      if (ZAP_POS(get_cell_type(floor_info[sx-dx][sy]))) {
	if (!bounce || rund(2)) bounce = 2;
      }
      switch(bounce) {
      case 0:
	dx = -dx;
	dy = -dy;
	continue;
      case 1:
	dy = -dy;
	sx -= dx;
	break;
      case 2:
	dx = -dx;
	sy -= dy;
	break;
      }
      Tmp_at_newsymbol(dirlet(dx,dy));
      continue;
    }
    if (get_cell_type(*levp) == POOL && abstype == 1 /* fire */) {
      range -= 3;
      set_cell_type(*levp, ROOM); //lev->typ = ROOM; // Hm, will that work?
      if (cansee(sx,sy)) {
	mnewsym(sx,sy);
	message("The water evaporates.");
      } else
	message("You hear a hissing sound.");
    }
    if ((mon = mon_at(sx,sy)) &&
       (type != -1 || mon->data->mlet != 'D')) {
      wakeup(mon);
      if (rnd(20) < 18 + mon->data->ac) {
	Short tmp = zap_damage(mon,abstype);
	if (mon->mhp < 1) {
	  if (type < 0) {
	    if (cansee(mon->mx,mon->my)) {
	      StrPrintF(ScratchBuffer, "%s is killed by the %s!",
		    Monnam(mon), fltxt);
	      message(ScratchBuffer);
	    }
	    mondied(mon);
	  } else
	    killed(mon);
	} else
	  hit_message(fltxt, mon, exclaim(tmp));
	range -= 2;
      } else
	miss_message(fltxt,mon);
    } else if (sx == you.ux && sy == you.uy) {
      nomul(0);
      if (rnd(20) < 18+you.uac) {
	Short dam = 0;
	range -= 2;
	StrPrintF(ScratchBuffer, "The %s hits you!",fltxt);
	message(ScratchBuffer);
	switch(abstype) {
	case 0:
	  dam = dice(2,6);
	  break;
	case 1:
	  if (Fire_resistance)
	    message("You don't feel hot!");
	  else dam = dice(6,6);
	  if (!rund(3))
	    burn_scrolls();
	  break;
	case 2:
	  nomul(-rnd(25)); /* sleep ray */
	  break;
	case 3:
	  if (Cold_resistance)
	    message("You don't feel cold!");
	  else dam = dice(6,6);
	  break;
	case 4:
	  you.uhp = -1;
	}
	losehp(dam,fltxt);
      } else {
	StrPrintF(ScratchBuffer, "The %s whizzes by you!",fltxt);
	message(ScratchBuffer);
      }
      stop_occupation_now = true; // stop_occupation(); // hmmm
    }
    if (!ZAP_POS(get_cell_type(*levp))) {
      Int8 bounce = 0;
      UChar rmn;
      if (cansee(sx,sy)) {
	StrPrintF(ScratchBuffer, "The %s bounces!",fltxt);
	message(ScratchBuffer);
      }
      range--;
      if (!dx || !dy || !rund(20)){
	dx = -dx;
	dy = -dy;
      } else {
	if (ZAP_POS(rmn = get_cell_type(floor_info[sx][sy-dy])) &&
	   (IS_ROOM(rmn) || ZAP_POS(get_cell_type(floor_info[sx+dx][sy-dy]))))
	  bounce = 1;
	if (ZAP_POS(rmn = get_cell_type(floor_info[sx-dx][sy])) &&
	   (IS_ROOM(rmn) || ZAP_POS(get_cell_type(floor_info[sx-dx][sy+dy]))))
	  if (!bounce || rund(2))
	    bounce = 2;

	switch(bounce) {
	case 0:
	  dy = -dy;
	  dx = -dx;
	  break;
	case 1:
	  dy = -dy;
	  break;
	case 2:
	  dx = -dx;
	  break;
	}
	Tmp_at_newsymbol(dirlet(dx,dy));
      }
    }
  }
  Tmp_at_cleanup();
}




/* returns damage to mon */
static Short zap_damage(monst_t *mon, Short type) // was 'zhit'
{
  Short tmp = 0;

  switch(type) {
  case 0:			/* magic missile */
    tmp = dice(2,6);
    break;
  case -1:		/* Dragon blazing fire */
  case 1:			/* fire */
    if (StrChr("Dg", mon->data->mlet)) break;
    tmp = dice(6,6);
    if (StrChr("YF", mon->data->mlet)) tmp += 7;
    break;
  case 2:			/* sleep*/
    mon->bitflags |= M_IS_FROZEN; //   mon->mfroz = 1;
    break;
  case 3:			/* cold */
    if (StrChr("YFgf", mon->data->mlet)) break;
    tmp = dice(6,6);
    if (mon->data->mlet == 'D') tmp += 7;
    break;
  case 4:			/* death*/
    if (StrChr(UNDEAD, mon->data->mlet)) break;
    tmp = mon->mhp+1;
    break;
  }
  mon->mhp -= tmp;
  return tmp;
}

#define	CORPSE_I_TO_C(otype)	(Char) ((otype >= DEAD_ACID_BLOB)\
		     ?  'a' + (otype - DEAD_ACID_BLOB)\
		     :	'@' + (otype - DEAD_HUMAN))
static Boolean revive(obj_t *obj)
{
  monst_t *mtmp = NULL;

  if (obj->olet == FOOD_SYM && obj->otype > CORPSE) {
    /* do not (yet) revive shopkeepers */
    /* Note: this might conceivably produce two monsters
       at the same position - strange, but harmless */
    mtmp = mkmon_at(CORPSE_I_TO_C(obj->otype), obj->ox, obj->oy);
    delobj(obj);
  }
  return (mtmp != NULL);		/* TRUE if some monster was created */
}


static void relocate(obj_t *obj) // was called 'rloco'
{
  Short tx, ty, otx, oty;

  otx = obj->ox;
  oty = obj->oy;
  do {
    tx = rund(DCOLS-3) + 2;
    ty = rund(DROWS);
  } while (!goodpos(tx,ty));
  obj->ox = tx;
  obj->oy = ty;
  if (cansee(otx,oty))
    newsym(otx,oty);
}


/* fractured by pick-axe or wand of striking */
/* no texts here! */
void fracture_rock(obj_t *obj)
{
  /* unpobj(obj); */ // This was commented out.
  obj->otype = ROCK;
  obj->quantity = 7 + rund(60);
  obj->owt = weight(obj);
  obj->olet = WEAPON_SYM;
  if (cansee(obj->ox, obj->oy))
    prl(obj->ox, obj->oy);
}

static void burn_scrolls()
{
  obj_t *obj, *obj2;
  Short cnt = 0;

  for (obj = invent; obj; obj = obj2) {
    obj2 = obj->nobj;
    if (obj->olet == SCROLL_SYM) {
      cnt++;
      useup(obj);
    }
  }
  if (cnt > 1) {
    message("Your scrolls catch fire!");
    losehp(cnt, "burning scrolls");
  } else if (cnt) {
    message("Your scroll catches fire!");
    losehp(1, "burning scroll");
  }
}
