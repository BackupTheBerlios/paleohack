/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
#include "bit.h"
#include "paleohackRsc.h"

/*
   Things to fix when you die (REMEMBER THIS INCLUDES QUITTING)
      - delete all the level records!
      - set level_exists[] to all false!
      - delete the save record!
*/

extern Short msglog_mode;

static Char * ordin(Short n) SEC_5;
Char *killer = NULL;
extern Char plname[PL_NSIZ];


extern Char char_class_names[MAX_CLASS][13];


#define	NAMSZ	8     /* CLEARLY expects a username here.... bah....
                       * I will definitely need to increase that. --sprite */
#define	DTHSZ	40    // eventually 'killer' gets StrNCopied to here.
//#define	PERSMAX	1
#define	POINTSMIN	1	/* must be > 0 */
//#define	ENTRYMAX	100	/* must be >= 10 */
#define	ENTRYMAX	20	/* must be >= 10 */
//#define PERS_IS_UID		/* delete for PERSMAX per name; now per uid */
typedef struct toptenentry {
  //  struct toptenentry *tt_next;
  Long points;
  Short level,maxlvl,hp,maxhp;
  ULong birthdate_is_uid;// use birthdate instead of uid for single-user system
  Char plchar;
  Char sex;
  Char name[NAMSZ+1];
  Char death[DTHSZ+1];
  // Char date[7];	/* yymmdd */   // It would be shorter to store ULong!..
  ULong date; // just call TimGetSeconds(); or whatever.
} topten_t;
topten_t tt;
Short tt_rank = -1;
Short tt_show_after_rank = -1;
Short tt_next_page;

extern Boolean level_exists[MAXLEVEL+1];



static Char * ordin(Short n) SEC_5;
static void calc_bonus_score(Char *st1) SEC_5;
static Short length_pet_names() SEC_5;
static void append_pet_names(Char *buf) SEC_5;
static Short length_gems() SEC_5;
static void append_gems(Char *buf) SEC_5;

static Short draw_topten(Short i) SEC_5;
static void create_tt(topten_t *tt) SEC_5;
static void print_tt_head(Short *y) SEC_5;
static Boolean print_tt_rec(topten_t *tt_rec, Short rank, Short *y, Boolean bold) SEC_5;
static void maybe_insert_tt(topten_t *tt_rec, Char *buf) SEC_5;

void unsave()
{
  Short i;
  // Remove the character record and all the level records!
    i = DmNumRecords(phSaveDB);
    //    StrPrintF(ScratchBuffer, "%d %d", i, REC_SAVECHAR);
    //    WinDrawChars(ScratchBuffer, StrLen(ScratchBuffer), 10, 10);
  while (DmNumRecords(phSaveDB) > REC_SAVECHAR) {
    DmRemoveRecord(phSaveDB, REC_SAVECHAR);
    i = DmNumRecords(phSaveDB);
    //    StrPrintF(ScratchBuffer, "%d", i);
    //    WinDrawChars(ScratchBuffer, StrLen(ScratchBuffer), 10, 60-11*i);
  }
  for (i = 0 ; i < MAXLEVEL+1 ; i++)
    level_exists[i] = false;  
  // There are probably some other things I should clear too.
  // Better remember collect them as they occur to me.
}




void done_in_by(monst_t *mtmp)
{
  static Char buf[BUFSZ];
  Char *mcall = mtmp->name;
  Char *gcall = (Char *) (mtmp->extra);
  Char *mname = mon_names + mtmp->data->mname_offset;
  message("You die ...");
  if (mtmp->data->mlet == ' ') {
    StrPrintF(buf, "the ghost of %s", (gcall && gcall[0]) ? gcall : "nobody");
    killer = buf;
  } else if (mcall && mcall[0]) {
    StrPrintF(buf, "%s called %s", mname, mcall);
    killer = buf;
  } else if (mtmp->bitflags & M_IS_INVISIBLE) {
    StrPrintF(buf, "invisible %s", mname);
    killer = buf;
  } else killer = mname;
  done("died");
}

static Char * ordin(Short n)
{
  Short d = n%10;
  if (n >= 11 && n <= 13) d = 0; // "th".
  switch(d) {
  case 1:  return "st";
  case 2:  return "nd";
  case 3:  return "rd";
  default: return "th";
  }
}


/* called with arg "died", "drowned", "escaped", "quit", "choked", "panicked",
   "burned", "starved" or "tricked" */
static Char *st1_keep_during_rip;
static Boolean need_rip;
void done(Char *st1)
{
  Boolean leftform = false;
  Word curfrm = FrmGetActiveFormID();

  if (you.dead) return; // hm, we've already been here
  need_rip = false;
#ifdef WIZARD
  if (wizard && *st1 == 'd') { // (died, drowned)
    you.uswallowedtime = 0;
    if (you.uhpmax < 0) you.uhpmax = 100;	/* arbitrary */
    you.uhp = you.uhpmax;
    message("For some reason you are still alive.");
    flags.move = 0;
    if (multi > 0) multi = 0; else multi = -1;
    flags.botl = 1;
    return;
  }
#endif WIZARD
  you.dead = true; // in case player exits before creating a new character.

  if (*st1 == 'q' && you.uhp < 1) {
    st1 = "died";
    killer = "quit while already on Charon's boat";
  }
  if (*st1 == 's') killer = "starvation"; else
    if (*st1 == 'd' && st1[1] == 'r') killer = "drowning"; else
      if (*st1 == 'p') killer = "panic"; else
	if (*st1 == 't') killer = "trickery"; else
	  if (!StrChr("bcd", *st1)) killer = st1; // (escaped, choked)
  paybill();
  unsave(); // ==   clearlocks();

  // Maybe I need to put wait_for_event in a while() LOOP that checks
  // whether there are any more messages left to display...
  if (curfrm != InvForm &&
      curfrm != InvMsgForm &&
      curfrm != InvActionForm) {
    while (message_clear(false)) {
      show_messages();
      wait_for_event(); //  if (flags.toplin == 1) more();
    }
  }

  if (StrChr("bcds", *st1)) { // XXXX remove true when done testing!
#ifdef WIZARD
    if (!wizard)
#endif WIZARD
      savebones();

    if (true) {
      /* if (!flags.notombstone)
	 outrip(); */
      leftform = true;
      need_rip = true;
    }
  }
  st1_keep_during_rip = st1;
  
  //  LeaveForm(); // leave MainForm... hm this causes crash sometimes?
  // hmmmm I hope we can't die in some other form.  like, inventory.
  FrmGotoForm(TombstoneForm);
  // If we're NOT need_rip,
  // it should proceed directly to done_postRIP.
  // otherwise it should draw tombstone first
}

#define X_CTR 80
#define LINE_H 11
#define SCR_H 160
#define SCR_W 160
// New!  Randomly generated grass.  Because I'm just that crazy.
Char old_stems[] = ")/\\\\_//(\\/(/\\)/\\//\\/|_)";
//#define RANDOM_STEM /* Uncomment this to have random flowers. */
#define STEMS 5
Char stem[] = "/\\)(_X";
Int8 stem_prob[STEMS] = { 6, 6, 3, 2, 2};
Char *rip_txt[3] = {"REST", "IN", "PEACE"};
#define TOMB_LINES 13
void draw_tombstone()
{
  Char buf[BUFSZ], c;
  Short y0, w, w0, i, x, y;
  y0 = (SCR_H - (TOMB_LINES+1) * LINE_H)/2; // Center tombstone vertically
  if (y0 < 0) y0 = 0;

  // Now draw the tombstone
  y = y0;
  w = FntCharWidth('_'); // I want 10 of these, 4 pixels, 1/4 of screen.
  for (x = 0 ; x < SCR_W/4 ; x += w)
    WinDrawChars("_", 1, (3*SCR_W)/8 + x, y);
  w = FntCharWidth('/'); // Let's assume '/' and '\' are the same width.
  y += LINE_H;
 // note, I'd better check the x calculations visually...
  for (i = 0 ; i < 5 ; i++, y += LINE_H) {
    WinDrawChars("/", 1, (3*SCR_W)/8 - (i+1)*w, y);
    WinDrawChars("\\", 1, (5*SCR_W)/8 + i*w, y);
  }
  x = (3*SCR_W)/8 - 5*w;
  for (i = 0 ; i < 7 ; i++, y += LINE_H) {
    WinDrawChars("|", 1, x, y);
    WinDrawChars("|", 1, SCR_W-x, y);
  }
  // Ok, the outline of tombstone is drawn.  


  // Draw the stems on the bottom line, and flowers above.
#ifdef RANDOM_STEM
  x = SCR_W/8;
  w = (7*SCR_W)/8;
  {
    Short tmp;
    for (i = 0, tmp = 0 ; i < STEMS ; i++) tmp += stem_prob[i];
  }
#else
  w = FntCharsWidth(old_stems, StrLen(old_stems) - 3);
  x = (SCR_W-x) - w; // Make the '|' (on the right) line up!
#endif
  y = y0 + TOMB_LINES * LINE_H;
  w0 = (FntCharWidth('(') - FntCharWidth('*')) / 2; // heh, about a pixel.
#ifdef RANDOM_STEM
  for ( ; x < w ; ) {
    Short k = 0, j = rund(tmp);
    while (k < STEMS && j >= stem_prob[k]) j -= stem_prob[k++];
    c = stem[k];
#else
  for (i = 0 ; i < StrLen(old_stems) ; i++) {
    c = old_stems[i];
#endif
    if (c == '(' || c == ')')
      WinDrawChars("*", 1, x+w0, y - (3*LINE_H)/4);
    WinDrawChars(&c, 1, x, y);
    x += FntCharWidth(c);
  }
  StrPrintF(buf, "%d", getyear());
  WinDrawChars_ctr(buf, StrLen(buf), SCR_W/2, y-2*LINE_H);



  // Ok now add text.....
  //  'x' is still the place where we drew the left '|'...
  // calculate how much space we have available to draw the text in.
  w = SCR_W - 2*(x + FntCharWidth('|') + FntCharWidth(' '));
  // If the death string is more than that, be sure to wrap it.
  y = y0 + 2 * LINE_H;
  for (i = 0 ; i < 3 ; i++, y += LINE_H)
    WinDrawChars_ctr(rip_txt[i], StrLen(rip_txt[i]), SCR_W/2, y-LINE_H/2);
  y += LINE_H;
  WinDrawChars_ctr(plname, StrLen(plname), SCR_W/2, y);
  y += LINE_H;
  StrPrintF(buf, "%ld Au", you.ugold);
  WinDrawChars_ctr(buf, StrLen(buf), SCR_W/2, y);
  y += LINE_H;
  if (!killer) killer = "programmer error"; // this should never happen..
  // Print the death string on the next two lines.
  if (0==StrNCompare(killer, "the ", 4) || 0==StrCompare(killer, "starvation"))
    StrPrintF(buf, "killed by");
  else {
    Char c = *killer;
    StrPrintF(buf, "killed by a");
    if (StrChr("aeiou", c))
      StrPrintF(buf+StrLen(buf), "n");
  }
  WinDrawChars_ctr(buf, StrLen(buf), SCR_W/2, y);
  y += LINE_H;

  // w is still the available width.
  // I wonder whether two lines will be enough.
  StrCopy(buf, killer);
  {
    Short len_str, len_line;
    Char *msg = buf;
    len_str = StrLen(msg);
    len_line = FntWordWrap(msg, w);
    if (len_line < len_str) {
      WinDrawChars_ctr(msg, len_line, SCR_W/2, y);
      y += LINE_H;
      msg += len_line;
    }
    WinDrawChars_ctr(msg, StrLen(msg), SCR_W/2, y);
    y += LINE_H;
  }
}


static void calc_bonus_score(Char *st1)
{
  Long tmp;
  tmp = you.ugold - you.ugold0; // how much you made during the game
  if (tmp < 0)
    tmp = 0;
  if (*st1 == 'd' || *st1 == 'b') // (died, drowned, burned)
    tmp -= tmp/10;
  you.urexp += tmp;
  you.urexp += 50 * maxdlevel;
  if (maxdlevel > 20)
    you.urexp += 1000*((maxdlevel > 30) ? 10 : maxdlevel - 20);
}

static Short length_pet_names()
{
  monst_t *mtmp;
  Short len = 0;
  keepdogs();
  mtmp = mydogs;
  while (mtmp) {
    len += 5 + StrLen(monnam(mtmp)); // 5 = " and ".
    mtmp = mtmp->nmon;
  }
  return len;
}
static void append_pet_names(Char *buf)
{
  monst_t *mtmp;
  //  keepdogs(); // Already called, in length_pet_names.
  mtmp = mydogs; // mtmp may be NULL, if you don't have any pets..
  while (mtmp) {
    StrPrintF(buf+StrLen(buf), " and %s", monnam(mtmp));
    if (mtmp->bitflags & M_IS_TAME)
      you.urexp += mtmp->mhp;
    mtmp = mtmp->nmon;
  }
}

static Short length_gems()
{
  Short len = 0;
  UInt worthlessct = 0;
  obj_t *otmp;
  Short i;
  for (otmp = invent ; otmp ; otmp = otmp->nobj) {
    if (otmp->olet == GEM_SYM) {
      BITSET(oc_name_known, otmp->otype);
      i = otmp->quantity * objects[otmp->otype].g_val;
      if (i == 0) {
	worthlessct += otmp->quantity;
	continue;
      }
    } else if (otmp->olet == AMULET_SYM) {
      otmp->bitflags |= O_IS_KNOWN;
      i = (otmp->spe < 0) ? 2 : 5000;
    } else continue;
    StrPrintF(ScratchBuffer, "   (worth %d Zorkmids),\n");
    len += StrLen(ScratchBuffer);
    len += StrLen(doname(otmp));
  }
  if (worthlessct) {
    StrPrintF(ScratchBuffer, "  %u worthless pieces of coloured glass,\n",
	      worthlessct);
    len += StrLen(ScratchBuffer);
  }
  return len;
}
static void append_gems(Char *buf)
{
  Boolean zorkmids = true, has_amulet = false;
  UInt worthlessct = 0;
  obj_t *otmp;
  Short i;
  StrPrintF(buf+StrLen(buf), "\n");
  for (otmp = invent; otmp; otmp = otmp->nobj) {
    if (otmp->olet == GEM_SYM) {
      //      BITSET(oc_name_known, otmp->otype); // Already called above
      i = otmp->quantity * objects[otmp->otype].g_val;
      if (i == 0) {
	worthlessct += otmp->quantity;
	continue;
      }
      you.urexp += i;
      StrPrintF(buf+StrLen(buf), "  %s (worth %d %s),\n",
	     doname(otmp), i, zorkmids?"Zorkmids":"Zd");
      zorkmids = false;
    } else if (otmp->olet == AMULET_SYM) {
      //      otmp->bitflags |= O_IS_KNOWN; // Already called above
      i = (otmp->spe < 0) ? 2 : 5000;
      you.urexp += i;
      StrPrintF(buf+StrLen(buf), "  %s (worth %d %s),\n",
	     doname(otmp), i, zorkmids?"Zorkmids":"Zd");
      if (otmp->spe >= 0) {
	has_amulet = true;
	killer = "escaped (with amulet)";
      }
    }
  }
  if (worthlessct)
    StrPrintF(buf+StrLen(buf), "  %u worthless piece%sof coloured glass,\n",
	   worthlessct, (worthlessct==1) ? " " : "s ");
  if (has_amulet) you.urexp *= 2;
}


// Caller must allocate a honking big buffer for buf.
// How much space will you need for buf:
// length_pet_names() + length(gems() + FOO
// where FOO is 243 characters... better call it 250...
// "Goodbye Speleologist 01234567890123456789012345678901...  "
// "You panicked on dungeon level 99 with 0123456789 points, "
// "and 0123456789 pieces of gold, after 0123456789 moves "
// "You were level 255 with a maximum of 65000 hit points when you panicked. "
Short done_postRIP_size()
{
  Short i = 250;
  i += length_pet_names();
  i += length_gems();
  return i;
}
void done_postRIP(Char *buf)
{
  Char *st1 = st1_keep_during_rip;

  if (*st1 == 'c') killer = st1;		/* after outrip() */

  if (!buf) return; // XXX Someone screwed up in a big way..
  StrPrintF(buf, "Goodbye %s %s...\n\n",
	    char_class_names[you.character_class], plname);
  // Calculate a bonus to your final score
  calc_bonus_score(st1);
  // Print different things depending on how you ended the game.
  if (*st1 == 'e') {
    killer = st1;
    StrPrintF(buf+StrLen(buf), "You");
    append_pet_names(buf);
    StrPrintF(buf+StrLen(buf), " escaped from the dungeon with %ld points, ",
	      you.urexp);
    append_gems(buf); // Also checks for amulet and increases your score.
  } else {
    StrPrintF(buf+StrLen(buf), "You %s on dungeon level %d with %ld points, ",
	      st1, dlevel, you.urexp);
  }
  StrPrintF(buf+StrLen(buf), "and %ld piece%sof gold,\nafter %ld move%s\n",
	    you.ugold, (you.ugold==1)?" ":"s ", moves, (moves==1)?".":"s.");
  StrPrintF(buf+StrLen(buf),
	    "You were level %u with a maximum of %d hit points when you %s.\n",
	    you.ulevel, you.uhpmax, st1);
  // What to do when the user hits the "Next screen" button:
  //  if (*st1 == 'e') {
  //    // Wait for user to hit a key before going on
  //    getret();	/* all those pieces of coloured glass ... */
  //    cls();
  //  }
  //#ifdef WIZARD
  //  if (!wizard)
  //#endif WIZARD
  //    topten();
  //  exit(0);

  
  // mess around with the database
  create_tt(&tt);
  tt_rank = tt_show_after_rank = -1;
  maybe_insert_tt(&tt, buf);
  
}



static void transition_to_topten()
{
  if (need_rip) {
    RectangleType r;
    RctSetRectangle(&r, 0, 0, SCR_W, SCR_H);
    WinEraseRectangle(&r, 0);
  }
  need_rip = false;
  // continue to the summary form.  when it's closed, we'll call draw_topten.
  msglog_mode = SHOW_DEAD;
  FrmPopupForm(MsgLogForm);
}




void init_topten()
{
  //  Short i;
  /*
  i = (tt_rank ? tt_rank-2 : tt_show_after_rank-1);
  draw_topten(i - 2); // we'll show 3 entries before yours.
  */
  tt_next_page = draw_topten(0);
}

static Short draw_topten(Short i)
{
  Short rec_i, rank;
  Short y = 0; //, fits, maxtop;
  VoidHand vh;
  topten_t *t1;
  RectangleType r;

  RctSetRectangle(&r, 0, 0, SCR_W, SCR_H);
  WinEraseRectangle(&r, 0);

  // tt_rank should be 0 or a one-based rank now.  (after maybe_insert_tt().)
  // if 0, then tt_show_after_rank should be a one-based rank now.
  // (otherwise, tt_show_after_rank is undefined.)

  print_tt_head(&y);

  if (i < 0) i = 0;
  // Scores can take 2 or 3 lines.  let's assume 3 lines.
  //  fits = (SCR_H - (LINE_H + 1)) / 3*LINE_H; // at least this many will fit.
  //  maxtop = DmNumRecords(phScoreDB) - fits;
  //  if (maxtop >= 0 && i > maxtop) i = maxtop;

  for (rec_i = i ; rec_i < DmNumRecords(phScoreDB) ; rec_i++) {
    vh = DmQueryRecord(phScoreDB, rec_i);
    t1 = (topten_t *) MemHandleLock(vh);
    rank = rec_i + 1;
    if (! print_tt_rec(t1, rank, &y, (rank == tt_rank)) ) {
      MemHandleUnlock(vh);
      return (rec_i - 1);
    }
    if (!tt_rank && rank == tt_show_after_rank)
      if (! print_tt_rec(&tt, 0, &y, true)) {
	MemHandleUnlock(vh);
	return rec_i;
      }
    MemHandleUnlock(vh);
    //    if (y > SCR_H - 2*LINE_H) break;
  }
  return rec_i;

}

static void create_tt(topten_t *tt)
{
  tt->points = you.urexp;
  tt->level = dlevel;
  tt->maxlvl = maxdlevel;
  tt->hp = (you.uhp < 0 ? 0 : you.uhp);
  tt->maxhp = you.uhpmax;
  tt->birthdate_is_uid = you.birthdate;
  tt->plchar = char_class_names[you.character_class][0];
  tt->sex = (my_prefs.is_male ? 'M' : 'F');
  tt->date = TimGetSeconds();
  StrNCopy(tt->name, plname, NAMSZ);
  tt->name[NAMSZ] = '\0';
  StrNCopy(tt->death, killer, DTHSZ);
  tt->death[DTHSZ] = '\0'; // "the Wizard of Yendor"?
  /* assure minimum number of points */
  if (tt->points < POINTSMIN)
    tt->points = 0;
}

// This will set tt_rank to 0 (not_on_list) or n (n'th on list, one-based.)
// If it sets tt_rank to 0, it will also set tt_show_after_rank to the
// (one-based) rank after which this unlisted entry should be displayed.
static void maybe_insert_tt(topten_t *tt_rec, Char *buf)
{
  VoidHand vh;
  topten_t *t1;
  Short rec_i;
  Long tmp_score = 0;
  /* tt_rank: -1 undefined, 0 not_on_list, n n_th on list  (n is one-based) */
  tt_rank = -1;
  for (rec_i = 0 ; rec_i < DmNumRecords(phScoreDB) ; rec_i++) {
    Boolean dupe = false;
    vh = DmQueryRecord(phScoreDB, rec_i);
    t1 = (topten_t *) MemHandleLock(vh);
   if (tt_rank < 0 && t1->points < tt.points)
      tt_rank = rec_i + 1; // insert 'before' t1... (but rank is one-based.)
    // if 'duplicate':
    //   if tt_rank is on_list, delete duplicate,
    //   else set tt_rank to not_on_list and set tt_show_after_rank to current.
    if ((t1->birthdate_is_uid == tt.birthdate_is_uid)
#ifndef PERS_IS_UID
	|| (0 == StrNCompare(t1->name, tt.name, NAMSZ) &&
	    t1->plchar == tt.plchar /*&& --occ_cnt <= 0*/ )
#endif PERS_IS_UID
	) {
      dupe = true;
      tmp_score = t1->points;
    }
    MemHandleUnlock(vh);
    if (dupe) {
      if (tt_rank > 0) {
	DmRemoveRecord(phScoreDB, rec_i);
	rec_i--; // don't skip the next entry!
      } else {
	tt_rank = 0;
	tt_show_after_rank = rec_i + 1; // rank is one-based.
	if (buf)
	  StrPrintF(buf+StrLen(buf),
		    "\n\nYou didn't beat your previous score of %ld points.",
		    tmp_score);
      }
    } // end 'if dupe'
  }
  // tt_rank should now be defined unless you have the lowest score, so..
  if (tt_rank < 0) {
    if (DmNumRecords(phScoreDB) < ENTRYMAX)
      // there's still room for you
      tt_rank = DmNumRecords(phScoreDB) + 1;
    else {
      tt_rank = 0;
      tt_show_after_rank = DmNumRecords(phScoreDB);
    }
  }
  // Ok!  If we decided to insert the record, better do that now.
  if (tt_rank > 0) {
    Short new_rec_i = tt_rank - 1;
    VoidPtr p;
    vh = DmNewRecord(phScoreDB, &new_rec_i, sizeof(topten_t));
    p = MemHandleLock(vh);
    DmWrite(p, 0, &tt, sizeof(topten_t));
    MemHandleUnlock(vh);
    DmReleaseRecord(phScoreDB, new_rec_i, true);
    if (buf) {
      if (tt_rank <= 10)
	StrPrintF(buf+StrLen(buf), "\n\nYou made the top ten list!");
      else
	StrPrintF(buf+StrLen(buf),
		  "\n\nYou reached the %d%s place on the top %d list.",
		  tt_rank, ordin(tt_rank), ENTRYMAX);
    }
  }
  // Hey, I also need to delete the nth record if n exceeds the limit.
  // ('cause there might NOT have been a duplicate to delete.)
  while (DmNumRecords(phScoreDB) >= ENTRYMAX) {
    DmRemoveRecord(phScoreDB, ENTRYMAX-1);
  }

}

static void print_tt_head(Short *yp)
{
  Short x = 0;
  Short w_digit, w_space, w_bracket;
  w_digit = FntCharWidth('0');
  w_space = FntCharWidth(' ');
  w_bracket = FntCharWidth('[');
  //  StrPrintF(ScratchBuffer, "#     Points   Name               Hp [max]");
  //  WinDrawChars(ScratchBuffer, StrLen(ScratchBuffer), x, y);
  WinDrawChars("#", 1, x, *yp);
  /*
  x += 3*w_digit + 2*w_space;
  WinDrawChars("Points", 6, x, *yp);
  x += 6*w_digit + 2*w_space;
  */
  x += 2*w_digit + 2*w_space + 6*w_digit;
  WinDrawChars_rj("Points", 6, x, *yp);
  x += 2*w_space;
  WinDrawChars("Name", 4, x, *yp);
  /* // Uncomment this if I decide to print HP in print_tt_rec....
  x = 160;
  WinDrawChars_rj("Hp [max]", 8, x, *yp);
  */
  *yp += LINE_H;
  WinDrawGrayLine(0, *yp, 160, *yp);
  *yp += 1;
}

// This will use 2 or 3 lines.
static Boolean print_tt_rec(topten_t *t1, Short rank, Short *yp, Boolean bold)
{
  Short x = 0, y0 = *yp;
  Short w_digit, w_space, w_bracket;
  Boolean quit = false, starv = false, killed = false;

  if (*yp >= SCR_H - LINE_H) return false;
  w_digit = FntCharWidth('0');
  w_space = FntCharWidth(' ');
  w_bracket = FntCharWidth('[');

  //  StrPrintF(ScratchBuffer, "001 000123 sprite-W");
  x = 2*w_digit; // 3*w_digit;
  if (rank) {
    StrPrintF(ScratchBuffer, "%d", rank); // Rank
    WinDrawChars_rj(ScratchBuffer, StrLen(ScratchBuffer), x, *yp);
  }
  x += 2*w_space + 6*w_digit;
  StrPrintF(ScratchBuffer, "%ld", t1->points); // Score
  WinDrawChars_rj(ScratchBuffer, StrLen(ScratchBuffer), x, *yp);
  x += 2*w_space;
  StrPrintF(ScratchBuffer, "%s-%c", t1->name, t1->plchar);
  WinDrawChars(ScratchBuffer, StrLen(ScratchBuffer), x, *yp);
  *yp += LINE_H;
  if (*yp >= SCR_H - LINE_H) return false;
  // Now it gets messy.
  if (!StrNCompare("escaped", t1->death, 7)) {
    if (!StrCompare(" (with amulet)", t1->death+7))
      StrPrintF(ScratchBuffer, "escaped the dungeon with amulet");
    else
      StrPrintF(ScratchBuffer, "escaped the dungeon [max level %d]",
	      t1->maxlvl);
  } else {
    if (!StrNCompare(t1->death, "quit", 4)) {
      quit = true;
      if (t1->maxhp < 3*t1->hp && t1->maxlvl < 4)
	StrPrintF(ScratchBuffer, "cravenly gave up");
      else
	StrPrintF(ScratchBuffer, "quit");
    } else if (!StrCompare(t1->death, "choked")) {
      StrPrintF(ScratchBuffer, "choked on %s food",
	      (t1->sex == 'F') ? "her" : "his");
    } else if (!StrNCompare(t1->death, "starv", 5)) {
      StrPrintF(ScratchBuffer, "starved to death");
      starv = true;
    } else {
      StrPrintF(ScratchBuffer, "was killed");
      killed = true;
    }
    StrPrintF(ScratchBuffer+StrLen(ScratchBuffer), " on%slevel %d",
	      (killed || starv) ? " " : " dungeon ", t1->level);
    if (t1->maxlvl != t1->level)
      StrPrintF(ScratchBuffer+StrLen(ScratchBuffer), " [max %d]", t1->maxlvl);
    if (quit && t1->death[4])
      StrPrintF(ScratchBuffer+StrLen(ScratchBuffer), t1->death + 4); // hmmmm?
  }
  if (killed)
    StrPrintF(ScratchBuffer+StrLen(ScratchBuffer), " by%s%s",
	      ( (!StrNCompare(t1->death, "trick", 5) ||
		 !StrNCompare(t1->death, "the ", 4))
		? " " :
		StrChr("aeiou", t1->death[0]) ? " an " : " a "),
	      t1->death);
  StrPrintF(ScratchBuffer+StrLen(ScratchBuffer), ".");
  // have to figure out where to wrap the darned thing.
  x = 0;
  if (FntCharsWidth(ScratchBuffer, StrLen(ScratchBuffer)) <= 160) {
    WinDrawChars(ScratchBuffer, StrLen(ScratchBuffer), x, *yp);    
    //    *yp += LINE_H;    // Nah.. we can save a line..
  } else {
    Short len = FntWordWrap(ScratchBuffer, 160);// fits on 1st line.
    WinDrawChars(ScratchBuffer, len, x, *yp);
    *yp += LINE_H;
    if (*yp >= SCR_H - LINE_H) return false;
    WinDrawChars(ScratchBuffer+len, StrLen(ScratchBuffer)-len, x, *yp);
  }

  // Finally, print hp / maxhp.
  /*
  x = 160;
  StrPrintF(ScratchBuffer, "[%d]", t1->maxhp); // max hp
  WinDrawChars_rj(ScratchBuffer, StrLen(ScratchBuffer), x, *yp);
  x -= (3*w_digit + 2*w_bracket + 2*w_space);
  if (t1->hp)
    StrPrintF(ScratchBuffer, "%d", t1->hp); // or make it "-" if <= 0 ...
  else
    StrPrintF(ScratchBuffer, "-");
  WinDrawChars_rj(ScratchBuffer, StrLen(ScratchBuffer), x, *yp);
  */
  *yp += LINE_H;

  if (bold) {
    RectangleType r;
    RctSetRectangle(&r, 0, y0, SCR_W, *yp - y0);
    WinInvertRectangle(&r, 0);
  }
  return true;
}


// First, show the tombstone.

Boolean Tombstone_Form_HandleEvent(EventPtr e)
{
  Boolean handled = false;
  FormPtr frm;
    
  switch (e->eType) {

  case frmOpenEvent:
    frm = FrmGetActiveForm();
    FrmDrawForm(frm);
    if (!you.dead) {
      need_rip = false;
      tt_next_page = draw_topten(0);
    } else {
      if (need_rip)
	draw_tombstone();
      else
	transition_to_topten();
    }
    handled = true;
    break;

  case keyDownEvent: case penDownEvent:
    if (need_rip)
      transition_to_topten(); // msglog form will call draw_topten on exit.
    else {
      if (tt_next_page >= DmNumRecords(phScoreDB)) {
	LeaveForm();
	if (you.dead)
	  FrmGotoForm(Chargen1Form); // xxx need to change this.  (I do?)
	// else you're just viewing it from the dungeon menu.
      } else {
	tt_next_page = draw_topten(tt_next_page);
      }
    }
    handled = true;
    break;

  default:
    break;
  }
  return handled;
}
