/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
#include "paleohackRsc.h"

// some need to move to prototypes.h
static Boolean opentin() SEC_2;
static Boolean do_eat_tin(obj_t *otmp) SEC_2;
static void unfaint() SEC_2;
static void newuhs(Boolean incr) SEC_2;
static Boolean eatcorpse(struct obj *otmp) SEC_2;
static void init_rumors() SEC_4;
static void show_rumor(Short i) SEC_4;
static void outrumor() SEC_4;

extern Short multi; // living in movesee.c right now..

//#define	TTSZ	SIZE(tintxts)
#define NUM_TIN_TYPES 6
/*  struct { Char *txt; Short nut; } tintxts[] = { */
/*    { "It contains first quality peaches - what a surprise!", 40 }, */
/*    { "It contains salmon - not bad!", 60 }, */
/*    { "It contains apple juice - perhaps not what you hoped for.", 20 }, */
/*    { "It contains some nondescript substance, tasting awfully.", 500 }, */
/*    { "It contains rotten meat. You vomit.", -50 }, */
/*    { "It turns out to be empty.", 0 } */
/*  }; */
Short tin_nutrition[NUM_TIN_TYPES] = { 40, 60, 20, 500, -50, 0 };
Char *tin_message[NUM_TIN_TYPES] = {
  "It contains first quality peaches - what a surprise!",
  "It contains salmon - not bad!",
  "It contains apple juice - perhaps not what you hoped for.",
  "It contains some nondescript substance, tasting awful.",
  "It contains rotten meat. You vomit.",
  "It turns out to be empty."
};

static struct {
  obj_t *tin;
  Short usedtime;
  Short reqtime;
} tin;


// Returns false if you are still trying, true if you are done trying.
static Boolean opentin()
{
  Short r;

  if (!carried(tin.tin))		/* perhaps it was stolen? */
    return true;		/* %% probably we should use tinoid */
  if (tin.usedtime++ >= 50) {
    message("You give up your attempt to open the tin.");
    return true;
  }
  if (tin.usedtime < tin.reqtime)
    return false;		/* still busy */
  
  message("You succeed in opening the tin.");
  useup(tin.tin);
  r = rund(2 * NUM_TIN_TYPES);
  if (r < NUM_TIN_TYPES) {
    message(tin_message[r]);
    lesshungry(tin_nutrition[r]);
    if (r == 1)	/* SALMON */ {
      Glib = rnd(15);
      message("Eating salmon made your fingers very slippery.");
    }
  } else {
    message("It contains spinach - this makes you feel like Popeye!");
    lesshungry(600);
    if (you.ustr < 118)
      you.ustr += rnd( ((you.ustr < 17) ? 19 : 118) - you.ustr);
    if (you.ustr > you.ustrmax) you.ustrmax = you.ustr;
    flags.botl |= BOTL_STR;
  }
  return true;
}

// A callbacky thing, for when eating-a-mimic wears off.
void M_eat_done()
{
  you.usym = '@';
  prme();
}


// XXXX I need a routine to call, when a person asks to 'e'at
// some food that is still on the ground and not in inventory....
Boolean eat_off_floor() // was part of doeat
{
  obj_t *otmp;
  /* Is there some food (probably a heavy corpse) here on the ground? */
  if (!Levitation) {
    for (otmp = fobj; otmp; otmp = otmp->nobj) {
      if (otmp->ox == you.ux && otmp->oy == you.uy &&
	  otmp->olet == FOOD_SYM) {
	if (0 == FrmCustomAlert(EatFloorP,
				(otmp->quantity == 1) ? "is" : "are",
				doname(otmp),
				(otmp->quantity == 1) ? "it?" : "one?")) {
	  if (otmp->quantity != 1)
	    splitobj(otmp, 1);
	  unlink_obj(otmp); //aka freeobj(otmp);
	  otmp = addinv(otmp);
	  addtobill(otmp);
	  return do_eat(otmp);
	}
      }
    }
  }
  return false;  // I could make it pop up inventory.. nah.
}

extern Boolean stop_occupation_now; // in apply.c
static Boolean do_eat_tin(obj_t *otmp) // XXX there may be a bug in hre
{
  Short tmp;
  if (uwep) {
    switch(uwep->otype) {
    case AXE:    case PICK_AXE:  tmp = 6; break;
    case DAGGER: case CRYSKNIFE: tmp = 3; break;
    case CAN_OPENER:             tmp = 1; break;
    default:
      goto no_opener;
    }
    StrPrintF(ScratchBuffer, "Using your %s you try to open the tin.",
	      aobjnam(uwep, NULL));
    message(ScratchBuffer);
  } else {
  no_opener:
    message("It is not so easy to open this tin.");
    if (Glib) {
      message("The tin slips out of your hands.");
      if (otmp->quantity > 1) {
	obj_t *obj;
	obj = splitobj(otmp, 1);
	if (otmp == uwep) setuwep(obj);
      }
      dropx(otmp);
      return true;
    }
    tmp = 10 + rund(1 + 500/((Short)(you.ulevel + you.ustr)));
  }
  tin.reqtime = tmp;
  tin.usedtime = 0;
  tin.tin = otmp;
  //    occupation = opentin; // always {dig, opentin, countgold} pointer.
  //    occtxt = "opening the tin"; // ... used for a "you stop %s" message.
  // Perhaps the best thing to do is to have individual loops
  // as in kMoria/src/dungeon.c for repeated commands...................
  // It's interrupted by "if (monster_nearby())"
  // in which case we "stop_occupation();"
  stop_occupation_now = false;
  while (true) {
    tick();
    if (monster_nearby() || stop_occupation_now) {
      message("You stop opening the tin.");
      break;
    } else if (opentin())
      break;
  }
  tock();  // refresh screen, print the several accumulated messages.
  return false; // do not tick any more turns!
}

// This will do the eating, after you have an otmp that is in your inventory.
Boolean do_eat(obj_t *otmp)
{
  static Char msgbuf[BUFSZ];
  objclass_t *ftmp;

  if (otmp->otype == TIN)
    return do_eat_tin(otmp);

  ftmp = &objects[otmp->otype];
  multi = -ftmp->oc_delay;
  if (otmp->otype >= CORPSE && eatcorpse(otmp)) goto eatx;
  if (!rund(7) && otmp->otype != FORTUNE_COOKIE) {
    message("Blecch!  Rotten food!");
    if (!rund(4)) {
      message("You feel rather light headed.");
      Confusion += dice(2,4);
    } else if (!rund(4)&& !Blind) {
      message("Everything suddenly goes dark.");
      Blind = dice(2,10);
      seeoff(false);
    } else if (!rund(3)) {
      if (Blind)
	message("The world spins and you slap against the floor.");
      else
	message("The world spins and goes dark.");
      nomul(-rnd(10));
      //      nomovemsg = "You are conscious again."; // XXX
    }
    lesshungry(ftmp->nutrition / 4);
  } else {
    if (you.uhunger >= 1500) {
      message("You choke over your food.");
      message("You die...");
      killer = oc_names + ftmp->oc_name_offset;// (was ftmp->oc_name) // xxx
      done("choked"); // XXXXX
      return false;
    }
    switch(otmp->otype){
    case FOOD_RATION:
      if (you.uhunger <= 200)
	message("That food really hit the spot!");
      else if (you.uhunger <= 700)
	message("That satiated your stomach!");
      else {
	message("You're having a hard time getting all that food down.");
	multi -= 2; // XXXX hey!  I need another while loop!!!
      }
      lesshungry(ftmp->nutrition);
      //      if (multi < 0) nomovemsg = "You finished your meal."; // XXX
      break;
    case TRIPE_RATION:
      message("Yak - dog food!");
      more_experienced(1,0);
      flags.botl |= BOTL_EXP;
      if (rund(2)) {
	message("You vomit.");
	morehungry(20);
	if (Sick) {
	  Sick = 0;	// David Neves
	  message("What a relief!");
	}
      } else	lesshungry(ftmp->nutrition);
      break;
    default:
      if (otmp->otype >= CORPSE)
	StrPrintF(ScratchBuffer, "That %s tasted terrible!", 
		  oc_names + ftmp->oc_name_offset);
      else
	StrPrintF(ScratchBuffer, "That %s was delicious!", 
		  oc_names + ftmp->oc_name_offset);
      message(ScratchBuffer);
      lesshungry(ftmp->nutrition);
      if (otmp->otype == DEAD_LIZARD && (Confusion > 2))
	Confusion = 2;
      else if (otmp->otype == FORTUNE_COOKIE) {
	if (Blind) {
	  message("This cookie has a scrap of paper inside!");
	  message("What a pity that you cannot read it!");
	} else {
	  outrumor();
	}
      } else if (otmp->otype == LUMP_OF_ROYAL_JELLY) {
				// This stuff seems to be VERY healthy!
	if (you.ustrmax < 118) you.ustrmax++;
	if (you.ustr < you.ustrmax) you.ustr++;
	you.uhp += rnd(20);
	if (you.uhp > you.uhpmax) {
	  if (!rund(17)) you.uhpmax++;
	  you.uhp = you.uhpmax;
	  flags.botl |= (BOTL_HP | BOTL_STR);
	}
	heal_legs();
      }
      break;
    }
  }
 eatx:
  if (you.dead) return false;
  //  if (multi < 0 && !nomovemsg) { // XXX
      StrPrintF(msgbuf, "You finished eating the %s.",
	        oc_names + ftmp->oc_name_offset);
    //    nomovemsg = msgbuf; // XXX
    //  }
    useup(otmp); // XXX there may be a bug in here
  return true;
}





/* called in hack.main.c */
void gethungry()
{
  --you.uhunger;
  if (moves % 2) {
    if (Regeneration) you.uhunger--;
    if (Hunger) you.uhunger--;
    /* a3:  if (Hunger & LEFT_RING) you.uhunger--;
       if (Hunger & RIGHT_RING) you.uhunger--;
       etc. */
  }
  if (moves % 20 == 0) {			/* jimt@asgb */
    if (uleft) you.uhunger--;
    if (uright) you.uhunger--;
  }
  newuhs(true);
}

/* called after vomiting and after performing feats of magic */
void morehungry(Short num)
{
  you.uhunger -= num;
  newuhs(true);
}

/* called after eating something (and after drinking fruit juice) */
void lesshungry(Short num)
{
  you.uhunger += num;
  newuhs(false);
}

// A callbacky thing, for when fainting wears off.
static void unfaint()
{
  you.uhs = FAINTING;
  flags.botl |= BOTL_HUNGER;
}



static void newuhs(Boolean incr)
{
  Short newhs, h = you.uhunger;

  newhs = (h > 1000) ? SATIATED :
    (h > 150) ? NOT_HUNGRY :
    (h > 50) ? HUNGRY :
    (h > 0) ? WEAK : FAINTING;

  if (newhs == FAINTING) {
    if (you.uhs == FAINTED)
      newhs = FAINTED;
    if (you.uhs <= WEAK || rund(20-you.uhunger/10) >= 19) {
      if (you.uhs != FAINTED && multi >= 0) {  // %%
	message("You faint from lack of food.");
	nomul(-10+(you.uhunger/10));
	//	nomovemsg = "You regain consciousness."; // XXX
	//	afternmv = unfaint; // XXX
	newhs = FAINTED;
      }
    } else
      if (you.uhunger < -(Short)(200 + 25*you.ulevel)) {
	you.uhs = STARVED;
	flags.botl |= BOTL_HUNGER;
	//	bot(); // XXX
	message("You die from starvation.");
	done("starved"); // xxx
	return;
      }
  }

  if (newhs != you.uhs) {
    if (newhs >= WEAK && you.uhs < WEAK)
      losestr(1);	// this may kill you -- see below
    else
      if (newhs < WEAK && you.uhs >= WEAK && you.ustr < you.ustrmax)
	losestr(-1);
    switch(newhs) {
    case HUNGRY:
      message((!incr) ? "You only feel hungry now." :
	      (you.uhunger < 145) ? "You feel hungry." :
	      "You are beginning to feel hungry.");
      break;
    case WEAK:
      message((!incr) ? "You feel weak now." :
	      (you.uhunger < 45) ? "You feel weak." :
	      "You are beginning to feel weak.");
      break;
    }
    you.uhs = newhs;
    flags.botl |= BOTL_HUNGER;
    if (you.uhp < 1) {
      message("You die from hunger and exhaustion.");
      killer = "exhaustion"; // xxx
      done("starved"); // xxx
      return;
    }
  }

}


#define	CORPSE_I_TO_C(otyp)	(Char) ((otyp >= DEAD_ACID_BLOB)\
		     ?  'a' + (otyp - DEAD_ACID_BLOB)\
		     :	'@' + (otyp - DEAD_HUMAN))

Char POISONOUS[] = "ADKSVabhks";
Boolean poisonous(obj_t *otmp)
{
  return (NULL != StrChr(POISONOUS, CORPSE_I_TO_C(otmp->otype)));
}

/* returns true if some text was printed */
static Boolean eatcorpse(struct obj *otmp)
{
  Char let = CORPSE_I_TO_C(otmp->otype);
  Boolean tp = false;
  if (let != 'a' && moves > otmp->age + 50 + rund(100)) {
    tp = true;
    message("Ulch -- that meat was tainted!");
    message("You get very sick.");
    Sick = 10 + rund(10);
    you.usick_cause_otype = otmp->otype;// used later to get the name
  } else if (StrChr(POISONOUS, let) && rund(5)) {
    tp = true;
    message("Ecch -- that must have been poisonous!");
    if (!Poison_resistance){
      losestr(rnd(4));
      losehp(rnd(15), "poisonous corpse");
    } else
      message("You don't seem affected by the poison.");
  } else if (StrChr("ELNOPQRUuxz", let) && rund(5)) {
    tp = true;
    message("You feel sick.");
    losehp(rnd(8), "cadaver");
  }
  switch(let) {
  case 'L':
  case 'N':
  case 't':
    Teleportation |= INTRINSIC;
    break;
  case 'W':
    //    pluslvl(); // XXX in potion.c
    break;
  case 'n':
    you.uhp = you.uhpmax;
    flags.botl |= BOTL_HP;
    /* fall into next case */
  case '@':
    message("You cannibal! You will be sorry for this!");
    /* tp != true; */
    /* fall into next case */
  case 'd':
    Aggravate_monster |= INTRINSIC;
    break;
  case 'I':
    if (!Invis) {
      Invis = 50+rund(100);
      if (!See_invisible)
	newsym(you.ux, you.uy);
    } else {
      Invis |= INTRINSIC;
      See_invisible |= INTRINSIC;
    }
    /* fall into next case */
  case 'y': // 'y' did something in Quest.
    /* fall into next case */
  case 'B':
    Confusion = 50;
    break;
  case 'D':
    Fire_resistance |= INTRINSIC;
    break;
  case 'E':
    Telepat |= INTRINSIC;
    break;
  case 'F':
  case 'Y':
    Cold_resistance |= INTRINSIC;
    break;
  case 'k':
  case 's':
    Poison_resistance |= INTRINSIC;
    break;
  case 'c':
    message("You turn to stone.");
    killer = "dead cockatrice"; // XXX
    done("died"); // XXX
    return true;
    break; /* NOTREACHED */
  case 'a':
    if (Stoned) {
      message("What a pity - you just destroyed a future piece of art!");
      tp = true;
      Stoned = 0;
    }
    break;
  case 'M':
    message("You cannot resist the temptation to mimic a treasure chest.");
    /* // some stuff that is not implemented yet
       tp = true;
       nomul(-30);
       afternmv = M_eat_done; // XXX
       nomovemsg = "You now again prefer mimicking a human."; // XXX
       you.usym = '$';
       prme();
    */
    break;
  }
  return tp;

}


////////////////////////////////////////////////////
///was rumors.c

extern Short      *rumors_start; // lock.c
Short n_rumors = 0;
Short n_used_rumors = -1;
UChar *usedbits;
#define used(i) (usedbits[(i)/sizeof(UChar)] & (1 << ((i) % (sizeof(UChar)))))

static void init_rumors()
{
  Short i;
  if (!rumors_start) return;
  n_rumors = rumors_start[0];
  n_used_rumors = 0;
  i = n_rumors/sizeof(UChar);
  usedbits = (UChar *) md_malloc(i+1); // md_malloc will also zero the array.
}

static void show_rumor(Short i)
{
  Short offset;
  Char *p;
  if (i < 0 || i >= n_rumors);
  offset = rumors_start[i+1];
  p = (Char *) rumors_start;
  p += offset;
  message("This cookie has a scrap of paper inside! It reads: ");
  message(p);
  // Some test-messages:
  //message("Any small object that is accidentally dropped will hide under a larger object."); // test to make sure it won't make 2lines into 3lines
  //message("Don't bother about money: only Leprechauns and shopkeepers are interested."); // test to make sure a 3line message isn't truncated
}

static void outrumor()
{
  Short rn,i;
  if (n_rumors <= n_used_rumors) return;
  if (n_used_rumors < 0) init_rumors();
  if (!n_rumors) return;
  rn = rund(n_rumors - n_used_rumors);
  i = 0;
  while (rn || used(i)) {
    // contemplate the i'th rumor
    if (!used(i)) rn--;
    i++;
  }
  usedbits[i/sizeof(UChar)] |= (1 << (i % sizeof(UChar)));
  n_used_rumors++;
  show_rumor(i);
}

