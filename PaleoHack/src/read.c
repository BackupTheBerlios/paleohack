/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
#include "paleohackRsc.h"
#include "bit.h"


Boolean monstersym(Char ch) SEC_4;
extern Short engrave_or_what;


extern UChar sense_what;
extern obj_t *sense_by_what;

tri_val_t do_read(obj_t *scroll)
{
  Boolean confused = (Confusion != 0);
  Boolean known = false;
  Boolean b;

  if (!scroll) return NO_OP;
  if (!(scroll->bitflags & O_IS_DESCKNOWN) && Blind) {
    message("Being blind, you cannot read the formula on the scroll.");
    return NO_OP;
  }
  if (Blind)
    message("As you pronounce the formula on it, the scroll disappears.");
  else
    message("As you read the scroll, it disappears.");
  if (confused)
    message("Being confused, you mispronounce the magic words ... ");

  switch(scroll->otype) {
#ifdef MAIL
  case SCR_MAIL:
    readmail(/* scroll */);
    break;
#endif MAIL
  case SCR_ENCHANT_ARMOR:
    {
      obj_t *otmp = some_armor();
      Char *oname;
      if (!otmp) {
	b = strange_feeling(scroll,"Your skin glows then fades.");
	// strange_feeling calls useup(scroll) etc, that's why we return now.
	// it returns true if it popped up the engrave form.
	return ((b) ? GO_ON : DONE);
      }
      oname = oc_names + objects[otmp->otype].oc_name_offset;
      if (confused) {
	StrPrintF(ScratchBuffer, "Your %s glows silver for a moment.", oname);
	message(ScratchBuffer);
	otmp->bitflags |= O_IS_RUSTFREE;
	break;
      }
      if (otmp->spe > 3 && rund(otmp->spe)) {
	StrPrintF(ScratchBuffer, "Your %s glows violently green for a while, then evaporates.",
		  oname);
	message(ScratchBuffer);
	useup(otmp);
	break;
      }
      StrPrintF(ScratchBuffer, "Your %s glows green for a moment.", oname);
      message(ScratchBuffer);
      otmp->bitflags &= ~O_IS_CURSED;
      otmp->spe++;
      break;
    }
  case SCR_DESTROY_ARMOR:
    if (confused) {
      obj_t *otmp = some_armor();
      if (!otmp) {
	b = strange_feeling(scroll,"Your bones itch.");
	return ((b) ? GO_ON : DONE);
      }
      StrPrintF(ScratchBuffer, "Your %s glows purple for a moment.",
		oc_names + objects[otmp->otype].oc_name_offset);
      message(ScratchBuffer);
      otmp->bitflags &= ~O_IS_RUSTFREE;
      break;
    }
    if (uarm) {
      message("Your armor turns to dust and falls to the floor!");
      useup(uarm);
    } else if (uarmh) {
      message("Your helmet turns to dust and is blown away!");
      useup(uarmh);
    } else if (uarmg) {
      message("Your gloves vanish!");
      useup(uarmg);
      selftouch("You"); // Checks to see if you're wielding a dead cockatrice.
    } else if (uarms) { // This case was added as a bugfix in 1980s.
      message("Your shield crumbles away!");
      useup(uarms);
    } else {
      b = strange_feeling(scroll,"Your skin itches.");
      return ((b) ? GO_ON : DONE);
    }
    break;
  case SCR_CONFUSE_MONSTER:
    if (confused) {
      message("Your hands begin to glow purple.");
      Confusion += rnd(100);
    } else {
      message("Your hands begin to glow blue.");
      you.umconf = true;
    }
    break;
  case SCR_SCARE_MONSTER:
    {
      Short ct = 0;
      monst_t *mtmp;
      for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
	if (cansee(mtmp->mx,mtmp->my)) {
	  if (confused) {
	    mtmp->bitflags &= ~(M_IS_ASLEEP | M_IS_FROZEN);
	    mtmp->mflee_and_time &= ~M_FLEEING;
	  } else {
	    mtmp->mflee_and_time |= M_FLEEING;
	  }
	  ct++;
	}
      if (!ct) {
	if (confused) message("You hear sad wailing in the distance.");
	else          message("You hear maniacal laughter in the distance.");
      }
      break;
    }
  case SCR_BLANK_PAPER:
    if (confused) message("You see strange patterns on this scroll.");
    else          message("This scroll seems to be blank.");
    break;
  case SCR_REMOVE_CURSE:
    {
      obj_t *obj;
      if (confused) message("You feel like you need some help.");
      else          message("You feel like someone is helping you.");
      for (obj = invent; obj ; obj = obj->nobj)
	if (obj->owornmask) {
	  if (confused) obj->bitflags |= O_IS_CURSED;
	  else          obj->bitflags &= ~O_IS_CURSED;
	}
      if (Punished && !confused) {
	// destroy the chain, leave the ball (handy eh)
	Punished = 0;
	unlink_obj(uchain); // freeobj
	unpobj(uchain);
	free_me((VoidPtr) uchain); //free((char *) uchain);
	uball->spe = 0;
	uball->owornmask &= ~W_BALL;
	uchain = uball = NULL;
      }
      break;
    }
  case SCR_CREATE_MONSTER:
    {
      Short cnt = 1;
      Boolean made = false;

      if (!rund(73)) cnt += rnd(4);
      if (confused)  cnt += 12;
      while (cnt--)
	made = (0!=makemon((confused ? PM_ACID_BLOB : NULL), you.ux, you.uy));
      if (made)
	message("You hear a tiny thunderclap.");
      break;
    }
  case SCR_ENCHANT_WEAPON:
    if (uwep && confused) {
      //      StrPrintF(ScratchBuffer, "Your %s glows silver for a moment.",
      //		oc_names + objects[uwep->otype].oc_name_offset);
      StrPrintF(ScratchBuffer, "Your %s %s silver for a moment.",
		aobjnam(uwep, "glow"));      // bug fix from 1980s.
      message(ScratchBuffer);
      uwep->bitflags |= O_IS_RUSTFREE;
    } else {
      tri_val_t t = chwepon(scroll, 1);		/* tests for !uwep */
      if (t == NO_OP) return DONE;
      if (t == GO_ON) return GO_ON;
      // else strange_feeling was not called, so keep on trucking.
    }
    break;
  case SCR_DAMAGE_WEAPON:
    if (uwep && confused) {
      //      StrPrintF(ScratchBuffer, "Your %s glows purple for a moment.",
      //		oc_names + objects[uwep->otype].oc_name_offset);
      StrPrintF(ScratchBuffer, "Your %s %s purple for a moment.",
		aobjnam(uwep, "glow"));      // bug fix from 1980s.
      message(ScratchBuffer);
      uwep->bitflags &= ~O_IS_RUSTFREE;
    } else {
      tri_val_t t = chwepon(scroll, -1);		/* tests for !uwep */
      if (t == NO_OP) return DONE;
      if (t == GO_ON) return GO_ON;
      // else strange_feeling was not called, so keep on trucking.
    }
    break;
  case SCR_TAMING:
    {
      Short i,j;
      Short bd = confused ? 5 : 1;
      monst_t *mtmp;

      for (i = -bd; i <= bd; i++) for (j = -bd; j <= bd; j++)
	if ((mtmp = mon_at(you.ux+i, you.uy+j)))
	  tamedog(mtmp, NULL);
      break;
    }
  case SCR_GENOCIDE:
    message("You have found a scroll of genocide!");
    known = true;
    if (confused) {
      Char buf[2] = { you.usym, 0 };
      do_genocide(buf);
    } else {
      engrave_or_what = GET_GENOCIDE;
      FrmPopupForm(EngraveForm);
      // hey... I should probably make it not take a turn until afterward..
      // adding:
      if (GO_ON != finish_do_read(scroll, known, confused))
	show_all_messages();
      // the 'else' will actually never happen.
      return GO_ON;
    }
    break;
  case SCR_LIGHT:
    if (!Blind) known = true;
    litroom(!confused);
    break;
  case SCR_TELEPORTATION:
    if (confused) {
      level_tele_start();
    } else {
      extern Short map_mode_teleport;
      Short uroom = inroom(you.ux, you.uy);
      map_mode_teleport = TELE_POTSCROLL;
      tele();
      if (uroom != inroom(you.ux, you.uy)) known = true;
    }
    if (Teleport_control) {
      // User is prompted for location to teleport to - DON'T take a turn.
      known = true; /* since it ASKS about teleport you KNOW it's teleport.
		       I assert: whether you're confused or not.
		       this may not be 'correct' behavior,
		       but it will sure make MY life easier. */
      if (GO_ON != finish_do_read(scroll, known, false/*confused*/))
	show_all_messages();
      return GO_ON;
    }
    break;
  case SCR_GOLD_DETECTION:
    /* Unfortunately this code has become slightly less elegant,
       now that gold and traps no longer are of the same type. */
    // Ha!  I can use the SenseForm again.
    if (confused) {
      trap_t *ttmp;

      if (!ftrap) {
	b = strange_feeling(scroll, "Your toes stop itching.");
	return ((b) ? GO_ON : DONE);
      } else {
	for (ttmp = ftrap; ttmp; ttmp = ttmp->ntrap)
	  if (ttmp->tx != you.ux || ttmp->ty != you.uy) {
	    sense_what = SENSE_GOLD_CONFUSED;
	    sense_by_what = scroll;
	    sense_init_screen();
	    FrmPopupForm(SenseForm);
	    return GO_ON; // postpone the tick
	  }
	message("Your toes itch!");
	break;
      }
    } else {
      gold_t *gtmp;

      if (!fgold) {
	b = strange_feeling(scroll, "You feel materially poor.");
	return ((b) ? GO_ON : DONE);
      } else {
	known = true;
	for (gtmp = fgold; gtmp; gtmp = gtmp->ngold)
	  if (gtmp->gx != you.ux || gtmp->gy != you.uy) {
	    sense_what = SENSE_GOLD;
	    sense_by_what = scroll;
	    sense_init_screen();
	    FrmPopupForm(SenseForm);
	    return GO_ON; // postpone the tick
	  }
	message("You notice some gold between your feet.");
	break;
      }
    }
    break;
  case SCR_FOOD_DETECTION:
    {
      Short ct = 0, ctu = 0;
      obj_t *obj;

      Char foodsym = confused ? POTION_SYM : FOOD_SYM;
      for (obj = fobj; obj; obj = obj->nobj)
	//	if (obj->olet == FOOD_SYM) { // Bug fix from 1980s:
	if (obj->olet == foodsym) {
	  if (obj->ox == you.ux && obj->oy == you.uy) ctu++;
	  else ct++;
	}
      if (!ct && !ctu) {
	b = strange_feeling(scroll,"Your nose twitches.");
	return ((b) ? GO_ON : DONE);
      } else if (!ct) {
	known = true;
	StrPrintF(ScratchBuffer, "You smell %s close nearby.",
		  confused ? "something" : "food");
	message(ScratchBuffer);
      } else {
	known = true;
	sense_what = (confused ? SENSE_FOOD_CONFUSED : SENSE_FOOD);
	sense_by_what = scroll;
	sense_init_screen();
	FrmPopupForm(SenseForm);
	return GO_ON; // postpone the tick
      }
      break;
    }
  case SCR_IDENTIFY:
    /* known = true; */ // <-- this was commented out.
    if (confused)
      message("You identify this as an identify scroll.");
    else
      message("This is an identify scroll.");
    useup(scroll);
    BITSET(oc_name_known, SCR_IDENTIFY);
    /*
      if (!confused)
      while (
      !ggetobj("identify", identify, rund(5) ? 1 : rund(5))
      && invent
      );
    */
    //    identify_count = rund(5) ? 1 : rund(5);
    if (!invent) {
      message("You have nothing to identify.");
      return DONE; // we already used up the scroll..
    } else {
      extern Boolean drop_not_identify;
      drop_not_identify = false;
      show_messages(); // maybe.
      FrmPopupForm(ObjTypeForm);
      return GO_ON;
    }
  case SCR_MAGIC_MAPPING: //Doesn't actually work yet. // Works (oct03)
    {
      //      struct rm *lev;
      Short num, zx, zy;
	
      known = true;
      StrPrintF(ScratchBuffer, "On this scroll %s a map!",
		confused ? "was" : "is");
      message(ScratchBuffer);
      for (zy = 0; zy < DROWS; zy++)
	for (zx = 0; zx < DCOLS; zx++) {
	  if (confused && rund(7)) continue;
	  //	  lev = &(levl[zx][zy]);
	  if ((num = get_cell_type(floor_info[zx][zy])) == 0) // was lev->typ
	    continue;
	  if (num == SCORR) {
	    set_cell_type(floor_info[zx][zy], CORR); // lev->typ = CORR;
	    floor_symbol[zx][zy] = CORR_SYM; // lev->scrsym = CORR_SYM;
	  } else if (num == SDOOR) {
	    set_cell_type(floor_info[zx][zy], DOOR); // lev->typ = DOOR;
	    floor_symbol[zx][zy] = DOOR_SYM; // lev->scrsym = '+';
	    /* do sth in doors ? */
	  } else if (get_cell_seen(floor_info[zx][zy])) continue;//lev->seen
	  if (num != ROOM) {
	    floor_info[zx][zy] |= (NEW_CELL|SEEN_CELL); //lev->seen=lev->new=1;
	    if (floor_symbol[zx][zy] == ' ' || !floor_symbol[zx][zy])
	      newsym(zx,zy);
	    else
	      on_scr(zx,zy);
	  }
	}
    }
    break;
  case SCR_AMNESIA:
    {
      void clear_visible();
      Short zx, zy;

      known = true;
      for (zx = 0; zx < DCOLS; zx++)
	for (zy = 0; zy < DROWS; zy++)
	  if (!confused || rund(7))
	    if (!cansee(zx,zy)) {
	      // floor_info[zx][zy] &= ~SEEN_CELL; // levl[zx][zy].seen = 0;
	      floor_info[zx][zy] &= ~(SEEN_CELL|NEW_CELL);//levl[zx][zy].seen=0;
	      // see if this fixes the "some stuff is not cleared" bug....
	    }
      clear_visible();
      refresh(); // docrt();
      message("Thinking of Maud you forget everything else.");
      break;
    }
  case SCR_FIRE:
    {	
      Short num=0;
      monst_t *mtmp;

      known = true;
      if (confused) {
	// fire resistance case was added as bugfix from 1980s.
	if (Fire_resistance) {
	  message("The scroll catches fire.");
	  message("You are uninjured!");
	} else {
	  message("The scroll catches fire and you burn your hands.");
	  losehp(1, "scroll of fire");
	}
      } else {
	message("The scroll erupts in a tower of flame!");
	if (Fire_resistance)
	  message("You are uninjured.");
	else {
	  num = rnd(6);
	  you.uhpmax -= num;
	  losehp(num, "scroll of fire");
	}
      }
      num = (2*num + 1)/3;
      for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	if (dist(mtmp->mx,mtmp->my) < 3) {
	  mtmp->mhp -= num;
	  if (mtmp->data->mlet == 'F' || mtmp->data->mlet == 'Y')
	    mtmp->mhp -= 3*num;	/* this might well kill 'F's */
	  if (mtmp->mhp < 1) {
	    killed(mtmp);
	    break;		/* primitive */
	  }
	}
      }
      break;
    }
  case SCR_PUNISHMENT:
    known = true;
    if (confused) {
      message("You feel guilty.");
      break;
    }
    message("You are being punished for your misbehaviour!");
    if (Punished){
      message("Your iron ball gets heavier.");
      uball->owt += 15;
      break;
    }
    Punished = INTRINSIC;
    setworn(mkobj_at(CHAIN_SYM, you.ux, you.uy), W_CHAIN);
    setworn(mkobj_at(BALL_SYM, you.ux, you.uy), W_BALL);
    uball->spe = 1;		/* special ball (see save) */
    break;
  default:
    StrPrintF(ScratchBuffer, "What weird language is this written in? (%u)",
	      scroll->otype);
    message(ScratchBuffer);
    break;
  }
  return finish_do_read(scroll, known, confused);
  //  return true;
}
// 'cause we also need to call this after leaving the SenseForm!
tri_val_t finish_do_read(obj_t *scroll, Boolean known, Boolean confused)
{
  Boolean go_on = false;
  if (!scroll) return NO_OP; // bug if that happens.
  // at least one case can kill you (cockatrice + destroy armor scroll) so:
  if (you.dead) return DONE;
    // XXX I need a "finish_do_read(sense_by_what, bool1, bool2)"
  if (!BITTEST(oc_name_known, scroll->otype)) {
    if (known && !confused) {
      // need to make its name known.
      BITSET(oc_name_known, scroll->otype);
      more_experienced(0,10);
    } else if (!oc_has_uname(scroll->otype)) {
      // here's what I'll do instead of do_call/docall(scroll) :
      show_all_messages();
      clone_for_call(scroll); // so we don't have to worry about useup!
      FrmPopupForm(EngraveForm);
      go_on = true;
    }
  }
  useup(scroll);
  if (go_on) return GO_ON;
  else       return DONE;
}



// identify() can be static because we have no mail.
// it needs to set oc_name_known...

// litroom(Boolean on) has been moved to movesee.c

Boolean monstersym(Char ch)
{
  struct permonst *mp;

  /*
   * can't genocide certain monsters
   */
  if (StrChr("12 &:", ch))
    return false;

  if (ch == (PM_EEL)->mlet)
    return true;
  for (mp = mons ; mp < &mons[CMNUM+2] ; mp++)
    if (mp->mlet == ch)
      return true;
  return false;
}


extern Char fut_geno[];
extern Char genocided[];
void do_genocide(Char *buf)
{
  monst_t *mtmp, *mtmp2;
  if (!buf || !*buf || !monstersym(*buf)) {
    // Caller should have caught this and demanded better input.
    message("That's not a monster.");
    return;
  }

  if (!StrChr(fut_geno, *buf))
    charcat(fut_geno, *buf);
  if (!StrChr(genocided, *buf))
    charcat(genocided, *buf);
  else {
    message("Such monsters do not exist in this world.");
    return;
  }
  for (mtmp = fmon; mtmp; mtmp = mtmp2) {
    mtmp2 = mtmp->nmon;
    if (mtmp->data->mlet == *buf)
      mondead(mtmp);
  }
  StrPrintF(ScratchBuffer, "Wiped out all %c's.", *buf);
  message(ScratchBuffer);
  if (*buf == you.usym) {
    killer = "scroll of genocide";     // ... I have tested it and it works!
    you.uhp = -1; // (we don't call 'done' yet?  ok..)
  }
}

