/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"

extern previous_state curr_state;

static void vtele() SEC_3;
static void teleds(Short nux, Short nuy) SEC_3;
static Boolean teleok(Short x, Short y) SEC_3;

Int8 maxdlevel = 1;

extern const Char mlarge[];

// used here and in search.c:
const Char *traps[TRAPNUM] = {
  " bear trap",
  "n arrow trap",
  " dart trap",
  " trapdoor",
  " teleportation trap",
  " pit",
  " sleeping gas trap",
  " piercer",
  " mimic"
};


trap_t * maketrap(Short x, Short y, Short trap_type)
{
  trap_t *ttmp;
  ttmp = (trap_t *) md_malloc(sizeof(trap_t));

  ttmp->ntrap = ftrap; // next trap!  may need to make this not-a-list.
  ftrap = ttmp; // make us the head of the list

  ttmp->tx = x;
  ttmp->ty = y;

  ttmp->trap_info = trap_type; // trap_seen == 0, trap_once == 0.

  return ttmp;
}


void deltrap(trap_t *trap) // was in invent.c!
{
  trap_t *ttmp;

  if (trap == ftrap) ftrap = ftrap->ntrap;
  else {
    for (ttmp = ftrap; ttmp->ntrap != trap; ttmp = ttmp->ntrap)
      if (!ttmp) { alert_message("error in deltrap"); return; }
    ttmp->ntrap = trap->ntrap;
  }
  free_me((VoidPtr) trap); //  free((char *) trap);
}



void do_trap(trap_t *trap) // was dotrap
{
  Short ttype = get_trap_type(trap->trap_info);

  nomul(0);
  if (get_trap_seen(trap->trap_info) && !rund(5) && ttype != PIT) {
    StrPrintF(ScratchBuffer, "You escape a%s.", traps[ttype]);
    message(ScratchBuffer);
  } else {
    trap->trap_info |= SEEN_TRAP;
    switch(ttype) {
    case SLP_GAS_TRAP:
      message("A cloud of gas puts you to sleep!");
      nomul(-rnd(25));
      break;
    case BEAR_TRAP:
      if (Levitation) {
	message("You float over a bear trap.");
	break;
      }
      you.utrap = 4 + rund(4);
      you.utraptype = TT_BEARTRAP;
      message("A bear trap closes on your foot!");
      break;
    case PIERC:
      deltrap(trap);
      if (makemon(PM_PIERCER, you.ux, you.uy)) {
	message("A piercer suddenly drops from the ceiling!");
	if (uarmh)
	  message("Its blow glances off your helmet.");
	else
	  thing_hit_you(3, dice(4,6), "falling piercer");
      }
      break;
    case ARROW_TRAP:
      message("An arrow shoots out at you!");
      if (!thing_hit_you(8, rnd(6), "arrow")){
	mksobj_at(ARROW, you.ux, you.uy);
	fobj->quantity = 1;
      }
      break;
    case TRAPDOOR:
      if (!xdnstair) {
	message("A trap door in the ceiling opens and a rock falls on your head!");
	if (uarmh) message("Fortunately, you are wearing a helmet!");
	losehp((uarmh ? 2 : dice(2,10)), "falling rock");
	mksobj_at(ROCK, you.ux, you.uy);
	fobj->quantity = 1;
	stackobj(fobj);
	if (Invisible) newsym(you.ux, you.uy);
      } else {
	Short newlevel = dlevel + 1;
	while (!rund(4) && newlevel < 29)
	  newlevel++;
	message("A trap door opens up under you!");
	if (Levitation || you.ustuck) {
	  message("For some reason you don't fall in.");
	  break;
	}

	goto_level(newlevel, false);
      }
      break;
    case DART_TRAP:
      message("A little dart shoots out at you!");
      if (thing_hit_you(7, rnd(3), "little dart")) {
	if (!rund(6))
	  poisoned("dart", "poison dart");
      } else {
	mksobj_at(DART, you.ux, you.uy);
	fobj->quantity = 1;
      }
      break;
    case TELEP_TRAP:
      if (get_trap_once(trap->trap_info)) {
	deltrap(trap);
	newsym(you.ux,you.uy);
	vtele();
      } else {
	newsym(you.ux,you.uy);
	tele();
      }
      break;
    case PIT:
      if (Levitation) {
	message("A pit opens up under you!");
	message("You don't fall in!");
	break;
      }
      message("You fall into a pit!");
      you.utrap = rund(6) + 2;
      you.utraptype = TT_PIT;
      losehp(rnd(6),"fall into a pit");
      selftouch("Falling, you");
      break;
    default:
      StrPrintF(ScratchBuffer, "BUG: You hit a trap with info=%u",
		trap->trap_info);
      message(ScratchBuffer);
    }
  }
}

Short mon_in_trap(monst_t *mtmp) // was mintrap
{
  trap_t *trap = trap_at(mtmp->mx, mtmp->my);
  Boolean wasintrap = (mtmp->bitflags & M_IS_TRAPPED);

  if (!trap) {
    mtmp->bitflags &= ~M_IS_TRAPPED;	/* perhaps teleported? */
  } else if (wasintrap) {
    if (!rund(40)) mtmp->bitflags &= ~M_IS_TRAPPED;
  } else {
    Short tt = get_trap_type(trap->trap_info);
    Short in_sight = cansee(mtmp->mx, mtmp->my);

    if (mtmp->mtraps_seen & (1 << tt)) {
      /* he has been in such a trap - perhaps he escapes */
      if (rund(4)) return 0;
    }
    mtmp->mtraps_seen |= (1 << tt);
    switch (tt) {
    case BEAR_TRAP:
      if (StrChr(mlarge, mtmp->data->mlet)) {
	if (in_sight) {
	  StrPrintF(ScratchBuffer,"%s is caught in a bear trap!",Monnam(mtmp));
	  message(ScratchBuffer);
	} else
	  if (mtmp->data->mlet == 'o')
	    message("You hear the roaring of an angry bear!");
	mtmp->bitflags |= M_IS_TRAPPED;
      }
      break;
    case PIT:
      /* there should be a mtmp/data -> floating */
      if (!StrChr("EywBfk'& ", mtmp->data->mlet)) { /* ab */
	mtmp->bitflags |= M_IS_TRAPPED;
	if (in_sight) {
	  StrPrintF(ScratchBuffer, "%s falls in a pit!", Monnam(mtmp));
	  message(ScratchBuffer);
	}
      }
      break;
    case SLP_GAS_TRAP:
      if (!(mtmp->bitflags & (M_IS_ASLEEP | M_IS_FROZEN))) {
	mtmp->bitflags |= M_IS_ASLEEP;
	if (in_sight) {
	  StrPrintF(ScratchBuffer, "%s suddenly falls asleep!", Monnam(mtmp));
	  message(ScratchBuffer);
	}
      }
      break;
    case TELEP_TRAP:
      rloc(mtmp);
      if (in_sight && !cansee(mtmp->mx,mtmp->my)) {
	StrPrintF(ScratchBuffer, "%s suddenly disappears!", Monnam(mtmp));
	  message(ScratchBuffer);
      }
      break;
    case ARROW_TRAP:
      if (in_sight) {
	StrPrintF(ScratchBuffer, "%s is hit by an arrow!", Monnam(mtmp));
	message(ScratchBuffer);
      }
      mtmp->mhp -= 3;
      break;
    case DART_TRAP:
      if (in_sight) {
	StrPrintF(ScratchBuffer, "%s is hit by a dart!", Monnam(mtmp));
	message(ScratchBuffer);
      }
      mtmp->mhp -= 2;
      /* not mondied here !! */
      break;
    case TRAPDOOR:
      if (!xdnstair) {
	mtmp->mhp -= 10;
	if (in_sight) {
	  StrPrintF(ScratchBuffer,
		    "A trap door in the ceiling opens and a rock hits %s!",
		    monnam(mtmp));
	  message(ScratchBuffer);
	}
	break;
      }
      if (mtmp->data->mlet != 'w') {
	fall_down(mtmp);
	if (in_sight) {
	  StrPrintF(ScratchBuffer, "Suddenly, %s disappears out of sight.",
		    monnam(mtmp));
	  message(ScratchBuffer);
	}
	return 2;	/* no longer on this level */
      }
      break;
    case PIERC:
      break;
    default:
      message("BUG: Some monster encountered a strange trap.");
    }
  }
  return ((mtmp->bitflags & M_IS_TRAPPED) != 0);
}

void selftouch(Char *arg)
{
  if (uwep && uwep->otype == DEAD_COCKATRICE) {
    StrPrintF(ScratchBuffer, "%s touch the dead cockatrice.", arg);
    message(ScratchBuffer);
    message("You turn to stone.");
    killer = oc_names + objects[uwep->otype].oc_name_offset; // xxx
    done("died"); // XXX
    return;
  }
}


void float_up()
{
  if (you.utrap) {
    if (you.utraptype == TT_PIT) {
      you.utrap = 0;
      message("You float up, out of the pit!");
    } else {
      message("You float up, only your leg is still stuck.");
    }
  } else
    message("You start to float in the air!");
}

void float_down()
{
  trap_t *trap;
  message("You float gently to the ground.");
  trap = trap_at(you.ux, you.uy);
  if (trap)
    switch(get_trap_type(trap->trap_info)) {
    case PIERC:
      break;
    case TRAPDOOR:
      if (!xdnstair || you.ustuck) break;
      /* fall into next case */
    default:
      do_trap(trap);
    }
  pickup(true);
}

static void vtele()
{
  room_t *croom;
  extern room_t rooms[MAX_ROOMS]; // in make_level.c .... SIGH...
  for (croom = &rooms[0] ; croom->hx >= 0 ; croom++)
    if (croom->rtype == VAULT) {
      Short x,y;

      x = rund(2) ? croom->lx : croom->hx;
      y = rund(2) ? croom->ly : croom->hy;
      if (teleok(x,y)) {
	teleds(x,y);
	return;
      }
    }
  tele();
}

// XXXX in tele, if you have "Teleport_control" you can choose where to go.
// gar gar gar.
// similarly, level_tele will let you choose what level to teleport to.
// I will put these off, because they require UI Frobbing.
void tele()
{
  //  Short nux,nuy;
  if (Teleport_control) {
    extern Boolean took_time;
    //    coord cc;
    took_time = false;
    curr_state.cmd = '\024';
    curr_state.item = NULL;
    curr_state.mode = MODE_GETCELL;
    message("To what position do you want to be teleported?");
    /* 1: force valid */
    //    cc = getpos(1, "the desired position");
    /* possible extensions: introduce a small error if
       magic power is low; allow transfer to solid rock */
  } else {
    tele_finish(0, 0, false);
  }
}

void tele_finish(Short x, Short y, Boolean controlled)
{
  Short nux,nuy;

  if (controlled) {
    if (teleok(x, y)) {
      teleds(x, y);
      return;
    }
    message("Sorry ...");
  }
  do {
    nux = rnd(DCOLS-1);
    nuy = rund(DROWS);
  } while (!teleok(nux, nuy));
  teleds(nux, nuy);
}

static void teleds(Short nux, Short nuy)
{
  if (Punished) unplacebc();
  unsee();
  you.utrap = 0;
  you.ustuck = 0;
  you.ux = nux;
  you.uy = nuy;
  setsee();
  if (Punished) placebc(true);
  if (you.uswallow) {
    you.uswallowedtime = you.uswallow = 0;
    refresh(); //docrt();
  }
  nomul(0);
  if (get_cell_type(floor_info[nux][nuy]) == POOL && !Levitation)
    drown();
  inshop();
  pickup(true);
  if (!Blind) read_engr_at(you.ux,you.uy);
}

static Boolean teleok(Short x, Short y)
{	/* might throw him into a POOL */
  return( !OUT_OF_BOUNDS(x,y) &&
	  !IS_ROCK(get_cell_type(floor_info[x][y])) &&
	  !mon_at(x,y) &&
	  !sobj_at(ENORMOUS_ROCK,x,y) &&
	  !trap_at(x,y)
	  );
  /* Note: gold is permitted (because of vaults) */
}

Boolean dotele()
{
  if (
#ifdef WIZARD
      !wizard &&
#endif WIZARD
      (!Teleportation || you.ulevel < 6 ||
       (you.character_class != CLASS_WIZARD && you.ulevel < 10))) {
    message("You are not able to teleport at will.");
    return false;
  }
  if (you.uhunger <= 100 || you.ustr < 6) {
    message("You lack the strength for a teleport spell."); //'lack' was 'miss'
    return true;
  }
  tele();
  morehungry(100);
  return true;
}

void placebc(Boolean attach)
{
  if (!uchain || !uball) {
    message("BUG: Where are your chain and ball??");
    return;
  }
  uball->ox = uchain->ox = you.ux;
  uball->oy = uchain->oy = you.uy;
  if (attach) {
    uchain->nobj = fobj;
    fobj = uchain;
    if (!carried(uball)) {
      uball->nobj = fobj;
      fobj = uball;
    }
  }
}

void unplacebc()
{
  if (!carried(uball)) {
    unlink_obj(uball);
    unpobj(uball);
  }
  unlink_obj(uchain);
  unpobj(uchain);
}

// Caller must have already prompted user and got 'newlevel' if applicable.
void level_tele(Short newlevel, Boolean controlled)
{
  if (controlled) {
    /* // This is the sort of thing the caller should do:
    Char buf[BUFSZ];

    do {
      message("To what level do you want to teleport? [type a number] ");
      getlin(buf);
    } while (!digit(buf[0]) && (buf[0] != '-' || !digit(buf[1])));
    newlevel = atoi(buf);
    */
  } else {
    newlevel  = 5 + rund(20);	/* 5 - 24 */
    if (dlevel == newlevel) {
      if (!xdnstair) newlevel--; else newlevel++;
    }
  }
  if (newlevel >= 30) {
    if (newlevel > MAXLEVEL) newlevel = MAXLEVEL;
    message("You arrive at the center of the earth ...");
    message("Unfortunately it is here that hell is located.");
    if (Fire_resistance) {
      message("But the fire doesn't seem to harm you.");
    } else {
      message("You burn to a crisp.");
      dlevel = maxdlevel = newlevel;
      killer = "visit to the hell";
      done("burned");
      return;
    }
  }
  if (newlevel < 0) {
    newlevel = 0;
    message("You are now high above the clouds ...");
    if (Levitation) {
      message("You float gently down to earth.");
      done("escaped");
      return;
    }
    message("Unfortunately, you don't know how to fly.");
    message("You fall down a few thousand feet and break your neck.");
    dlevel = 0;
    killer = "fall";
    done("died");
    return;
  }

  goto_level(newlevel, false); /* calls done("escaped") if newlevel==0 */
}


void drown() // not tested AT ALL yet
{
  message("You fall into a pool!");
  message("You can't swim!");
  if (rund(3) < you.uluck+2) {
    /* most scrolls become unreadable */
    obj_t *obj;
    for (obj = invent ; obj ; obj = obj->nobj)
      if (obj->olet == SCROLL_SYM && rund(12) > you.uluck)
	obj->otype = SCR_BLANK_PAPER;
    /* we should perhaps merge these scrolls ?  */ // if you say so.
    message("You attempt a teleport spell.");	// utcsri!carroll

    dotele();
    if (get_cell_type(floor_info[(Short) you.ux][(Short) you.uy]) != POOL)
      return; // reflexively, you teleport and survive!

  }
  message("You drown ...");
  killer = "pool of water";
  done("drowned");
  return;
}
