/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"

coord mtrack[MTSZ];     /* monster track */

static Short do_chugw(monst_t *mtmp); // SEC_3; // XXXX had a bug here?
static void mpickgold(monst_t *mtmp) SEC_3;
static void mpickgems(monst_t *mtmp) SEC_3;
//static Boolean getwn(monst_t *mtmp) { return false; } // fake...
//static void wormdead(monst_t *mtmp) { return; } // fake...
//static void initworm(monst_t *mtmp) { return; } // fake...


// hey... when monsters are freed, if mtmp->extra != NULL, then free it TOO.
struct monst *fdmon;	/* chain of dead monsters, need not to be saved */
/* we do not free monsters immediately, in order to have their name
   available shortly after their demise */ // see monfree and dmonsfree.


Short warnlevel;		/* used by movemon and do_chugw */
Long lastwarntime;
Short lastwarnlev;
#define MAXWARN 6
static const Char *warnings[MAXWARN] = {
  "white", "pink", "red", "ruby", "purple", "black"
};


// There may be a bug in here.  Or in something this calls.
void movemon()
{
  monst_t *mtmp;
  Short fr;
  Boolean b;
  UChar ctr;

  warnlevel = 0;

  while (true) {
    /* find a monster that we haven't treated yet */
    /* note that mtmp or mtmp->nmon might get killed
       while mtmp moves, so we cannot just walk down the
       chain (even new monsters might get created!) */
    for (mtmp = fmon ; mtmp ; mtmp = mtmp->nmon)
      if (mtmp->mlastmoved < moves) goto next_mon;
    /* treated all monsters */
    break;

  next_mon:
    mtmp->mlastmoved = moves;

    /* most monsters drown in pools */
    {
      Boolean inpool, iseel;

      inpool = (get_cell_type(floor_info[mtmp->mx][mtmp->my]) == POOL);
      iseel = (mtmp->data->mlet == ';');
      if (inpool && !iseel) {
	if (cansee(mtmp->mx,mtmp->my)) {
	  StrPrintF(ScratchBuffer, "%s drowns.", Monnam(mtmp));
	  message(ScratchBuffer);
	}
	mondead(mtmp);
	continue;
      }
      /* but eels have a difficult time outside */
      if (iseel && !inpool) {
	if (mtmp->mhp > 1) mtmp->mhp--;
	ctr = mtmp->mflee_and_time & ~M_FLEEING;
	mtmp->mflee_and_time = (ctr + 2) | M_FLEEING; // flee for 2 extra turns
      }
    }

    if ((ctr = mtmp->mcansee_and_blinded & ~M_CAN_SEE)) {
      b = mtmp->mcansee_and_blinded & M_CAN_SEE;
      if (--ctr == 0) b = true;
      mtmp->mcansee_and_blinded = ctr | b;
    }
    if ((ctr = mtmp->mflee_and_time & ~M_FLEEING)) {
      b = mtmp->mflee_and_time & M_FLEEING;
      if (--ctr == 0) b = false;
      mtmp->mflee_and_time = ctr | b;
    }
    if (mtmp->bitflags & M_IS_MIMIC) continue;
    if (mtmp->mspeed != MSLOW || !(moves % 2)){
      /* continue if the monster died fighting */
      fr = -1;
      if (Conflict && cansee(mtmp->mx, mtmp->my)
	  && (fr = fightm(mtmp)) == 2)
	continue;
      if (fr < 0 && do_chugw(mtmp)) // xxx I have tracked heisenbug this far.
	continue;
    }
    if (mtmp->mspeed == MFAST && do_chugw(mtmp))
      continue;
  }

  warnlevel -= you.ulevel;
  if (warnlevel >= MAXWARN)
    warnlevel = MAXWARN-1;
  if (warnlevel >= 0)
    if (warnlevel > lastwarnlev || moves > lastwarntime + 5){
      Char *rr;
      switch(Warning & (LEFT_RING | RIGHT_RING)){
      case LEFT_RING:
	rr = "Your left ring glows";
	break;
      case RIGHT_RING:
	rr = "Your right ring glows";
	break;
      case LEFT_RING | RIGHT_RING:
	rr = "Both your rings glow";
	break;
      default:
	rr = "Your fingertips glow";
	break;
      }
      StrPrintF(ScratchBuffer, "%s %s!", rr, warnings[warnlevel]);
      message(ScratchBuffer);
      lastwarntime = moves;
      lastwarnlev = warnlevel;
    }

  /* remove all dead monsters */
  while ((mtmp = fdmon)) {
    fdmon = mtmp->nmon;
    if (mtmp->extra) free_me((VoidPtr) mtmp->extra); // XXX for dogs+guards
    if (mtmp->name) free_me((VoidPtr) mtmp->name); // for mon named by player.
    free_me((VoidPtr) mtmp);
  }

}


void justswld(monst_t *mtmp, Char *name)
{
  mtmp->mx = you.ux;
  mtmp->my = you.uy;
  you.ustuck = mtmp;
  pmon(mtmp);
  kludge("%s swallows you!", name);
  wait_for_event(); // more(); // XXXX need to test.  "hit any key to proceed"
  seeoff(true);
  you.uswallow = true;
  you.uswallowedtime = 0;
  swallowed();
}

void youswld(monst_t *mtmp, Short dam, Short die, Char *name)
{
  if (mtmp != you.ustuck) return;
  kludge("%s digests you!", name);
  you.uhp -= dam;
  if (you.uswallowedtime++ >= die) {	/* a3 */
    message("It totally digests you!");
    you.uhp = -1;
  }
  if (you.uhp < 1) done_in_by(mtmp); // XXX
  /* flags.botlx = 1;		/ * should we show status line ? */
}


static Short do_chugw(monst_t *mtmp)
{
  Short x = mtmp->mx;
  Short y = mtmp->my;
  Short d = do_chug(mtmp);
  Short dd;
  if (!d)		/* monster still alive */
    if (Warning)
      if (!mtmp->bitflags & M_IS_PEACEFUL)
	if (mtmp->data->mlevel > warnlevel)
	  if ((dd = dist(mtmp->mx,mtmp->my)) < dist(x,y))
	    if (dd < 100)
	      if (!canseemon(mtmp))
		warnlevel = mtmp->data->mlevel;
  return(d);
}

/* returns true if monster died moving, false otherwise */
Boolean do_chug(monst_t *mtmp)
{
  permonst_t *mdat;
  Short tmp=0, nearby, scared;

  if (mtmp->bitflags & M_IS_CHAMELEON && !rund(6))
    newcham(mtmp, &mons[dlevel+14+rund(CMNUM-14-dlevel)]);
  mdat = mtmp->data;
  if (mdat->mlevel < 0) {
    StrPrintF(ScratchBuffer,"BUG:bad monster %c (%d)",mdat->mlet,mdat->mlevel);
    message(ScratchBuffer); // panic!
    return false;
  }

  /* regenerate monsters */
  if ((!(moves % 20) || StrChr(MREGEN, mdat->mlet)) &&
      mtmp->mhp < mtmp->mhpmax)
    mtmp->mhp++;

  if (mtmp->bitflags & M_IS_FROZEN)
    return false; /* frozen monsters don't do anything */

  if (mtmp->bitflags & M_IS_ASLEEP) {
    /* wake up, or get out of here. */
    /* ettins are hard to surprise */
    /* Nymphs and Leprechauns do not easily wake up */
    if (cansee(mtmp->mx,mtmp->my) &&
	(!Stealth || (mdat->mlet == 'e' && rund(10))) &&
	(!StrChr("NL",mdat->mlet) || !rund(50)) &&
	(Aggravate_monster || StrChr("d1", mdat->mlet)
	 || (!rund(7) && !(mtmp->bitflags & M_IS_MIMIC) )))
      mtmp->bitflags &= ~M_IS_ASLEEP;
    else return false;
  }

  /* not frozen or sleeping: wipe out texts written in the dust */
  wipe_engr_at(mtmp->mx, mtmp->my, 1);

  /* confused monsters get unconfused with small probability */
  if (mtmp->bitflags & M_IS_CONFUSED && !rund(50))
    mtmp->bitflags &= ~M_IS_CONFUSED;

  /* some monsters teleport */
  if ((mtmp->mflee_and_time & M_FLEEING) &&
      StrChr("tNL", mdat->mlet) && !rund(40)) {
    rloc(mtmp);
    return false;
  }
  if (mdat->mmove < rnd(6)) return false;

  /* fleeing monsters might regain courage */
  if ((mtmp->mflee_and_time & M_FLEEING) && // m is fleeing,
      !(mtmp->mflee_and_time & ~M_FLEEING) && // but flee timer expired.
      mtmp->mhp == mtmp->mhpmax && !rund(25))
    mtmp->mflee_and_time = 0;

  nearby = (dist(mtmp->mx, mtmp->my) < 3);
  scared = (nearby && (sengr_at("Elbereth", you.ux, you.uy) ||
		       sobj_at(SCR_SCARE_MONSTER, you.ux, you.uy)));
  if (scared && !(mtmp->mflee_and_time & M_FLEEING)) {
    mtmp->mflee_and_time = M_FLEEING | (rund(7) ? rnd(10) : rnd(100));
  }

  if (!nearby ||
      (mtmp->mflee_and_time & M_FLEEING) ||
      (mtmp->bitflags & M_IS_CONFUSED) ||
      ((mtmp->bitflags & M_IS_INVISIBLE) && !rund(3)) ||
      (StrChr("BIuy", mdat->mlet) && !rund(4)) ||
      (mdat->mlet == 'L' && !you.ugold && (mtmp->mgold || rund(2))) ||
      (!(mtmp->mcansee_and_blinded & M_CAN_SEE) && !rund(4)) ||
      (mtmp->bitflags & M_IS_PEACEFUL)
      ) {
    tmp = m_move(mtmp,0);	/* 2: monster died moving */
    if (tmp == 2 || (tmp && mdat->mmove <= 12))
      return (tmp == 2);
  }

  if (!StrChr("Ea", mdat->mlet) && nearby &&
      !(mtmp->bitflags & M_IS_PEACEFUL) && you.uhp > 0 && !scared) {
    if (mon_hit_you(mtmp))
      return true;	/* monster died (e.g. 'y' or 'F') */
  }
  /* extra movement for fast monsters */
  if (mdat->mmove-12 > rnd(12)) tmp = m_move(mtmp,1);
  return (tmp == 2);
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
Short m_move(monst_t *mtmp, Short after)
{
  monst_t *mtmp2;
  Short nx,ny,omx,omy,appr,nearer,cnt,i,j;
  Int8 /*xchar*/ gx,gy,nix,niy,chcnt;
  Int8 chi;
  Boolean likegold=false, likegems=false, likeobjs=false;
  Char msym = mtmp->data->mlet;
  Int8 mmoved = 0;	/* not strictly nec.: chi >= 0 will do */
  coord poss[9];
  Short info[9];

  if (mtmp->bitflags & M_IS_FROZEN || mtmp->bitflags & M_IS_ASLEEP)
    return 0;
  if (mtmp->bitflags & M_IS_TRAPPED) {
    i = mon_in_trap(mtmp);
    if (i == 2) return 2;	/* he died */
    if (i == 1) return 0;	/* still in trap, so didn't move */
  }
  if (mtmp->bitflags & M_IS_HIDER && obj_at(mtmp->mx, mtmp->my) && rund(10))
    return 0;		/* do not leave hiding place */

#ifndef NOWORM
  if (mtmp->wormno)
    goto not_special;
#endif NOWORM

  /* my dog gets a special treatment */
  if (mtmp->bitflags & M_IS_TAME) {
    return( dog_move(mtmp, after) );
  }

  /* likewise for shopkeeper */
  if (mtmp->bitflags & M_IS_SHOPKEEPER) {
    mmoved = shk_move(mtmp);
    if (mmoved >= 0)
      goto postmov;
    mmoved = 0;		/* follow player outside shop */
  }

  /* and for the guard */
  if (mtmp->bitflags & M_IS_GUARD) {
    mmoved = gd_move();
    goto postmov;
  }

  /* teleport if that lies in our nature ('t') or when badly wounded ('1') */
  if ((msym == 't' && !rund(5)) ||
      (msym == '1' && (mtmp->mhp < 7 || (!xdnstair && !rund(5)) ||
		       get_cell_type(floor_info[you.ux][you.uy]) == STAIRS))) {
    if (mtmp->mhp < 7 || (msym == 't' && rund(2)))
      rloc(mtmp);
    else
      mnexto(mtmp);
    mmoved = 1;
    goto postmov;
  }

  /* spit fire ('D') or use a wand ('1') when appropriate */
  if (StrChr("D1", msym))
    inrange(mtmp);

  if (msym == 'U' && !(mtmp->bitflags & M_IS_CANCELLED) && canseemon(mtmp) &&
      (mtmp->mcansee_and_blinded & M_CAN_SEE) && rund(5)) {
    if (!Confusion) {
      StrPrintF(ScratchBuffer, "%s's gaze has confused you!", Monnam(mtmp));
      message(ScratchBuffer);
    } else
      message("You are getting more and more confused.");
    if (rund(3)) mtmp->bitflags |= M_IS_CANCELLED;
    Confusion += dice(3,4);		/* timeout */
  }
 not_special:
  if (!(mtmp->mflee_and_time & M_FLEEING) && you.uswallow && you.ustuck!=mtmp)
    return true;
  appr = 1;
  if (mtmp->mflee_and_time & M_FLEEING) appr = -1;
  if ((mtmp->bitflags & M_IS_CONFUSED) || Invis ||
      !(mtmp->mcansee_and_blinded & M_CAN_SEE) ||
      (StrChr("BIy", msym) && !rund(3)))
    appr = 0;
  omx = mtmp->mx;
  omy = mtmp->my;
  gx = you.ux;
  gy = you.uy;
  if (msym == 'L' && appr == 1 && mtmp->mgold > you.ugold)
    appr = -1;

  /* random criterion for 'smell' or track finding ability
     should use mtmp->msmell or sth
  */
  if (msym == '@' ||
      ('a' <= msym && msym <= 'z')) {
    coord *cp;
    Int8 mroom;
    mroom = inroom(omx,omy);
    if (mroom < 0 || mroom != inroom(you.ux,you.uy)){
      cp = get_track(omx,omy);
      if (cp){
	gx = cp->x;
	gy = cp->y;
      }
    }
  }

  /* look for gold or jewels nearby */
  likegold = (StrChr("LOD", msym) != NULL);
  likegems = (StrChr("ODu", msym) != NULL);
  likeobjs = mtmp->bitflags & M_IS_HIDER;
#define	SRCHRADIUS	25
  {
    Int8 /*xchar*/ mind = SRCHRADIUS;		/* not too far away */
    Short dd;
    if (likegold) {
      gold_t *gold;
      for (gold = fgold ; gold ; gold = gold->ngold)
	if ((dd = DIST(omx,omy,gold->gx,gold->gy)) < mind) {
	  mind = dd;
	  gx = gold->gx;
	  gy = gold->gy;
	}
    }
    if (likegems || likeobjs) {
      obj_t *otmp;
      for (otmp = fobj ; otmp ; otmp = otmp->nobj)
	if (likeobjs || otmp->olet == GEM_SYM)
	  if (msym != 'u' ||
	      objects[otmp->otype].g_val != 0)
	    if ((dd = DIST(omx,omy,otmp->ox,otmp->oy)) < mind) {
	      mind = dd;
	      gx = otmp->ox;
	      gy = otmp->oy;
	    }
    }
    if (mind < SRCHRADIUS && appr == -1) {
      if (dist(omx,omy) < 10) {
	gx = you.ux;
	gy = you.uy;
      } else
	appr = 1;
    }
  }
  nix = omx;
  niy = omy;
  cnt = mfindpos(mtmp,poss,info,
		 msym == 'u' ? NOTONL :
		 (msym == '@' || msym == '1') ? (ALLOW_SSM | ALLOW_TRAPS) :
		 StrChr(UNDEAD, msym) ? NOGARLIC : ALLOW_TRAPS);
  /* ALLOW_ROCK for some monsters ? */
  chcnt = 0;
  chi = -1;
  for (i=0 ; i < cnt ; i++) {
    nx = poss[i].x;
    ny = poss[i].y;
    for (j = 0 ; j < MTSZ && j < cnt-1 ; j++)
      if (nx == mtmp->mtrack[j].x && ny == mtmp->mtrack[j].y)
	if (rund(4*(cnt-j)))
	  goto nxti;
#ifdef STUPID
    /* some stupid compilers think that this is too complicated */
    {
      Short d1 = DIST(nx,ny,gx,gy);
      Short d2 = DIST(nix,niy,gx,gy);
      nearer = (d1 < d2);
    }
#else
    nearer = (DIST(nx,ny,gx,gy) < DIST(nix,niy,gx,gy));
#endif STUPID
    if ((appr == 1 && nearer) || (appr == -1 && !nearer) ||
	!mmoved ||
	(!appr && !rund(++chcnt))){
      nix = nx;
      niy = ny;
      chi = i;
      mmoved = 1;
    }
  nxti:
    ;
  }
  if (mmoved) {
    if (info[(Short) chi] & ALLOW_M) {
      mtmp2 = mon_at(nix,niy);
      if (hitmm(mtmp,mtmp2) == 1 && rund(4) &&
	  hitmm(mtmp2,mtmp) == 2)
	return 2;
      return 0;
    }
    if (info[(Short) chi] & ALLOW_U) {
      hit_you(mtmp, dice(mtmp->data->damn, mtmp->data->damd)+1);
      return 0;
    }
    mtmp->mx = nix;
    mtmp->my = niy;
    for (j = MTSZ-1 ; j > 0 ; j--)
      mtmp->mtrack[j] = mtmp->mtrack[j-1];
    mtmp->mtrack[0].x = omx;
    mtmp->mtrack[0].y = omy;
#ifndef NOWORM
    if (mtmp->wormno) worm_move(mtmp);
#endif NOWORM
  } else {
    if (msym == 'u' && rund(2)) {
      rloc(mtmp);
      return 0;
    }
#ifndef NOWORM
    if (mtmp->wormno) worm_nomove(mtmp);
#endif NOWORM
  }
 postmov:
  if (mmoved == 1) {
    if (mon_in_trap(mtmp) == 2)	/* he died */
      return 2;
    if (likegold) mpickgold(mtmp);
    if (likegems) mpickgems(mtmp);
    if (mtmp->bitflags & M_IS_HIDER) mtmp->bitflags |= M_IS_UNDETECTED;
  }
  pmon(mtmp);
  return(mmoved);
}





static void mpickgold(monst_t *mtmp)
{
  gold_t *gold;
  while ((gold = gold_at(mtmp->mx, mtmp->my))) {
    mtmp->mgold += gold->amount;
    freegold(gold);
    if (floor_symbol[mtmp->mx][mtmp->my] == GOLD_SYM)
      newsym(mtmp->mx, mtmp->my);
  }
}

static void mpickgems(monst_t *mtmp)
{
  obj_t *otmp;
  for (otmp = fobj ; otmp ; otmp = otmp->nobj)
    if (otmp->olet == GEM_SYM)
      if (otmp->ox == mtmp->mx && otmp->oy == mtmp->my)
	if (mtmp->data->mlet != 'u' || objects[otmp->otype].g_val != 0) {
	  unlink_obj(otmp);
	  mpickobj(mtmp, otmp);
	  if (floor_symbol[mtmp->mx][mtmp->my] == GEM_SYM)
	    newsym(mtmp->mx, mtmp->my);	/* %% */
	  return;	/* pick only one object */
	}
}


/* return number of acceptable neighbour positions */
// mfindpos was mfndpos before I bought a vowel
Short mfindpos(monst_t *mon, coord poss[9], Short info[9], Short flag)
{
  Short x,y,nx,ny,cnt = 0;
  monst_t *mtmp;
  Boolean pool;
  UChar old_tile, next_tile;

  x = mon->mx;
  y = mon->my;
  old_tile = get_cell_type(floor_info[x][y]); // nowtyp = levl[x][y].typ;

  pool = (mon->data->mlet == ';');
 nexttry:
  /* eels prefer the water, but if there is no water nearby,
     they will crawl over land */
  if (mon->bitflags & M_IS_CONFUSED) {
    flag |= ALLOW_ALL;
    flag &= ~NOTONL;
  }
  for (nx = x-1 ; nx <= x+1 ; nx++)
    for (ny = y-1 ; ny <= y+1 ; ny++)
      if ( (nx != x || ny != y) &&
	   (!(OUT_OF_BOUNDS(nx,ny))) &&
	   //	   (!IS_ROCK(ntyp = levl[nx][ny].typ)) &&  ... translation:
	   (!IS_ROCK(next_tile = get_cell_type(floor_info[nx][ny]))) &&
	   (!(nx != x && ny != y && (old_tile == DOOR ||
				     next_tile == DOOR))) &&
	   ((next_tile == POOL) == pool)
	   ) {
	info[cnt] = 0;
	if (nx == you.ux && ny == you.uy) {
	  if (!(flag & ALLOW_U)) continue;
	  info[cnt] = ALLOW_U;
	} else if ((mtmp = mon_at(nx,ny))) {
	  if (!(flag & ALLOW_M)) continue;
	  info[cnt] = ALLOW_M;
	  if (mtmp->bitflags & M_IS_TAME) {
	    if (!(flag & ALLOW_TM)) continue;
	    info[cnt] |= ALLOW_TM;
	  }
	}
	if (sobj_at(CLOVE_OF_GARLIC, nx, ny)) {
	  if (flag & NOGARLIC) continue;
	  info[cnt] |= NOGARLIC;
	}
	if (sobj_at(SCR_SCARE_MONSTER, nx, ny) ||
	    (!(mon->bitflags & M_IS_PEACEFUL) &&
	     sengr_at("Elbereth", nx, ny))) {
	  if (!(flag & ALLOW_SSM)) continue;
	  info[cnt] |= ALLOW_SSM;
	}
	if (sobj_at(ENORMOUS_ROCK, nx, ny)) {
	  if (!(flag & ALLOW_ROCK)) continue;
	  info[cnt] |= ALLOW_ROCK;
	}
	if (!Invis && online(nx,ny)){
	  if (flag & NOTONL) continue;
	  info[cnt] |= NOTONL;
	}
	/* we cannot avoid traps of an unknown kind */
	{
	  trap_t *ttmp = trap_at(nx, ny);
	  Short tt;
	  if (ttmp) {
	    tt = 1 << (get_trap_type(ttmp->trap_info));
	    if (mon->mtraps_seen & tt) {
	      if (!(flag & tt)) continue;
	      info[cnt] |= tt;
	    }
	  }
	}
	poss[cnt].x = nx;
	poss[cnt].y = ny;
	cnt++;
      }
  if (!cnt && pool && old_tile != POOL) {
    pool = false;
    goto nexttry;
  }
  return(cnt);
}




// distance to you..
Short dist(Short x, Short y)
{
  return((x-you.ux)*(x-you.ux) + (y-you.uy)*(y-you.uy));
}





void poisoned(Char *string, Char *pname)
{
  Short i;

  if (Blind) message("It was poisoned.");
  else {
    StrPrintF(ScratchBuffer, "The %s was poisoned!", string);
    message(ScratchBuffer);
  }
  if (Poison_resistance) {
    message("The poison doesn't seem to affect you.");
    return;
  }
  i = rund(10);
  if (i == 0) {
    you.uhp = -1;
    message("I am afraid the poison was deadly ...");
  } else if (i <= 5) {
    losestr(rund(3)+3);
  } else {
    losehp(rund(10)+6, pname);
  }
  if (you.uhp < 1) {
    // (pname is either a pointer into mon_names or a const text string.)
    killer = pname;
    done("died");
    return;
  }
}

void mondead(monst_t *mtmp)
{
  release_objs(mtmp, true);
  unpmon(mtmp);
  unlink_mon(mtmp);
  unstuck(mtmp);
  if (mtmp->bitflags & M_IS_SHOPKEEPER) shkdead(mtmp);
  if (mtmp->bitflags & M_IS_GUARD) gddead();
#ifndef NOWORM
  if (mtmp->wormno) wormdead(mtmp);
#endif NOWORM
  monfree(mtmp);
}


// xxx how about replmon - do I actually use it?
/* called when monster is moved to larger structure */
void replmon(monst_t *mtmp, monst_t *mtmp2)
{
  unlink_mon(mtmp);
  monfree(mtmp);
  mtmp2->nmon = fmon;
  fmon = mtmp2;
  if (you.ustuck == mtmp) you.ustuck = mtmp2;
  if (mtmp2->bitflags & M_IS_SHOPKEEPER) replshk(mtmp,mtmp2); // xxx untested
  if (mtmp2->bitflags & M_IS_GUARD) replgd(mtmp,mtmp2); // xxx I haven't tested
}


// I have named "relmon" to "unlink_mon", just as "freeobj" is now "unlink_obj"
void unlink_mon(monst_t *mon) // this was "relmon()".. useless name
{
  monst_t *mtmp;

  if (mon == fmon) fmon = fmon->nmon;
  else {
    for (mtmp = fmon; mtmp->nmon != mon; mtmp = mtmp->nmon)
      if (!mtmp) { alert_message("error in unlink_mon"); return; }
    mtmp->nmon = mon->nmon;
  }
}

// used only in mon.c and worm.c ...
void monfree(monst_t *mtmp)
{
  mtmp->nmon = fdmon;
  fdmon = mtmp;
}

// dmonsfree has been INLINED.


void unstuck(monst_t *mtmp)
{
  if (you.ustuck == mtmp) {
    if (you.uswallow) {
      you.ux = mtmp->mx;
      you.uy = mtmp->my;
      you.uswallow = 0;
      setsee();
      refresh(); // docrt();
    }
    you.ustuck = NULL;
  }
}

/* it is the callers responsibility to check that there is room for c */
// This is like StrCat except it appends a single character.  was in end.c
void charcat(Char *s, Char c)
{
  while (*s) s++;
  *s++ = c;
  *s = 0;
}


void killed(monst_t *mtmp)
{
  Short tmp,nk,x,y;
  permonst_t *mdat;

  if (mtmp->bitflags & M_IS_CHAMELEON) mtmp->data = PM_CHAMELEON;
  mdat = mtmp->data;
  if (Blind) message("You destroy it!");
  else {
    StrPrintF(ScratchBuffer, "You destroy %s!",
	      (mtmp->bitflags & M_IS_TAME) ? amonnam(mtmp, "poor") :
	                                     monnam(mtmp));
    message(ScratchBuffer);
  }
  if (you.umconf) {
    if (!Blind) message("Your hands stop glowing blue.");
    you.umconf = false;
  }

  /* count killed monsters */
#define	MAXMONNO	100
  nk = 1;		      /* in case we cannot find it in mons */
  tmp = mdat - mons;    /* index in mons array (if not 'd', '@', ...) */
  if (tmp >= 0 && tmp < CMNUM+2) {
    extern Char fut_geno[];
    you.nr_killed[tmp]++;
    if ((nk = you.nr_killed[tmp]) > MAXMONNO &&
	!StrChr(fut_geno, mdat->mlet))
      charcat(fut_geno, mdat->mlet);
  }

  /* punish bad behaviour */
  if (mdat->mlet == '@') { Telepat = 0; you.uluck -= 2; }
  if (mtmp->bitflags & (M_IS_PEACEFUL | M_IS_TAME)) you.uluck--;
  if (mdat->mlet == 'u') you.uluck -= 5;
  if ((Short)you.uluck < LUCKMIN) you.uluck = LUCKMIN;

  /* give experience points */
  tmp = 1 + mdat->mlevel * mdat->mlevel;
  if (mdat->ac < 3) tmp += 2*(7 - mdat->ac);
  if (StrChr("AcsSDXaeRTVWU&In:P", mdat->mlet))
    tmp += 2*mdat->mlevel;
  if (StrChr("DeV&P",mdat->mlet)) tmp += (7*mdat->mlevel);
  if (mdat->mlevel > 6) tmp += 50;
  if (mdat->mlet == ';') tmp += 1000;

#ifdef NEW_SCORING // New scoring was turned off when I found it.  --bas
  /* ------- recent addition: make nr of points decrease
     when this is not the first of this kind */
  {
    Short ul = you.ulevel;
    Short ml = mdat->mlevel;
    Short tmp2;

    if (ul < 14)    /* points are given based on present and future level */
      for (tmp2 = 0; !tmp2 || ul + tmp2 <= ml; tmp2++)
	if (you.uexp + 1 + (tmp + ((tmp2 <= 0) ? 0 : 4<<(tmp2-1)))/nk
	    >= 10*pow((unsigned)(ul-1))) // surely pow wants 2 args?? --bas
	  if (++ul == 14) break;

    tmp2 = ml - ul -1;
    tmp = (tmp + ((tmp2 < 0) ? 0 : 4<<tmp2))/nk;
    if (!tmp) tmp = 1;
  }
  /* note: ul is not necessarily the future value of you.ulevel */
  /* ------- end of recent valuation change ------- */
#endif NEW_SCORING

  more_experienced(tmp,0);
  flags.botl |= BOTL_EXP;
  while (you.ulevel < 14 && you.uexp >= newuexp()) {
    StrPrintF(ScratchBuffer, "Welcome to experience level %u.", ++you.ulevel);
    message(ScratchBuffer);
    tmp = rnd(10);
    if (tmp < 3) tmp = rnd(10);
    you.uhpmax += tmp;
    you.uhp += tmp;
    flags.botl |= BOTL_HP;
  }

  /* dispose of monster and make cadaver */
  x = mtmp->mx;	y = mtmp->my;
  mondead(mtmp);
  tmp = mdat->mlet;
  if (tmp == 'm') { /* he killed a minotaur, give him a wand of digging */
    /* note: the dead minotaur will be on top of it! */
    mksobj_at(WAN_DIGGING, x, y);
    /* if (cansee(x,y)) atl(x,y,fobj->olet); */ // I found it commented.
    stackobj(fobj);
  } else
#ifndef NOWORM
    if (tmp == 'w') {
      mksobj_at(WORM_TOOTH, x, y);
      stackobj(fobj);
    } else
#endif	NOWORM
      if (!letter(tmp) || (!StrChr("mw", tmp) && !rund(3))) tmp = 0;

  if (ACCESSIBLE(get_cell_type(floor_info[x][y]))) /* might be mimic in wall
						      or dead eel*/
    if (x != you.ux || y != you.uy)	/* might be here after swallowed */
      if (StrChr("NTVm&",mdat->mlet) || rund(5)) {
	obj_t *obj2 = mkobj_at(tmp,x,y);
	if (cansee(x,y))
	  print(x,y,obj2->olet);
	stackobj(obj2);
      }
}


void kludge(Char *str, Char *arg)
{
  if (Blind) {
    if (*str == '%') StrPrintF(ScratchBuffer, str, "It");
    else StrPrintF(ScratchBuffer, str, "it");
  } else StrPrintF(ScratchBuffer, str, arg);
  message(ScratchBuffer);
}


void res_cham()	/* force all chameleons to become normal */ // was rescham
{
  monst_t *mtmp;

  for (mtmp = fmon ; mtmp ; mtmp = mtmp->nmon)
    if (mtmp->bitflags & M_IS_CHAMELEON) {
      mtmp->bitflags &= ~M_IS_CHAMELEON;
      newcham(mtmp, PM_CHAMELEON);
    }
}

/* make a chameleon look like a new monster */
/* returns 1 if the monster actually changed */
Boolean newcham(monst_t *mtmp, permonst_t *mdat)
{
  Short mhp, hpn, hpd;

  if (mdat == mtmp->data) return false;	/* still the same monster */

#ifndef NOWORM
  if (mtmp->wormno) wormdead(mtmp);	/* throw tail away */
#endif NOWORM

  if (you.ustuck == mtmp) {
    if (you.uswallow) {
      you.uswallow = false;
      you.uswallowedtime = 0;
      mnexto(mtmp);
      refresh(); //      docrt ();
      prme();
    }
    you.ustuck = NULL;
  }
  hpn = mtmp->mhp;
  hpd = (mtmp->data->mlevel)*8;
  if (!hpd) hpd = 4;
  mtmp->data = mdat;
  mhp = (mdat->mlevel)*8;
  /* new hp: same fraction of max as before */
  mtmp->mhp = 2 + (hpn*mhp)/hpd;
  hpn = mtmp->mhpmax;
  mtmp->mhpmax = 2 + (hpn*mhp)/hpd;
  if (mdat->mlet == 'I')
    mtmp->bitflags |= M_IS_INVISIBLE;
  else mtmp->bitflags &= ~M_IS_INVISIBLE;
  //  mtmp->minvis = (mdat->mlet == 'I') ? 1 : 0;

#ifndef NOWORM
  if (mdat->mlet == 'w' && getwn(mtmp)) initworm(mtmp);
  /* perhaps we should clear mtmp->mtame here? */
#endif NOWORM

  unpmon(mtmp);	/* necessary for 'I' and to force pmon */
  pmon(mtmp);
  return true;
}


/* Make monster mtmp next to you (if possible) */
void mnexto(monst_t *mtmp)
{
  PointType mm;
  mm = enexto(you.ux, you.uy);
  mtmp->mx = mm.x;
  mtmp->my = mm.y;
  pmon(mtmp);
}

//static Boolean ishuman(monst_t *mtmp)
//{
//  return(mtmp->data->mlet == '@');
//}
#define IS_HUMAN(a) ((a)->data->mlet == '@')

void setmangry(monst_t *mtmp)
{
  if (!(mtmp->bitflags & M_IS_PEACEFUL)) return;
  if (mtmp->bitflags & M_IS_TAME) return;
  mtmp->bitflags &= ~M_IS_PEACEFUL;//mpeaceful = 0;
  if (IS_HUMAN(mtmp)) {
    StrPrintF(ScratchBuffer, "%s gets angry!", Monnam(mtmp));
    message(ScratchBuffer);
  }
}

/* not one hundred procent correct: now a snake may hide under an
   invisible object */
Boolean canseemon(monst_t *mtmp)
{
  return((!(mtmp->bitflags & M_IS_INVISIBLE) || See_invisible)
	 && (!(mtmp->bitflags & M_IS_HIDER) || !obj_at(mtmp->mx,mtmp->my))
	 && cansee(mtmp->mx, mtmp->my));
}


