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

Boolean do_read(obj_t *scroll)
{
  Boolean confused = (Confusion != 0);
  Boolean known = false;

  if (!scroll) return false;
  if (!(scroll->bitflags & O_IS_DESCKNOWN) && Blind) {
    message("Being blind, you cannot read the formula on the scroll.");
    return false;
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
	strange_feeling(scroll,"Your skin glows then fades.");
	return true;
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
	strange_feeling(scroll,"Your bones itch.");
	return true;
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
    } else {
      strange_feeling(scroll,"Your skin itches.");
      return true;
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

      if (!rund(73)) cnt += rnd(4);
      if (confused)  cnt += 12;
      while (cnt--)
	makemon((confused ? PM_ACID_BLOB : NULL), you.ux, you.uy);
      break;
    }
  case SCR_ENCHANT_WEAPON:
    if (uwep && confused) {
      StrPrintF(ScratchBuffer, "Your %s glows silver for a moment.",
		oc_names + objects[uwep->otype].oc_name_offset);
      message(ScratchBuffer);
      uwep->bitflags |= O_IS_RUSTFREE;
    } else
      if (!chwepon(scroll, 1))		/* tests for !uwep */
	return true;
    break;
  case SCR_DAMAGE_WEAPON:
    if (uwep && confused) {
      StrPrintF(ScratchBuffer, "Your %s glows purple for a moment.",
		oc_names + objects[uwep->otype].oc_name_offset);
      message(ScratchBuffer);
      uwep->bitflags &= ~O_IS_RUSTFREE;
    } else
      if (!chwepon(scroll, -1))	/* tests for !uwep */
	return true;
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
    }
    // hey... I should probably make it not take a turn until afterward..
    break;
  case SCR_LIGHT:
    if (!Blind) known = true;
    litroom(!confused);
    break;
  case SCR_TELEPORTATION:
    if (confused)
      ; //      level_tele(); // XXXX Not implemented yet!  Needs UI!
    else {
      Short uroom = inroom(you.ux, you.uy);
      //      tele(); // XXXX Not implemented yet!  Needs UI!
      if (uroom != inroom(you.ux, you.uy)) known = true;
    }
    break;
  case SCR_GOLD_DETECTION:
    /* Unfortunately this code has become slightly less elegant,
       now that gold and traps no longer are of the same type. */
    // Ha!  I can use the SenseForm again.
    if (confused) {
      trap_t *ttmp;

      if (!ftrap) {
	strange_feeling(scroll, "Your toes stop itching.");
	return true;
      } else {
	for (ttmp = ftrap; ttmp; ttmp = ttmp->ntrap)
	  if (ttmp->tx != you.ux || ttmp->ty != you.uy) {
	    sense_what = SENSE_GOLD_CONFUSED;
	    sense_by_what = scroll;
	    FrmPopupForm(SenseForm);
	    return false; // postpone the tick
	  }
	message("Your toes itch!");
	break;
      }
    } else {
      gold_t *gtmp;

      if (!fgold) {
	strange_feeling(scroll, "You feel materially poor.");
	return true;
      } else {
	known = true;
	for (gtmp = fgold; gtmp; gtmp = gtmp->ngold)
	  if (gtmp->gx != you.ux || gtmp->gy != you.uy) {
	    sense_what = SENSE_GOLD;
	    sense_by_what = scroll;
	    FrmPopupForm(SenseForm);
	    return false; // postpone the tick
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

      for (obj = fobj; obj; obj = obj->nobj)
	if (obj->olet == FOOD_SYM) {
	  if (obj->ox == you.ux && obj->oy == you.uy) ctu++;
	  else ct++;
	}
      if (!ct && !ctu) {
	strange_feeling(scroll,"Your nose twitches.");
	return true;
      } else if (!ct) {
	known = true;
	StrPrintF(ScratchBuffer, "You smell %s close nearby.",
		  confused ? "something" : "food");
	message(ScratchBuffer);
      } else {
	known = true;
	sense_what = (confused ? SENSE_FOOD_CONFUSED : SENSE_FOOD);
	sense_by_what = scroll;
	FrmPopupForm(SenseForm);
	return false; // postpone the tick
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
    message("Identify is not implemented yet");
    return true;
  case SCR_MAGIC_MAPPING: // XXX Doesn't actually work yet.
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
	  } else
	    if (num == SDOOR) {
	      set_cell_type(floor_info[zx][zy], DOOR); // lev->typ = DOOR;
	      floor_symbol[zx][zy] = DOOR_SYM; // lev->scrsym = '+';
	      /* do sth in doors ? */
	    } else if (!get_cell_seen(floor_info[zx][zy])) continue;//lev->seen
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
      Short zx, zy;

      known = true;
      for (zx = 0; zx < DCOLS; zx++)
	for (zy = 0; zy < DROWS; zy++)
	  if (!confused || rund(7))
	    if (!cansee(zx,zy))
	      floor_info[zx][zy] &= ~SEEN_CELL; // levl[zx][zy].seen = 0;
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
	message("The scroll catches fire and you burn your hands.");
	losehp(1, "scroll of fire");
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
  finish_do_scroll(scroll, known, confused);
  return true;
}
// 'cause we also need to call this after leaving the SenseForm!
void finish_do_scroll(obj_t *scroll, Boolean known, Boolean confused)
{
  if (!scroll) return;
    // XXX I need a "finish_do_scroll(sense_by_what, bool1, bool2)"
  if (!BITTEST(oc_name_known, scroll->otype)) {
    if (known && !confused) {
      // XXX need to make its name known.
      BITSET(oc_name_known, scroll->otype);
      more_experienced(0,10);
    } else if (!oc_has_uname(scroll->otype)) {
      // here's what I'll do instead of do_call/docall(scroll) :
      show_all_messages();
      clone_for_call(scroll); // so we don't have to worry about useup!
      FrmPopupForm(EngraveForm);
    }
  }
  useup(scroll);
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
    killer = "scroll of genocide"; // XXXX
    you.uhp = -1; // hmm but we don't call 'done' yet?
  }
}
