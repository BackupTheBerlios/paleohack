/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
#include "paleohackRsc.h"

extern previous_state curr_state;
static obj_t *current_ice_box; // used by <VERB>_ice_box().

//static Boolean put_in_ice_box(obj_t *obj) SEC_2; // was "in_ice_box"
static Boolean use_ice_box(obj_t *obj); // keep in default section to be safe
static monst_t * bchit(Short ddx, Short ddy, Short range, Char sym) SEC_2;
static void use_whistle(obj_t *obj) SEC_2;
static void use_magic_whistle(obj_t *obj) SEC_2;
static Boolean dig(Boolean cont) SEC_2;
static Boolean begin_pick_axe(obj_t *obj) SEC_2;
static void do_dig(Int8 x, Int8 y, Boolean down, UChar level) SEC_2;



void end_turn_start_turn(); // in main.c

Boolean do_apply(obj_t *obj)
{
  Boolean result = true;

  if (!obj) return false;

  switch(obj->otype) {
  case EXPENSIVE_CAMERA:
    curr_state.cmd = 'C'; // c for camera
    curr_state.item = obj;
    curr_state.mode = MODE_DIRECTIONAL;
    draw_directional();
    return false; // do not 'tick'
  case ICE_BOX:
    result = use_ice_box(obj);
    break;
  case PICK_AXE:
    result = begin_pick_axe(obj);
    if (result)
      end_turn_start_turn(); // you might spend time wielding it first.
    curr_state.cmd = 'P'; // p for pick-axe
    curr_state.item = obj;
    curr_state.mode = MODE_DIRECTIONAL;
    draw_directional();
    return false;
  case MAGIC_WHISTLE:
    if (you.character_class == CLASS_WIZARD || you.ulevel > 9) {
      use_magic_whistle(obj);
      break;
    }
    /* fall into next case */
  case WHISTLE:
    use_whistle(obj);
    break;

  case CAN_OPENER:
    if (!carrying(TIN))
      message("You have no can to open.");
    else {
      message("You cannot open a tin without eating its contents.");
      message("In order to open and eat, use the 'eat' command.");
      if (obj != uwep)
	message("Opening tins will be much easier if you wield the can-opener.");
    }
    result = false;
    show_messages();
    break;
  default:
    message("Sorry, I don't know how to use that.");
    result = false;
    break;
  }
  nomul(0);
  return result;
}



// Call this AFTER you come back from the directional dude....
extern Short multi; // living in movesee.c right now..
void use_camera(obj_t *obj)
{
  monst_t *mtmp;
  if (!you.dx && !you.dy && !you.dz) return; // Caller should have checked!

  if (you.uswallow) {
    StrPrintF(ScratchBuffer, "You take a picture of %s's stomach.", 
	      (you.ustuck) ? monnam(you.ustuck) : "???");
    message(ScratchBuffer);
    return;
  }

  if (you.dz) {
    StrPrintF(ScratchBuffer, "You take a picture of the %s.",
	      (you.dz > 0) ? "floor" : "ceiling");
    message(ScratchBuffer);
    return;
  }

  mtmp = bchit(you.dx, you.dy, DCOLS, '!');
  if (!mtmp) return; // no monster, no effect.
  //  message("found one");

  // You can use a camera to blind a monster (except a 'yellow light')
  if (mtmp->bitflags & M_IS_ASLEEP) {
    //    message("asleep");
    mtmp->bitflags &= ~M_IS_ASLEEP;
    StrPrintF(ScratchBuffer, "The flash awakens %s.", monnam(mtmp)); /* a3 */
    message(ScratchBuffer);
  } else if (mtmp->data->mlet != 'y') {
    if ((mtmp->mcansee_and_blinded & M_CAN_SEE) ||    // can see,
	(mtmp->mcansee_and_blinded & ~M_CAN_SEE)) {   // or, blinded.
      Short tmp = dist(mtmp->mx, mtmp->my);
      Short tmp2;
      //      message("can see or blinded");
      if (cansee(mtmp->mx, mtmp->my)) {
	StrPrintF(ScratchBuffer,"%s is blinded by the flash!", Monnam(mtmp));
	message(ScratchBuffer);
      }
      setmangry(mtmp);
      if (tmp < 9 && !(mtmp->bitflags & M_IS_SHOPKEEPER) && rund(4)) {
	mtmp->mflee_and_time |= M_FLEEING; // set "mflee"...
	if (rund(4)) mtmp->mflee_and_time |= rnd(100); // ...set "time"
      }
      if (tmp < 3) mtmp->mcansee_and_blinded = 0; // turn off See AND Blinded
      else {
	tmp2 = mtmp->mcansee_and_blinded & ~M_CAN_SEE;
	tmp2 += rnd(1 + 50/tmp);
	if (tmp2 > 127) tmp2 = 127;
	mtmp->mcansee_and_blinded = tmp2;
	mtmp->mcansee_and_blinded &= ~M_CAN_SEE; // make sure this bit's off
      }
    }
    /*
    else {
      StrPrintF(ScratchBuffer, "%d %s can't see!",
		mtmp->mcansee_and_blinded,  monnam(mtmp));
      message(ScratchBuffer);
    }
    */
  }
  /*
  else {
    StrPrintF(ScratchBuffer, "%d %s is 'y'",
	      mtmp->mcansee_and_blinded,  monnam(mtmp));
    message(ScratchBuffer);
  }
  */
}





Boolean put_in_ice_box(obj_t *obj)
{
  if (!obj)
    return false;
  if (obj == current_ice_box ||
      (Punished && (obj == uball || obj == uchain))) {
    message("You must be kidding.");
    return false;
  }
  if (obj->owornmask & (W_ARMOR | W_RING)) {
    message("You cannot refrigerate something you are wearing.");
    return false;
  }
  if (obj->owt + current_ice_box->owt > 70) {
    message("It won't fit.");
    return true;	/* be careful! */
  }
  if (obj == uwep) {
    if (uwep->bitflags & O_IS_CURSED) {
      message("Your weapon is welded to your hand!");
      return false;
    }
    setuwep(NULL);
  }
  current_ice_box->owt += obj->owt;
  obj->o_container_id = current_ice_box->o_id;
  obj->age = moves - obj->age;	/* actual age */
  // move obj to the things-that-are-contained list...
  unlink_inv(obj);
  obj->nobj = fcobj;
  fcobj = obj;
  return true;
}

// Return true if obj is in current_ice_box.
Boolean ck_ice_box(obj_t *obj) // Keep this in the default section! Func ptrs!
{
  return (obj->o_container_id == current_ice_box->o_id);
}

Short out_ice_box(obj_t *obj) // Keep this in the default section! Func ptrs!
{
  obj_t *otmp;
  // unlink obj from fcobj...
  if (obj == fcobj) fcobj = fcobj->nobj;
  else {
    for (otmp = fcobj; otmp->nobj != obj; otmp = otmp->nobj)
      if (!otmp->nobj) {
	message("out_ice_box: it's not in there!");
	return 0;
      }
    otmp->nobj = obj->nobj;
  }
  current_ice_box->owt -= obj->owt;
  obj->age = moves - obj->age;	/* simulated point of time */
  addinv(obj);
  return 1; // xxx this was 0.  but.. makes no difference?
}


// I am using "askchain" and function pointers but it might be oogy.
static Boolean use_ice_box(obj_t *obj) // keep in default section to be safe
{
  Short cnt = 0;
  obj_t *otmp;
  current_ice_box = obj;	/* for use by in/out_ice_box */
  for (otmp = fcobj; otmp; otmp = otmp->nobj)
    if (otmp->o_container_id == obj->o_id)
      cnt++;
  if (cnt) {
    if (FrmCustomAlert(IceBoxP, "take", "out of", NULL) == 0) // 0==Yes 1==No
      if (askchain(fcobj, NULL, "Take out", false, out_ice_box, ck_ice_box, 0))
	return true;
    if (FrmCustomAlert(IceBoxP, "put", "in", NULL) != 0) // 0==Yes 1==No
      return false;
  } else {
    //    message("Your ice-box is empty.");
    //    show_messages();
  }
  /* call getobj: 0: allow cnt; #: allow all types; %: expect food */
  /*
  otmp = getobj("0#%", "put in");
  if (!otmp || !put_in_ice_box(otmp))
    flags.move = multi = 0;
  */
  if (getobj_init("0#%", "put (in box)", ACT_REFRIGERATE))
    FrmPopupForm(InvActionForm);
  return false;
}


// Looks for a monster to be Hit by a Beam from a Camera..
// also, show an animation of the beam of light.
static monst_t * bchit(Short ddx, Short ddy, Short range, Char sym)
{
  monst_t *mtmp = (monst_t *) NULL;
  Short x = you.ux, y = you.uy;

  if (sym) Tmp_at_init(sym);
  while (range--) {
    x += ddx; y += ddy;
    mtmp = mon_at(x,y);
    if (mtmp) break; // Found a monster!
    if (!ZAP_POS(get_cell_type(floor_info[x][y]))) { // Hit a wall or something
      x -= ddx; y -= ddy;
      break;
    }
    if (sym) Tmp_at(x, y); // animation
  }
  if (sym) Tmp_at_cleanup();
  return mtmp;
}


/* ARGSUSED */
void set_whistletime(monst_t *mtmp, Long t); // in dog.c
static void use_whistle(obj_t *obj)
{
  monst_t *mtmp = fmon;
  message("You produce a high whistling sound.");
  while (mtmp) {
    if (dist(mtmp->mx, mtmp->my) < you.ulevel*20) {
      if (mtmp->bitflags & M_IS_ASLEEP)
	mtmp->bitflags &= ~M_IS_ASLEEP;
      if (mtmp->bitflags & M_IS_TAME)
	set_whistletime(mtmp, moves);
    }
    mtmp = mtmp->nmon;
  }
}

/* ARGSUSED */
static void use_magic_whistle(obj_t *obj)
{
  monst_t *mtmp = fmon;
  message("You produce a strange whistling sound.");
  while (mtmp) {
    if (mtmp->bitflags & M_IS_TAME) mnexto(mtmp);
    mtmp = mtmp->nmon;
  }
}


// dig goes here.  and holetime.
typedef struct digging_s {
  coord pos;
  Short effort;	/* effort expended on current pos */
  Boolean down;
  Boolean is_in_use;
  UChar level;
} digging_state;
digging_state digs;
/* When will hole be finished? Very rough indication used by shopkeeper. */
Short holetime() {
  return( (digs.is_in_use) ? (250 - digs.effort)/20 : -1);
}
static Boolean dig(Boolean cont) // return false if we should stop
{
  //  struct rm *lev;
  Short dpx = digs.pos.x, dpy = digs.pos.y;

  /* perhaps a nymph stole his pick-axe while he was busy digging */
  /* or perhaps he teleported away */
  if (you.uswallow || !uwep || uwep->otype != PICK_AXE ||
      digs.level != dlevel ||
      ((digs.down && (dpx != you.ux || dpy != you.uy)) ||
       (!digs.down && dist(dpx,dpy) > 2)))
    return false;

  digs.effort += 10 + abon() + uwep->spe + rund(5);
  if (digs.down) {
    if (!xdnstair) {
      message("The floor here seems too hard to dig in.");
      return false;
    }
    if (digs.effort > 250) {
      dighole();
      return false;	/* done with digging */
    }
    if (digs.effort > 50) {
      trap_t *ttmp = trap_at(dpx,dpy);

      if (!ttmp) {
	ttmp = maketrap(dpx, dpy, PIT);
	ttmp->trap_info |= SEEN_TRAP;
	message("You have dug a pit.");
	you.utrap = rund(4) + 2;
	you.utraptype = TT_PIT;
	return false;
      }
    }
  } else
    if (digs.effort > 100) {
      Char *digtxt;
      obj_t *obj;
      UChar tile_type;

      tile_type = get_cell_type(floor_info[dpx][dpy]);//lev = &levl[dpx][dpy];
      if ((obj = sobj_at(ENORMOUS_ROCK, dpx, dpy))) {
	fracture_rock(obj);
	digtxt = "The rock falls apart.";
      } else if (!tile_type || tile_type == SCORR) {
	set_cell_type(floor_info[dpx][dpy], CORR);
	digtxt = "You succeeded in cutting away some rock.";
      } else if (tile_type == HWALL || tile_type == VWALL
		 || tile_type == SDOOR) {
	set_cell_type(floor_info[dpx][dpy], (xdnstair ? DOOR : ROOM) );
	digtxt = "You just made an opening in the wall.";
      } else
	digtxt = "Now what exactly was it that you were digging in?";
      mnewsym(dpx, dpy);
      prl(dpx, dpy);
      message(digtxt);		/* after mnewsym & prl */
      return false;
    } else {
      if (IS_WALL(get_cell_type(floor_info[dpx][dpy]))) {
	Short rno = inroom(dpx,dpy);
	extern room_t rooms[MAX_ROOMS]; // in make_level.c .... SIGH...
	if (rno >= 0 && rooms[rno].rtype >= 8) {
	  message("This wall seems too hard to dig into.");
	  return false;
	}
      }
      if (!cont)
	message("You hit the rock with all your might.");
    }
  return true;
}


void dighole()
{
  trap_t *ttmp = trap_at(you.ux, you.uy);

  if (!xdnstair) {
    message("The floor here seems too hard to dig in.");
  } else {
    if (ttmp)
      set_trap_type(ttmp->trap_info, TRAPDOOR);
    else
      ttmp = maketrap(you.ux, you.uy, TRAPDOOR);
    ttmp->trap_info |= SEEN_TRAP;
    message("You've made a hole in the floor.");
    if (!you.ustuck) {
      if (inshop())
      	shopdig(true);
      message("You fall through ...");
      if (you.utraptype == TT_PIT) {
	you.utrap = 0;
	you.utraptype = false;
      }
      goto_level(dlevel+1, false);
    }
  }
}




// use_pick_axe goes here.  it will require a directional form.
Char sdir[] = "hykulnjb><";
Boolean stop_occupation_now;
static Boolean begin_pick_axe(obj_t *obj)
{
  //  Char dirsyms[12];
  //  Char *dsp = dirsyms, *sdp = sdir;
  //  struct rm *lev;

  if (obj != uwep) {
    if (uwep && uwep->bitflags & O_IS_CURSED) {
      /* Andreas Bormann - ihnp4!decvax!mcvax!unido!ab */
      message("Since your weapon is welded to your hand,");
      message("you cannot use that pick-axe.");
      return false;
    }
    StrPrintF(ScratchBuffer, "You now wield %s.", doname(obj));
    message(ScratchBuffer);
    setuwep(obj);
    return true; // need to take a turn
  }
  // (Then there was some code to print the valid directions.  Omitted.)
  return false;
}

static void do_dig(Int8 x, Int8 y, Boolean down, UChar level)
{
  Boolean new_dig = false, cont = false;
  // get ready to dig....
  digs.is_in_use = true;
  if (digs.pos.x != x || digs.pos.y != y || digs.down != down ||
      digs.level != level) {
    new_dig = true;
    digs.pos.x = x;
    digs.pos.y = y;
    digs.down = down;
    digs.level = level;
    digs.effort = 0;
    if (down && inshop()) shopdig(false);
  }
  StrPrintF(ScratchBuffer, "You %s digging%s",
	    (new_dig ? "start" : "continue"), (down ? "in the floor." : "."));
  message(ScratchBuffer);
  // ...ok, ready to dig.
  stop_occupation_now = false;
  do {
    tick();
    if (monster_nearby() || stop_occupation_now) {
      message("You stop digging.");
      break;
    } else if (!dig(cont))
      break;
  } while ((cont = true));
  // main loop will call tock() to print messages; don't do it here.
  digs.is_in_use = false;
}

// So.  All of these actions are supposed to take at least a tick.
Boolean use_pick_axe(obj_t *obj)
{
  monst_t *mtmp;
  Short rx, ry;
  // the caller promises that dx||dy||dz.
  // these were local globals:

  if (you.uswallow && attack(you.ustuck)) return true;
  if (you.dz < 0) {
    message("You cannot reach the ceiling."); return true;
  } else if (you.dz > 0 && Levitation ) {
    message("You cannot reach the floor."); return true;
  }
  if (you.dz == 0) {
    // Digging in a wall.
    if (Confusion)
      confdir();
    rx = you.ux + you.dx;
    ry = you.uy + you.dy;
    if ((mtmp = mon_at(rx, ry)) && attack(mtmp))
      return true;
    if (OUT_OF_BOUNDS(rx, ry)) {
      message("Clash!");
      return true;
    }
    if (get_cell_type(floor_info[rx][ry]) == DOOR) {
      StrPrintF(ScratchBuffer, "Your %s against the door.",
		aobjnam(obj, "clang"));
      message(ScratchBuffer);
      return true;
    } else if (!IS_ROCK(get_cell_type(floor_info[rx][ry])) &&
	       !sobj_at(ENORMOUS_ROCK, rx, ry)) {
      /* ACCESSIBLE or POOL */
      StrPrintF(ScratchBuffer, "You swing your %s through thin air.",
		aobjnam(obj, NULL));
      message(ScratchBuffer);
      return true;
    } else {
      do_dig(rx, ry, false, dlevel);
      //      occupation = dig;
      //      occtxt = "digging";
      return false;
    }
  } else {
    // Digging in the floor.
    do_dig(you.ux, you.uy, true, dlevel);
    //    occupation = dig;
    //    occtxt = "digging";
    return false;
  }

}
