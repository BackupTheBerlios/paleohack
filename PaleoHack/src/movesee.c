/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
#include "paleohackRsc.h"

// was hack.c

static Int8 seelx, seehx, seely, seehy;        /* corners of lit room */

static void movobj(obj_t *obj, Short ox, Short oy) SEC_4;

extern Boolean level_exists[MAXLEVEL+1]; // level.c

void move_visible_window(Short left_x, Short top_y, Boolean center);//display.c

/* called on movement:
	1. when throwing ball+chain far away
	2. when teleporting
	3. when walking out of a lit room
 */
void unsee()
{
  Short x,y;
  /* // this was commented out...
    if(you.udispl){
    you.udispl = false;
    newsym(you.udisx, you.udisy);
    }
  */
  if (seehx){
    seehx = 0;
  } else {
    for (x = you.ux-1; x < you.ux+2; x++)
      for (y = you.uy-1; y < you.uy+2; y++) {
	if (OUT_OF_BOUNDS(x, y)) continue;
	if (!get_cell_lit(floor_info[x][y]) &&
	    floor_symbol[x][y] == '.') {
	  floor_symbol[x][y] =' ';
	  floor_info[x][y] |= NEW_CELL;
	  on_scr(x,y);
	}
      }
  }
}

Short multi; // says whether we're in a multi-turn action.
// it's also used, with negative value, for paralysis type effects!
void nomul(Short nval) // Aborts a "multiple" move
{
  if (multi < 0) return;
  multi = nval;
  flags.mv = flags.run = 0;
}


Int8 xdir[8] = { -1,-1, 0, 1, 1, 1, 0,-1 }; // should be const, but
Int8 ydir[8] = {  0,-1,-1,-1, 0, 1, 1, 1 }; //   can't w/ segmented app!
void confdir() // was in cmd.c
{
  Int8 x = rund(8);
  you.dx = xdir[x];
  you.dy = ydir[x];
}

/* called:
	in hack.eat.c: seeoff(0) - blind after eating rotten food
	in hack.mon.c: seeoff(0) - blinded by a yellow light
	in hack.mon.c: seeoff(1) - swallowed
	in hack.do.c:  seeoff(0) - blind after drinking potion
	in hack.do.c:  seeoff(1) - go up or down the stairs
	in hack.trap.c:seeoff(1) - fall through trapdoor
 */
Char loc_symbol(Short x, Short y); // display.c
/* mode==false: don't hide @; blindness
 * mode==true:  hide @; misc movement (swallowed or in transit btwn levels) */
void seeoff(Boolean mode)
{
  Short x,y;

  if (you.udispl && mode) {
    you.udispl = false;
    floor_symbol[you.udisx][you.udisy] = loc_symbol(you.udisx,you.udisy);
  }
  if (seehx) {
    seehx = 0;
  } else
    if (!mode) {
      for (x = you.ux-1; x < you.ux+2; x++)
	for (y = you.uy-1; y < you.uy+2; y++) {
	  if (OUT_OF_BOUNDS(x, y)) continue;
	  if (!get_cell_lit(floor_info[x][y]) && floor_symbol[x][y] == '.')
	    floor_info[x][y] &= ~SEEN_CELL;
	}
    }
}


// returns true if you should call 'tick'.  false if no turn taken.
Boolean do_move() // was domove in hack.c
{
  Int8 oldx, oldy;
  monst_t *mtmp = NULL;
  //  struct rm *tmpr,*ust;
  trap_t *trap = NULL;
  obj_t *otmp;
  UChar your_tile, next_tile, your_info, next_info;

  you_wipe_engr(rnd(5));

  if (inv_weight() > 0){
    message("You collapse under your load.");
    nomul(0);
    return true; //    return false; // yes it should take a turn.
  }
  if (you.uswallow) {
    you.dx = you.dy = 0;
    if (you.ustuck) { // note: it would be a bug for ustuck to be NULL here
      you.ux = you.ustuck->mx;
      you.uy = you.ustuck->my;
    }
  } else {
    if (Confusion) {
      Int8 tries = 0;
      do {
	confdir();
	if (tries++ > 8) break;
      } while (OUT_OF_BOUNDS(you.ux+you.dx, you.uy+you.dy) ||
	     IS_ROCK(get_cell_type(floor_info[you.ux+you.dx][you.uy+you.dy])));
    }
    if (OUT_OF_BOUNDS(you.ux+you.dx, you.uy+you.dy)) {
      nomul(0);
      return false;
    }
  }

  //  ust = &levl[u.ux][u.uy]; // this line left in as a mile marker
  your_info = floor_info[you.ux][you.uy];
  next_info = floor_info[you.ux+you.dx][you.uy+you.dy];
  your_tile = get_cell_type(your_info);
  next_tile = get_cell_type(next_info);
  oldx = you.ux;
  oldy = you.uy;

  if (!you.uswallow) {
    trap = trap_at(you.ux+you.dx, you.uy+you.dy); // we'll use this later on..
    if (trap && get_trap_seen(trap->trap_info))
      nomul(0);
  }

  if (you.ustuck && !you.uswallow &&
      (you.ux+you.dx != you.ustuck->mx ||
       you.uy+you.dy != you.ustuck->my)) {
    if (dist(you.ustuck->mx, you.ustuck->my) > 2){
      /* perhaps it fled (or was teleported or ... ) */
      you.ustuck = NULL;
    } else {
      if (Blind)
	message("You cannot escape from it!");
      else {
	StrPrintF(ScratchBuffer, "You cannot escape from %s!",
		  monnam(you.ustuck));
	message(ScratchBuffer);
      }
      nomul(0);
      return false; // XXXX check whether this actually takes a turn
    }
  }

  if (you.uswallow || (mtmp = mon_at(you.ux+you.dx,you.uy+you.dy))) {
    /* attack monster */

    nomul(0);
    gethungry();
    if (multi < 0) return true;	/* we just fainted */

    /* try to attack; note that it might evade */
    if (attack(you.uswallow ? you.ustuck : mtmp))
      return true;
  }

  /* not attacking an animal, so we try to move */
  if (you.utrap) {
    if (you.utraptype == TT_PIT) {
      message("You are still in a pit.");
      you.utrap--;
    } else {
      message("You are caught in a beartrap.");
      if ((you.dx && you.dy) || !rund(5)) you.utrap--;
    }
    return true; // my butt thinks this should be true
  }

  //  tmpr = &levl[u.ux+u.dx][u.uy+u.dy]; // just another mile marker

  // Do not move through doors DIAGONALLY... (also don't move thru solid rock)
  if (IS_ROCK(next_tile)  ||
      (you.dx && you.dy && (next_tile == DOOR || your_tile == DOOR))){
    // if we are running, stop the run
    flags.move = 0;
    nomul(0);
    return false;
  }

  // Moving rocks (or failing to move rocks).
  while ((otmp = sobj_at(ENORMOUS_ROCK, you.ux+you.dx, you.uy+you.dy))) {
    Short rx = you.ux + 2 * you.dx, ry = you.uy + 2 * you.dy;
    UChar to_cell;
    trap_t *ttmp;
    nomul(0);
    to_cell = get_cell_type(floor_info[rx][ry]);
    if (!OUT_OF_BOUNDS(rx,ry) && !IS_ROCK(to_cell) &&
	(to_cell != DOOR || !(you.dx && you.dy)) &&
	!sobj_at(ENORMOUS_ROCK, rx, ry)) {
      if (mon_at(rx,ry)) {
	message("You hear a monster behind the rock.");
	message("Perhaps that's why you cannot move it.");
	goto cannot_push;
      }
      if ((ttmp = trap_at(rx,ry)))
	switch(get_trap_type(ttmp->trap_info)) {
	case PIT:
	  message("You push the rock into a pit!");
	  deltrap(ttmp);
	  delobj(otmp);
	  message("It completely fills the pit!");
	  continue;
	case TELEP_TRAP:
	  message("You push the rock and suddenly it disappears!");
	  delobj(otmp);
	  continue;
	}
      if (to_cell == POOL) {
	set_cell_type(floor_info[rx][ry], ROOM);
	mnewsym(rx,ry);
	prl(rx,ry);
	message("You push the rock into the water.");
	message("Now you can cross the water!");
	delobj(otmp);
	continue;
      }
      otmp->ox = rx;
      otmp->oy = ry;
      /* pobj(otmp); */ // this pobj was commented out when I got here.
      if (cansee(rx,ry)) print(rx,ry,otmp->olet);
      if (Invisible) newsym(you.ux+you.dx, you.uy+you.dy);

      {
	static Long lastmovetime = 0;
	/* note: this var contains garbage initially and
	   after a restore */ // Oh rapture.  I'll initialize it to 0, then?
	if (moves > lastmovetime+2 || moves < lastmovetime)
	  message("With great effort you move the enormous rock.");
	lastmovetime = moves;
      }
    } else {
      message("You try to move the enormous rock, but in vain.");
    cannot_push:
      if ((!invent || inv_weight()+90 <= 0) &&
	  (!you.dx || !you.dy ||
	   (IS_ROCK(get_cell_type(floor_info[you.ux][you.uy+you.dy])) &&
	    IS_ROCK(get_cell_type(floor_info[you.ux+you.dx][you.uy]))))) {
	message("However, you can squeeze yourself into a small opening.");
	break;
      } else
	return true;
    }
  } // end while rock!

  // Squeezing past a thing.
  if (you.dx && you.dy &&
      IS_ROCK(get_cell_type(floor_info[you.ux][you.uy+you.dy])) &&
      IS_ROCK(get_cell_type(floor_info[you.ux+you.dx][you.uy])) &&
      invent && inv_weight()+40 > 0) {
    message("You are carrying too much to get through.");
    nomul(0);
    return true;
  }
  // Dragging ball & chain
  if (Punished && DIST(you.ux+you.dx, you.uy+you.dy,
		       uchain->ox, uchain->oy) > 2) {
    if (carried(uball)) {
      movobj(uchain, you.ux, you.uy);
    } else if (DIST(you.ux+you.dx, you.uy+you.dy, uball->ox, uball->oy) < 3) {
      /* leave ball, move chain under/over ball */
      movobj(uchain, uball->ox, uball->oy);
    } else if (inv_weight() + (Short) uball->owt/2 > 0) {
      StrPrintF(ScratchBuffer, "You cannot%sdrag the heavy iron ball.",
		invent ? " carry all that and also " : " ");
      nomul(0);
      return true; // take a turn?
    } else {
      movobj(uball, uchain->ox, uchain->oy);
      unpobj(uball);/* BAH %% */
      uchain->ox = you.ux;
      uchain->oy = you.uy;
      nomul(-2);
      spin_multi("");
    }
  } // end of dragging.

  // Ok, enough fooling around.
  you.ux += you.dx;
  you.uy += you.dy;
  // YOU'VE JUST MOVED! YAY! If you're now on stairs or door, stop running.
  if (flags.run) {
    if (next_tile == DOOR ||
	(xupstair == you.ux && yupstair == you.uy) ||
	(xdnstair == you.ux && ydnstair == you.uy))
      nomul(0);
  }

  if (next_tile == POOL && !Levitation)
    drown();	/* not necessarily fatal */

  // this little section was commented out:  (and I uncommented it)
  if (you.udispl) {
    you.udispl = 0;
    newsym(oldx,oldy);
  }

  if (!Blind) {
    if (get_cell_lit(your_info)) {
      if (get_cell_lit(next_info)) {
	if (next_tile == DOOR)       prl1(you.ux+you.dx, you.uy+you.dy);
	else if (your_tile == DOOR)  nose1(oldx-you.dx, oldy-you.dy);
      } else {
	unsee();
	prl1(you.ux+you.dx, you.uy+you.dy);
      }
    } else {
      if (get_cell_lit(next_info)) setsee();
      else {
	prl1(you.ux+you.dx, you.uy+you.dy);
	if (next_tile == DOOR) {
	  if (you.dy) {
	    prl(you.ux-1, you.uy);
	    prl(you.ux+1, you.uy);
	  } else {
	    prl(you.ux, you.uy-1);
	    prl(you.ux, you.uy+1);
	  }
	}
      }
      nose1(oldx-you.dx, oldy-you.dy);
    }
  } else {
    pru();
  }
  if (!flags.nopick) pickup(true);
  if (trap) do_trap(trap);		/* fall into pit, arrow trap, etc. */
  inshop();
  if (!Blind) read_engr_at(you.ux, you.uy);
  return true;
}

static void movobj(obj_t *obj, Short ox, Short oy)
{
  /* Some dirty programming to get display right */
  unlink_obj(obj);
  unpobj(obj);
  obj->nobj = fobj;
  fobj = obj;
  obj->ox = ox;
  obj->oy = oy;
}

Boolean do_pickup()
{
  if (!gold_at(you.ux, you.uy) && !obj_at(you.ux, you.uy)) {
    message("There is nothing here to pick up.");
    return false;
  }
  if (Levitation) {
    message("You cannot reach the floor.");
    return true;
  }
  pickup(false);
  return true;
}


void pickup(Boolean all)
{
  struct gold *gold;
  struct obj *obj, *obj2, *otmp;
  Short wt, ctr;

  if (Levitation) return;
  while ((gold = gold_at(you.ux,you.uy))) {
    StrPrintF(ScratchBuffer, "%ld gold piece%s",
	      gold->amount, (gold->amount == 1) ? "." : "s.");
    message(ScratchBuffer);
    you.ugold += gold->amount;
    flags.botl |= BOTL_GOLD;
    freegold(gold);
    if (flags.run) nomul(0);
    if (Invisible) newsym(you.ux,you.uy);
  }

  // check for more than one object
  if (!all) {
    for (obj = fobj, ctr = 0 ; obj ; obj = obj->nobj)
      if (obj->ox == you.ux && obj->oy == you.uy && (!Punished || obj!=uchain))
	ctr++;
    if (ctr < 2) all = true;
    else         message("There are several objects here.");
  }
  if (flags.askpick) all = false;

  for (obj = fobj; obj; obj = obj2) {
    obj2 = obj->nobj;	// get next obj now; perhaps obj will be picked up
    if (obj->ox == you.ux && obj->oy == you.uy) {
      if (flags.run) nomul(0);

      if (Punished && obj == uchain) continue;

      if (!all) {
	Short tmp = FrmCustomAlert(PickUpThisP, "Pick up", doname(obj), NULL);
	if      (tmp == PICKUP_QUIT) return;
	else if (tmp == PICKUP_NO)   continue;
	else if (tmp == PICKUP_ALL)  all = true;
      }

      if (obj->otype == DEAD_COCKATRICE && !uarmg) {
	message("Touching the dead cockatrice is a fatal mistake.");
	message("You turn to stone.");
	killer = "cockatrice cadaver";
	done("died");
	return;
      }

      if (obj->otype == SCR_SCARE_MONSTER) {
	// You're allowed to [try to!!] pick it up exactly ONCE.
	if (!obj->spe) obj->spe = 1;
	else {
	  message("The scroll turns to dust as you pick it up.");
	  delobj(obj);
	  continue;
	}
      }

      wt = inv_weight() + obj->owt;
      if (wt > 0) {
	// Too heavy.  See if we can lift "some" of it.
	Short qq = 0;
	if (obj->quantity > 1) {
	  Short savequan = obj->quantity;
	  Short iw = inv_weight();
	  for (qq = 1; qq < savequan; qq++){
	    obj->quantity = qq;
	    if (iw + weight(obj) > 0)
	      break;
	  }
	  obj->quantity = savequan;
	  qq--;
	  // ok, we can carry qq of them
	  if (qq > 0) {
	    StrPrintF(ScratchBuffer, "You can only carry %s of the %s lying here.",
		      (qq == 1) ? "one" : "some",
		      doname(obj));
	    message(ScratchBuffer);
	    splitobj(obj, qq); // creates a new object at obj->nobj.
	    // the 'for' loop captured the old value of obj->nobj already,
	    // so we _won't_ ask whether to pick up the remainder.
	    // if desired, splitobj returns a pointer to it; we could say:
	    //	obj2 = splitobj(obj,qq);
	  }
	}
	if (qq <= 0) {
	  StrPrintF(ScratchBuffer, "There %s %s here, but %s.",
		    (obj->quantity == 1) ? "is" : "are",
		    doname(obj),
		    (!invent ? "it is too heavy for you to lift"
		     : "you cannot carry any more"));
	  message(ScratchBuffer);
	  return; // was 'break;'... but there's nothing after the loop anyway.
	}
      }

      for (otmp = invent, ctr = 0 ; otmp != NULL ; otmp = otmp->nobj)
	ctr++;
      if (ctr >= 52) {
	message("Your knapsack cannot accomodate any more items.");
	break;
      }

      if (wt > -5) message("You have a little trouble lifting");
      unlink_obj(obj); //freeobj(obj);
      if (Invisible) newsym(you.ux,you.uy);
      // sets obj->unpaid if necessary:
      addtobill(obj);
      {
	Short pickquan = obj->quantity;
	Short mergquan;
	if (!Blind) //also done by prinv(), but addinv() needs it for merging:
	  obj->bitflags |= O_IS_DESCKNOWN; // obj->dknown = 1;
	obj = addinv(obj);         // might merge it with other objects
	mergquan = obj->quantity;
	obj->quantity = pickquan;  // to fool prinv()
	prinv(obj);
	obj->quantity = mergquan;
      }
    } // end if ux uy
  } // end for all objects
}



/* stop running if we see something interesting */
/* turn around a corner if that is the only way we can proceed */
/* do not turn left or right twice */
void lookaround()
{
  Short x,y,i,x0=0,y0=0,m0=0,i0 = 9;
  Short corrct = 0;
  monst_t *mtmp;
  Boolean do_corr, no_turn = false;

  if (Blind || flags.run == 0) return;

  if ((flags.run == 1) && (get_cell_type(floor_info[you.ux][you.uy]) == ROOM))
    return;

  // iterate through the 9 (8) squares adjacent to you...
  for (x = you.ux - 1 ; x <= you.ux + 1 ; x++) {
    for (y = you.uy - 1 ; y <= you.uy + 1 ; y++) {
      if (x == you.ux && y == you.uy) continue;
      if (!get_cell_type(floor_info[x][y])) continue;

      // If there's a visible monster adjacent to us,
      // and it's not tame (or we're about to step on our dog), then stop.
      mtmp = mon_at(x,y);
      if (mtmp && !(mtmp->bitflags & M_IS_MIMIC) &&
	  (!(mtmp->bitflags & M_IS_INVISIBLE) || See_invisible)) {
	if (!(mtmp->bitflags & M_IS_TAME) ||
	    (x == you.ux + you.dx && y == you.uy + you.dy)) {
	  nomul(0); // stop running
	  return;
	}
      } else mtmp = 0; /* invisible M cannot influence us */

      // We're not interested in the square directly behind us..
      if ((x == you.ux - you.dx) && (y == you.uy - you.dy)) continue;

      do_corr = false;
      switch (floor_symbol[x][y]) {
      case VWALL_SYM: // '|'
      case HWALL_SYM: // '-'  but really emdash
      case ROOM_SYM:  // '.'
      case SCORR_SYM: // ' '
	break;
      case DOOR_SYM:  // '+'
	if (x != you.ux && y != you.uy) break; // diagonal doors are boring
	if (flags.run != 1) {
	  nomul(0); // non-diagonal door.  stop running.
	  return;
	}
	do_corr = true; // goto corr;
	break;
      case CORR_SYM:
	do_corr = true;
	break;
      case TRAP_SYM: // '^'
	if (flags.run == 1)
	  do_corr = true;
	else if (x == you.ux+you.dx && y == you.uy+you.dy) {
	  nomul(0); // trap in your path!  stop running!
	  return;
	}
	break;
      default:	/* e.g. objects or trap or stairs */
	if (flags.run == 1)
	  do_corr = true;
	else if (!mtmp) {		/* d */
	  nomul(0); // non-dog thingy.  stop running.
	  return;
	}
	break;
      }
      // This if statement was the 'corr:' block.
      if (do_corr) {
	if (flags.run == 1 || flags.run == 3) {
	  i = DIST(x, y, you.ux+you.dx, you.uy+you.dy);
	  if (i > 2) continue; // go on to the next x,y in 'for y'...
	  if (corrct == 1 && DIST(x, y, x0, y0) != 1)
	    no_turn = true;
	  if (i < i0) {
	    i0 = i;
	    x0 = x;
	    y0 = y;
	    m0 = mtmp ? 1 : 0;
	  }
	}
	corrct++;
      } // end of 'corr:' block.
    } // end for y
  } // end for x

  if (corrct > 1 && flags.run == 2) {
    nomul(0); // It's an intersection; stop running.
    return;
  }

  // Here we might decide to change direction...
  if ((flags.run == 1 || flags.run == 3) && !no_turn && !m0 && i0 &&
      (corrct == 1 || (corrct == 2 && i0 == 1))) {
    /* make sure that we do not turn too far */
    if (i0 == 2) {
      if (you.dx == y0-you.uy && you.dy == you.ux-x0)
	i = 2;		/* straight turn right */
      else
	i = -2;		/* straight turn left */
    } else if (you.dx && you.dy) {
      if ((you.dx == you.dy && y0 == you.uy) ||
	  (you.dx != you.dy && y0 != you.uy))
	i = -1;		/* half turn left */
      else
	i = 1;		/* half turn right */
    } else {
      if ((x0-you.ux == y0-you.uy && !you.dy) ||
	  (x0-you.ux != y0-you.uy && you.dy))
	i = 1;		/* half turn right */
      else
	i = -1;		/* half turn left */
    }
    i += you.last_str_turn;
    if (i <= 2 && i >= -2) {
      you.last_str_turn = i;
      you.dx = x0-you.ux;
      you.dy = y0-you.uy;
    }
  }

  // all done!
}






/* something like lookaround, but we are not running */
/* react only to monsters that might hit us */
Boolean monster_nearby()
{
  Short x,y;
  monst_t *mtmp;
  if (Blind) return false;
  for (x = you.ux-1 ; x <= you.ux+1 ; x++) {
    for (y = you.uy-1 ; y <= you.uy+1 ; y++) {
      if (x == you.ux && y == you.uy) continue;
      mtmp = mon_at(x,y);
      if (mtmp && !(mtmp->bitflags & (M_IS_MIMIC | M_IS_PEACEFUL | M_IS_TAME |
				      M_IS_FROZEN | M_IS_ASLEEP)) &&
	  (mtmp->data->mlet != 'E' && mtmp->data->mlet != 'a') &&
	  (See_invisible || !(mtmp->bitflags & M_IS_INVISIBLE) ))
	return true; // an apparent monster of threatening visage!
    }
  }
  return false;
}

Boolean cansee(Short x, Short y)
{
  if (Blind || you.uswallow) return false;
  if (dist(x,y) < 3) return true;
  if (get_cell_lit(floor_info[x][y]) && 
      seelx <= x && x <= seehx && seely <= y && y <= seehy)
     return true;
  return false;
}



void setsee()
{
  Short x,y;

  if (Blind) {
    pru();
    return;
  }
  if (!get_cell_lit(floor_info[you.ux][you.uy])) {
    seelx = you.ux-1;
    seehx = you.ux+1;
    seely = you.uy-1;
    seehy = you.uy+1;
  } else {
    for (seelx = you.ux; get_cell_lit(floor_info[seelx-1][you.uy]); seelx--)  ;
    for (seehx = you.ux; get_cell_lit(floor_info[seehx+1][you.uy]); seehx++)  ;
    for (seely = you.uy; get_cell_lit(floor_info[you.ux][seely-1]); seely--)  ;
    for (seehy = you.uy; get_cell_lit(floor_info[you.ux][seehy+1]); seehy++)  ;
  }
  for (y = seely; y <= seehy; y++)
    for (x = seelx; x <= seehx; x++) {
      prl(x,y);
    }
       
  if (!get_cell_lit(floor_info[you.ux][you.uy]))
    seehx = 0; /* seems necessary elsewhere */
  else {
    if (seely==you.uy)  for (x = you.ux-1; x <= you.ux+1; x++)  prl(x,seely-1);
    if (seehy==you.uy)  for (x = you.ux-1; x <= you.ux+1; x++)  prl(x,seehy+1);
    if (seelx==you.ux)  for (y = you.uy-1; y <= you.uy+1; y++)  prl(seelx-1,y);
    if (seehx==you.ux)  for (y = you.uy-1; y <= you.uy+1; y++)  prl(seehx+1,y);
  }
}

// was in read.c
void nosee(Short x, Short y); // in display.c
void litroom(Boolean on)
{
  Short num,zx,zy;
  UChar floor_type = get_cell_type(floor_info[you.ux][you.uy]);
  Boolean lit = get_cell_lit(floor_info[you.ux][you.uy]);
  /* first produce the text (provided he is not blind) */
  if (!Blind) {
    if (!on) {
      if (you.uswallow || !xdnstair || floor_type == CORR || !lit) {
	message("It seems even darker in here than before.");
	return;
      } else
	message("It suddenly becomes dark in here.");
    } else {
      if (you.uswallow) {
	if (you.ustuck) {
	  StrPrintF(ScratchBuffer, "%s's stomach is lit.", Monnam(you.ustuck));
	  message(ScratchBuffer);
	}
	return;
      }
      if (!xdnstair) {
	message("Nothing Happens.");
	return;
      }
      if (floor_type == CORR) {
	message("The corridor lights up around you, then fades.");
	return;
      } else if (lit) {
	message("The light here seems better now.");
	return;
      } else
	message("The room is lit.");
    }
  }

  if (lit == on)
    return;
  if (floor_type == DOOR) {
    if (IS_ROOM(get_cell_type(floor_info[you.ux][you.uy+1]))) zy = you.uy+1;
    else if (IS_ROOM(get_cell_type(floor_info[you.ux][you.uy-1])))
      zy = you.uy-1;
    else zy = you.uy;
    if (IS_ROOM(get_cell_type(floor_info[you.ux+1][you.uy]))) zx = you.ux+1;
    else if (IS_ROOM(get_cell_type(floor_info[you.ux-1][you.uy])))
      zx = you.ux-1;
    else zx = you.ux;
  } else {
    zx = you.ux;
    zy = you.uy;
  }
  for (seelx = you.ux;
       (num = get_cell_type(floor_info[seelx-1][zy])) != CORR && num != 0;
       seelx--) ;
  for (seehx = you.ux;
       (num = get_cell_type(floor_info[seehx+1][zy])) != CORR && num != 0;
       seehx++) ;
  for (seely = you.uy;
       (num = get_cell_type(floor_info[zx][seely-1])) != CORR && num != 0;
       seely--) ;
  for (seehy = you.uy;
       (num = get_cell_type(floor_info[zx][seehy+1])) != CORR && num != 0;
       seehy++) ;
  for (zy = seely; zy <= seehy; zy++)
    for (zx = seelx; zx <= seehx; zx++) {
      floor_info[zx][zy] |= LIT_CELL;
      if (!Blind && dist(zx,zy) > 2) {
	if (on) prl(zx,zy);
	else    nosee(zx,zy);
      }
    }
  if (!on) seehx = 0;
}





Short abon()
{
  if (you.ustr == 3) return(-3);
  else if (you.ustr < 6) return(-2);
  else if (you.ustr < 8) return(-1);
  else if (you.ustr < 17) return(0);
  else if (you.ustr < 69) return(1);	/* up to 18/50 */
  else if (you.ustr < 118) return(2);
  else return(3);
}

Short dbon()
{
  if (you.ustr < 6) return(-1);
  else if (you.ustr < 16) return(0);
  else if (you.ustr < 18) return(1);
  else if (you.ustr == 18) return(2);	/* up to 18 */
  else if (you.ustr < 94) return(3);	/* up to 18/75 */
  else if (you.ustr < 109) return(4);	/* up to 18/90 */
  else if (you.ustr < 118) return(5);	/* up to 18/99 */
  else return(6);
}

/* may kill you; cause may be poison or monster like 'A' */
void losestr(Short num)
{
  you.ustr -= num;
  while (you.ustr < 3) {
    you.ustr++;
    you.uhp -= 6;
    you.uhpmax -= 6;
  }
  flags.botl |= (BOTL_STR | BOTL_HP);
  do_feep(400, 9);
}


void losehp(Short n, Char *knam)
{
  you.uhp -= n;
  if (you.uhp > you.uhpmax)
    you.uhpmax = you.uhp;	/* perhaps n was negative */
  flags.botl |= BOTL_HP;
  if (you.uhp < 1) {
    // (knam is either a pointer into mon_names or a const text string.)
    killer = knam;	/* the thing that killed you */
    done("died");
    return;
  }
}

void losehp_m(Short n, struct monst *mtmp)
{
  you.uhp -= n;
  flags.botl |= BOTL_HP;
  if (you.uhp < 1)
    done_in_by(mtmp);
}

Long newuexp()
{
  return(10*(1L << (you.ulevel-1)));
}

/* hit by V or W */
void losexp()
{
  Short num;

  if (you.ulevel > 1) {
    StrPrintF(ScratchBuffer, "Goodbye level %u.", you.ulevel);
    you.ulevel--;
    message(ScratchBuffer);
  } else
    you.uhp = -1; // you're dead but you haven't noticed yet?
  num = rnd(10);
  you.uhp -= num;
  you.uhpmax -= num;
  you.uexp = newuexp();
  flags.botl |= (BOTL_EXP | BOTL_HP);
}

// this was in do.c
Boolean do_down()
{
  if (you.ux != xdnstair || you.uy != ydnstair) {
    message("You can't go down here.");
    return false;
  }
  if (you.ustuck) {
    message("You are being held, and cannot go down.");
    return true;
  }
  if (Levitation) {
    message("You're floating high above the stairs.");
    return false;
  }

  goto_level(dlevel+1, true);
  return true;
}

// this was in do.c
Boolean do_up()
{
  if (you.ux != xupstair || you.uy != yupstair) {
    message("You can't go up here.");
    return false;
  }
  if (you.ustuck) {
    message("You are being held, and cannot go up.");
    return true;
  }
  if (!Levitation && inv_weight() + 5 > 0) {
    message("Your load is too heavy to climb the stairs.");
    return true;
  }

  goto_level(dlevel-1, true);
  return true;
}

// this was in main.c   // unused now
/*
static void glo(Short foo)
{
  //  / * construct the string  xlock.n  * /
  //  Char *tf;
  //
  //  tf = lock;
  //  while (*tf && *tf != '.') tf++;
  //  (void) sprintf(tf, ".%d", foo);
}
*/

// this was in do.c
void clear_visible(); // display.c
void goto_level(Short newlevel, Boolean at_stairs)
{
  //  Short fd;
  Boolean up = (newlevel < dlevel);

  if (newlevel <= 0) {
    done("escaped");    /* in fact < 0 is impossible */ // if you say so.
    return;
  }
  if (newlevel > MAXLEVEL) newlevel = MAXLEVEL;	/* strange ... */
  if (newlevel == dlevel) return;	      /* this can happen */

  //  glo(dlevel);
  //  fd = creat(lock, FMASK);
  if (false /*fd < 0*/) { // I don't have fd.  Use this if SaveDB unwriteable?
    /*
     * This is not quite impossible: e.g., we may have
     * exceeded our quota. If that is the case then we
     * cannot leave this level, and cannot save either.
     * Another possibility is that the directory was not
     * writable.
     */
    StrPrintF(ScratchBuffer, "A mysterious force prevents you from going %s.",
	      up ? "up" : "down");
    message(ScratchBuffer);
    return;
  }

  if (Punished) unplacebc();
  you.utrap = 0;				/* needed in level_tele */
  you.ustuck = NULL;				/* idem */
  keepdogs();
  seeoff(true);
  if (you.uswallow) {				/* idem */
    you.uswallow = false;
    you.uswallowedtime = 0;
  }
  //   flags.nscrinh = 1; // XXXX see display.c
  you.ux = FAR;				/* hack */
  inshop();			/* probably was a trapdoor */

  savelev(dlevel, true);

  dlevel = newlevel;
  if (maxdlevel < dlevel)
    maxdlevel = dlevel;
  //  glo(dlevel);

  if (!level_exists[dlevel]) // Check whether level exists.
    mklev();
  else if (!getlev(dlevel, true)) // If it does, try to open it. Maybe succeed.
    mklev();

  if (at_stairs) {
    if (up) {
      you.ux = xdnstair;
      you.uy = ydnstair;
      if (!you.ux) {		/* entering a maze from below? */
	you.ux = xupstair;	/* this will confuse the player! */
	you.uy = yupstair;
      }
      //      if (Punished && !Levitation) {
      //	message("With great effort you climb the stairs.");
      //	placebc(true);
      //      }
      // I heard that the above will cause crash if Punished && Levitation.
      if (Punished) {
	if (!Levitation)
	  message("With great effort you climb the stairs.");
	placebc(true);
      }
    } else {
      you.ux = xupstair;
      you.uy = yupstair;
      if (inv_weight() + 5 > 0 || Punished) {
	message("You fall down the stairs.");	/* %% */
	losehp(rnd(3), "fall");
	if (Punished) {
	  if (uwep != uball && rund(3)){
	    message("... and are hit by the iron ball.");
	    losehp(rnd(20), "iron ball");
	  }
	  placebc(true);
	}
	selftouch("Falling, you");
      }
    }
    {
      monst_t *mtmp = mon_at(you.ux, you.uy);
      if (mtmp)
	mnexto(mtmp);
    }
  } else {	/* trapdoor or level_tele */
    do {
      you.ux = rnd(DCOLS-1);
      you.uy = rund(DROWS);
    } while(get_cell_type(floor_info[you.ux][you.uy]) != ROOM ||
	    mon_at(you.ux,you.uy));
    if (Punished) {
      if (uwep != uball && !up /* %% */ && rund(5)) {
	message("The iron ball falls on your head.");
	losehp(rnd(25), "iron ball");
      }
      placebc(true);
    }
    selftouch("Falling, you");
  }
  inshop();
  init_track();

  losedogs();
  {
    monst_t *mtmp;
    if ((mtmp = mon_at(you.ux, you.uy))) mnexto(mtmp);	/* riv05!a3 */
  }
  //  flags.nscrinh = 0; // XXX see display.c
  setsee();
  seeobjs();	/* make old cadavers disappear - riv05!a3 */
  move_visible_window(you.ux, you.uy, true);
  refresh(); //  docrt();
  flags.botl = BOTL_ALL;
  print_stats(0);
  pickup(true);
  read_engr_at(you.ux, you.uy);
}

// this was in do.c
void more_experienced(Short exp, Short rexp)
{
  you.uexp += exp;
  you.urexp += 4 * exp + rexp;
  if (exp) flags.botl |= BOTL_EXP;
  if (you.urexp >= ((you.character_class == CLASS_WIZARD) ? 1000 : 2000))
    flags.beginner = false;
}

// this was in potion.c
void pluslvl()
{
  Short num;

  message("You feel more experienced.");
  num = rnd(10);
  you.uhpmax += num;
  you.uhp += num;
  if (you.ulevel < 14) {
    you.uexp = newuexp()+1;
    you.ulevel++;
    StrPrintF(ScratchBuffer, "Welcome to experience level %u.", you.ulevel);
    message(ScratchBuffer);
  }
  flags.botl |= BOTL_EXP | BOTL_HP;
}



// NOTE: This usually returns a NEGATIVE number representing "how much
// more you can safely carry".
Short inv_weight() // surely this should be in invent.c?
{
  obj_t *otmp = invent;
  Short wt = (you.ugold + 500)/1000;
  Short carrcap;
  if (Levitation)			/* pugh@cornell */
    carrcap = MAX_CARR_CAP;
  else {
    carrcap = 5*(((you.ustr > 18) ? 20 : you.ustr) + you.ulevel);
    if (carrcap > MAX_CARR_CAP) carrcap = MAX_CARR_CAP;
    if (Wounded_legs & LEFT_SIDE) carrcap -= 10;
    if (Wounded_legs & RIGHT_SIDE) carrcap -= 10;
  }
  for (otmp = invent ; otmp ; otmp = otmp->nobj)
    wt += otmp->owt;
  return (wt - carrcap);
}

// was in do.c
void set_wounded_legs(Long side, Short timex)
{
  if (!Wounded_legs || (Wounded_legs & TIMEOUT))
    Wounded_legs |= side + timex;
  else
    Wounded_legs |= side;
}
// was in do.c
void heal_legs()
{
  if (Wounded_legs) {
    if ((Wounded_legs & BOTH_SIDES) == BOTH_SIDES)
      message("Your legs feel somewhat better.");
    else
      message("Your leg feels somewhat better.");
    Wounded_legs = 0;
  }
}

