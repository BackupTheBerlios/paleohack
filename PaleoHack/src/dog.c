/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"

typedef struct {
  Long hungrytime;      /* at this time dog gets hungry */
  Long eattime;         /* dog is eating */
  Long droptime;        /* moment dog dropped object */
  UInt dropdist;        /* dist of drpped obj from @ */
  UInt apport;          /* amount of training */
  Long whistletime;     /* last time he whistled */
} edog_t;

static void initedog(monst_t *mtmp) SEC_4;
Boolean follower(monst_t *mtmp) SEC_4;
static Short dogfood(obj_t *obj) SEC_4;

void makedog()
{
  monst_t *mtmp = makemon(PM_SM_DOG, you.ux, you.uy);
  if (!mtmp) return; /* dogs were genocided */
  initedog(mtmp);
}

#define EDOG(mp)        ( (edog_t *)((mp)->extra) )

static void initedog(monst_t *mtmp)
{
  mtmp->extra = (void *) md_malloc(sizeof(edog_t));  // Dangerous!
  mtmp->extra_len = sizeof(edog_t);
  // When monsters are freed, we must make sure to free 'extra' also
  // iff it is not NULL!
  mtmp->bitflags |= M_IS_TAME | M_IS_PEACEFUL;
  /*   mtmp->extra-> // What was I about to type here??  gar. */
  EDOG(mtmp)->hungrytime = 1000 + moves;
  EDOG(mtmp)->eattime = 0;
  EDOG(mtmp)->droptime = 0;
  EDOG(mtmp)->dropdist = 10000;
  EDOG(mtmp)->apport = 10;
  EDOG(mtmp)->whistletime = 0;
}

void set_whistletime(monst_t *mtmp, Long t)
{
  EDOG(mtmp)->whistletime = t;
}

void losedogs()
{
  monst_t *mtmp;
  while ((mtmp = mydogs)) {
    mydogs = mtmp->nmon; // pull mtmp off the head of mydogs
    mtmp->nmon = fmon;   // push mtmp onto the head of fmon
    fmon = mtmp;
    mnexto(mtmp);
  }
  while ((mtmp = fallen_down)) {
    fallen_down = mtmp->nmon; // same for monsters that fell through trapdr
    mtmp->nmon = fmon;
    fmon = mtmp;
    rloc(mtmp);
  }
}


/*
// aiee!  make this be NOT recursive. X X X
// should not be too hard: have a mtmp_next, set it before calling unlink_mon.
// move the "mtmp = mtmp->nmon" explicitly into the loop in the 'else',
// and in the 'if' do "mtmp = mtmp_next" instead.
// Going to bed now.
void keepdogs_alt()
{
  monst_t *mtmp;
  // search through all of fmon for monsters that are close to you and
  // are your "followers" (and are not asleep or frozen).
  //  message("keepdogs is recursive... evil."); X X X X
  for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
    if (dist(mtmp->mx,mtmp->my) < 3 && follower(mtmp)
	&& !(mtmp->bitflags & (M_IS_ASLEEP | M_IS_FROZEN))) {
      unlink_mon(mtmp); // dike this monster out of the fmon list..
      mtmp->nmon = mydogs;
      mydogs = mtmp; // and put it on the head of the mydogs list.
      unpmon(mtmp);
      keepdogs();       / * we destroyed the link, so use recursion * /
      return;           / * (admittedly somewhat primitive) * /
    }
}
*/
// The new, politically correct version:
void keepdogs()
{
  monst_t *mtmp, *next_mon = NULL;
  // search through all of fmon for monsters that are close to you and
  // are your "followers" (and are not asleep or frozen).
  for (mtmp = fmon ; mtmp ; mtmp = next_mon) {
    next_mon = mtmp->nmon; // get 'next' BEFORE we maybe clobber mtmp->nmon.
    if (dist(mtmp->mx,mtmp->my) < 3 && follower(mtmp)
	&& !(mtmp->bitflags & (M_IS_ASLEEP | M_IS_FROZEN))) {
      unlink_mon(mtmp); // dike this monster out of the fmon list..
      mtmp->nmon = mydogs; // set its "next" to the old head of mydogs,
      mydogs = mtmp; // and put it on the head of the mydogs list.
      unpmon(mtmp); // undraw it
    }
  }
}

void fall_down(struct monst *mtmp)
{
  unlink_mon(mtmp);
  mtmp->nmon = fallen_down;
  fallen_down = mtmp;
  unpmon(mtmp);
  mtmp->bitflags &= ~M_IS_TAME; // mtmp->mtame = 0;
}



/* return quality of food; the lower the better */
#define	DOGFOOD	0
#define	CADAVER	1
#define	ACCFOOD	2
#define	MANFOOD	3
#define	APPORT	4
#define	POISON	5
#define	UNDEF	6
static Short dogfood(obj_t *obj)
{
  UChar t = obj->otype;
  switch(obj->olet) {
  case FOOD_SYM:
    if (t == TRIPE_RATION) return DOGFOOD;
    else if (t < CARROT)   return ACCFOOD;
    else if (t < CORPSE)   return MANFOOD;
    else if (poisonous(obj) || (obj->age+50 <= moves) || t == DEAD_COCKATRICE)
      return POISON;
    else return CADAVER;
    break;
  default:
    if (!(obj->bitflags & O_IS_CURSED)) return APPORT;
    /* fall into next case */
  case BALL_SYM:
  case CHAIN_SYM:
  case ROCK_SYM:
    return UNDEF;
  }
}


/* return 0 (no move), 1 (move) or 2 (dead) */
#define DOG_NO_MOVE 0
#define DOG_MOVE 1
#define DOG_DEAD 2
Short dog_move(struct monst *mtmp, Short after)
{
  Short nx,ny,omx,omy,appr,nearer,j;
  Short udist,chi=-1,i,whappr;
  monst_t *mtmp2;
  permonst_t *mdat = mtmp->data;
  edog_t *edog = EDOG(mtmp);
  obj_t *obj;
  trap_t *trap;
  Int8 cnt,chcnt,nix,niy;
  Int8 dogroom,uroom;
  Int8 gx,gy,gtyp,otyp;	/* current goal */
  coord poss[9];
  Short info[9];
#define GDIST(x,y) ((x-gx)*(x-gx) + (y-gy)*(y-gy))
#define DDIST(x,y) ((x-omx)*(x-omx) + (y-omy)*(y-omy))

  if (moves <= edog->eattime) return DOG_NO_MOVE; /* dog is still eating */
  omx = mtmp->mx;
  omy = mtmp->my;
  whappr = (moves - EDOG(mtmp)->whistletime < 5);
  if (moves > edog->hungrytime + 500 && !(mtmp->bitflags & M_IS_CONFUSED)) {
    mtmp->bitflags |= M_IS_CONFUSED;
    mtmp->mhpmax /= 3;
    if (mtmp->mhp > mtmp->mhpmax)
      mtmp->mhp = mtmp->mhpmax;
    if (cansee(omx,omy))
      StrPrintF(ScratchBuffer, "%s is confused from hunger.", Monnam(mtmp));
    else StrPrintF(ScratchBuffer, "You feel worried about %s.", monnam(mtmp));
    message(ScratchBuffer);
  } else
    if (moves > edog->hungrytime + 750 || mtmp->mhp < 1) {
      if (cansee(omx,omy)) {
	StrPrintF(ScratchBuffer, "%s dies from hunger.", Monnam(mtmp));
	message(ScratchBuffer);
      } else
	message("You have a sad feeling for a moment, then it passes.");
      mondied(mtmp);
      return DOG_DEAD;
    }
  dogroom = inroom(omx,omy);
  uroom = inroom(you.ux,you.uy);
  udist = dist(omx,omy);

  /* maybe we tamed him while being swallowed --jgm */
  if (!udist) return DOG_NO_MOVE;

  /* if we are carrying sth then we drop it (perhaps near @) */
  /* Note: if apport == 1 then our behaviour is independent of udist */
  if (mtmp->minvent) {
    if (!rund(udist) || !rund((Short) edog->apport))
      if (rund(10) < edog->apport){
	release_objs(mtmp, (mtmp->bitflags & M_IS_INVISIBLE));
	if (edog->apport > 1) edog->apport--;
	edog->dropdist = udist;		/* hpscdi!jon */
	edog->droptime = moves;
      }
  } else {
    if ((obj = obj_at(omx,omy))) if (!StrChr("0_", obj->olet)) {
      if ((otyp = dogfood(obj)) <= CADAVER) {
	nix = omx;
	niy = omy;
	goto eatobj;
      }
      if (obj->owt < 10*mtmp->data->mlevel)
	if (rund(20) < edog->apport+3)
	  if (rund(udist) || !rund((Short) edog->apport)) {
	    unlink_obj(obj);
	    unpobj(obj);
	    /* if (floor_symbol[omx][omy] == obj->olet)
	       newsym(omx,omy); */ // This conditional was commented out.
	    mpickobj(mtmp,obj);
	  }
    }
  }

  /* first we look for food */
  gtyp = UNDEF;	/* no goal as yet */

  gx = gy = 0;	/* suppress 'used before set' message */

  for (obj = fobj ; obj ; obj = obj->nobj) {
    otyp = dogfood(obj);
    if (otyp > gtyp || otyp == UNDEF) continue;
    if (inroom(obj->ox, obj->oy) != dogroom) continue;
    if (otyp < MANFOOD &&
	(dogroom >= 0 || DDIST(obj->ox,obj->oy) < 10)) {
      if (otyp < gtyp || (otyp == gtyp &&
			  DDIST(obj->ox,obj->oy) < DDIST(gx,gy))) {
	gx = obj->ox;
	gy = obj->oy;
	gtyp = otyp;
      }
    } else
      if (gtyp == UNDEF && dogroom >= 0 &&
	  uroom == dogroom &&
	  !mtmp->minvent && edog->apport > rund(8)) {
	gx = obj->ox;
	gy = obj->oy;
	gtyp = APPORT;
      }
  }
  if (gtyp == UNDEF ||
      (gtyp != DOGFOOD && gtyp != APPORT && moves < edog->hungrytime)){
    if (dogroom < 0 || dogroom == uroom) {
      gx = you.ux;
      gy = you.uy;
#ifndef QUEST
    } else {
      extern room_t rooms[MAX_ROOMS]; // in make_level.c .... SIGH...
      extern PointType doors[MAX_DOORS]; // ditto
      Short tmp = rooms[dogroom].fdoor;
      cnt = rooms[dogroom].door_ctr;

      gx = gy = FAR;	/* random, far away */
      while (cnt--) {
	if (dist(gx,gy) >
	    dist(doors[tmp].x, doors[tmp].y)) {
	  gx = doors[tmp].x;
	  gy = doors[tmp].y;
	}
	tmp++;
      }
      /* here gx == FAR e.g. when dog is in a vault */
      if (gx == FAR || (gx == omx && gy == omy)) {
	gx = you.ux;
	gy = you.uy;
      }
#endif QUEST
    }
    appr = (udist >= 9) ? 1 : (mtmp->mflee_and_time & M_FLEEING) ? -1 : 0;
    if (after && udist <= 4 && gx == you.ux && gy == you.uy)
      return DOG_NO_MOVE;
    if (udist > 1) {
      if (!IS_ROOM(get_cell_type(floor_info[you.ux][you.uy])) || !rund(4) ||
	  whappr ||
	  (mtmp->minvent && rund((Short) edog->apport)))
	appr = 1;
    }
    /* if you have dog food he'll follow you more closely */
    if (appr == 0) {
      obj = invent;
      while (obj) {
	if (obj->otype == TRIPE_RATION) {
	  appr = 1;
	  break;
	}
	obj = obj->nobj;
      }
    }
  } else	appr = 1;	/* gtyp != UNDEF */
  if (mtmp->bitflags & M_IS_CONFUSED) appr = 0;

  if (gx == you.ux && gy == you.uy && (dogroom != uroom || dogroom < 0)) {
    coord *cp;
    cp = get_track(omx,omy);
    if (cp){
      gx = cp->x;
      gy = cp->y;
    }
  }

  nix = omx;
  niy = omy;
  cnt = mfindpos(mtmp,poss,info,ALLOW_M | ALLOW_TRAPS);
  chcnt = 0;
  chi = -1;
  for (i=0; i<cnt; i++) {
    nx = poss[i].x;
    ny = poss[i].y;
    if (info[i] & ALLOW_M) {
      mtmp2 = mon_at(nx,ny);
      if (mtmp2->data->mlevel >= mdat->mlevel+2 ||
	  mtmp2->data->mlet == 'c')
	continue;
      if (after) return DOG_NO_MOVE; /* hit only once each move */

      if (hitmm(mtmp, mtmp2) == 1 && rund(4) &&
	  mtmp2->mlastmoved != moves &&
	  hitmm(mtmp2,mtmp) == 2) return DOG_DEAD;
      return DOG_NO_MOVE;
    }

    /* dog avoids traps */
    /* but perhaps we have to pass a trap in order to follow @ */
    if ((info[i] & ALLOW_TRAPS) && (trap = trap_at(nx,ny))){
      if (!get_trap_seen(trap->trap_info) && rund(40)) continue;
      if (rund(10)) continue;
    }

    /* dog eschewes cursed objects */
    /* but likes dog food */
    obj = fobj;
    while (obj) {
      if (obj->ox != nx || obj->oy != ny)
	goto nextobj;
      if (obj->bitflags & O_IS_CURSED) goto nxti;
      if (obj->olet == FOOD_SYM &&
	  (otyp = dogfood(obj)) < MANFOOD &&
	  (otyp < ACCFOOD || edog->hungrytime <= moves)) {
	/* Note: our dog likes the food so much that he
	   might eat it even when it conceals a cursed object */
	nix = nx;
	niy = ny;
	chi = i;
      eatobj:
	edog->eattime = moves + obj->quantity * objects[obj->otype].oc_delay;
	if (edog->hungrytime < moves)
	  edog->hungrytime = moves;
	edog->hungrytime += 5*obj->quantity * objects[obj->otype].nutrition;
	mtmp->bitflags &= ~M_IS_CONFUSED;
	if (cansee(nix,niy)) {
	  StrPrintF(ScratchBuffer, "%s ate %s.", Monnam(mtmp), doname(obj));
	  message(ScratchBuffer);
	}
	/* perhaps this was a reward */
	if (otyp != CADAVER)
	  edog->apport += 200/(edog->dropdist+moves-edog->droptime);
	delobj(obj);
	goto newdogpos;
      }
    nextobj:
      obj = obj->nobj;
    }

    for (j=0; j<MTSZ && j<cnt-1; j++)
      if (nx == mtmp->mtrack[j].x && ny == mtmp->mtrack[j].y)
	if (rund(4*(cnt-j))) goto nxti;

    /* Some stupid C compilers cannot compute the whole expression at once. */
    nearer = GDIST(nx,ny);
    nearer -= GDIST(nix,niy);
    nearer *= appr;
    if ((nearer == 0 && !rund(++chcnt)) || nearer<0 ||
	(nearer > 0 && !whappr &&
	 ( (omx == nix && omy == niy && !rund(3)) ||
	   !rund(12) )
	 )) {
      nix = nx;
      niy = ny;
      if (nearer < 0) chcnt = 0;
      chi = i;
    }
  nxti:
    ;
  }
 newdogpos:
  if (nix != omx || niy != omy) {
    if (info[chi] & ALLOW_U){
      hit_you(mtmp, dice(mdat->damn, mdat->damd)+1);
      return DOG_NO_MOVE;
    }
    mtmp->mx = nix;
    mtmp->my = niy;
    for (j=MTSZ-1; j>0; j--) mtmp->mtrack[j] = mtmp->mtrack[j-1];
    mtmp->mtrack[0].x = omx;
    mtmp->mtrack[0].y = omy;
  }
  if (mon_in_trap(mtmp) == 2)	/* he died */
    return DOG_DEAD;
  pmon(mtmp);
  return DOG_MOVE;
}



////////////////////////////////////////
// inroom has been MOVED to make_level.c
////////////////////////////////////////



Boolean tamedog(monst_t *mtmp, obj_t *obj)
{
  /*  monst_t *mtmp2;  */ // I don't need this now

  // too busy howling at the moon...
  if (flags.moon_phase == FULL_MOON && night() && rund(6))
    return false;

  /* If we cannot tame him, at least he's no longer afraid. */
  mtmp->mflee_and_time = 0; // mtmp->mflee = 0;  mtmp->mfleetim = 0;

  // Can't tame: tame/frozen things, long worms, shopkeepers, guards, &@12.
  if ((mtmp->bitflags & (M_IS_TAME | M_IS_FROZEN)) ||
#ifndef NOWORM
      mtmp->wormno ||
#endif NOWORM
      (mtmp->bitflags & (M_IS_SHOPKEEPER | M_IS_GUARD)) ||
      StrChr(" &@12", mtmp->data->mlet))
    return false;

  if (obj) {
    if (dogfood(obj) >= MANFOOD) return false;
    if (cansee(mtmp->mx, mtmp->my)) {
      StrPrintF(ScratchBuffer, "%s devours the %s.", Monnam(mtmp),
		oc_names + objects[obj->otype].oc_name_offset);
      message(ScratchBuffer);
    }
    free_obj(obj, NULL);
  }
  
  
  // Ok, because of the different way I am doing stuff,
  // I think that the reallocing and copying is not needed.

  /*    mtmp2 = newmonst(sizeof(struct edog) + mtmp->mnamelth); */
  // ... initedog will take care of allocating the 'edog' part,
  // as for namelth, I have not decided how to implement that yet.
  /*    *mtmp2 = *mtmp; */
  /*    mtmp2->mxlth = sizeof(struct edog); */
  /*    if (mtmp->mnamelth) (void) strcpy(NAME(mtmp2), NAME(mtmp)); */
  initedog(mtmp); // this was mtmp2, of course
  /*    replmon(mtmp,mtmp2); */
  return true;
}
