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
//extern const Char vowels[6];

extern Char char_class_names[MAX_CLASS][13];


#define	NAMSZ	8     /* CLEARLY expects a username here.... bah....
                       * I will definitely need to increase that. --sprite */
#define	DTHSZ	40    // eventually 'killer' gets StrNCopied to here.
#define	PERSMAX	1
#define	POINTSMIN	1	/* must be > 0 */
#define	ENTRYMAX	100	/* must be >= 10 */
#define PERS_IS_UID		/* delete for PERSMAX per name; now per uid */
typedef struct toptenentry {
  struct toptenentry *tt_next;
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
topten_t *tt_head;

extern Boolean level_exists[MAXLEVEL+1];



static Char * ordin(Short n) SEC_5;
static void calc_bonus_score(Char *st1) SEC_5;
static Short length_pet_names() SEC_5;
static void append_pet_names(Char *buf) SEC_5;
static Short length_gems() SEC_5;
static void append_gems(Char *buf) SEC_5;

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
  Char *mname = mon_names + mtmp->data->mname_offset;
  message("You die ...");
  if (mtmp->data->mlet == ' ') {
    StrPrintF(buf, "the ghost of %s", (mcall && mcall[0]) ? mcall : "nobody");
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
  need_rip = false;
  if (you.dead) return; // hm, we've already been here
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
  while (message_clear(false)) {
    show_messages();
    wait_for_event(); //  if (flags.toplin == 1) more();
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
//Char stems[] = ")/\\\\_//(\\/(/\\)/\\//\\/|_)";
#define STEMS 5
Char stem[] = "/\\)(_X";
Int8 stem_prob[STEMS] = { 6, 6, 3, 2, 2};
Char *rip_txt[3] = {"REST", "IN", "PEACE"};
#define TOMB_LINES 13
void draw_tombstone()
{
  Char buf[BUFSZ];
  Short y0, w, w0, i, x, y, tmp;
  y0 = (SCR_H - (TOMB_LINES+1) * LINE_H)/2; // Center tombstone vertically
  if (y0 < 0) y0 = 0;

  // First draw the stems on the bottom line, and flowers above.
  x = SCR_W/8;
  w = (7*SCR_W)/8;
  y = y0 + TOMB_LINES * LINE_H;
  w0 = (FntCharWidth('(') - FntCharWidth('*')) / 2; // heh, about a pixel.
  for (i = 0, tmp = 0 ; i < STEMS ; i++) tmp += stem_prob[i];
  for ( ; x < w ; ) {
    Short k = 0, j = rund(tmp);
    while (k < STEMS && j >= stem_prob[k]) j -= stem_prob[k++];
    if (stem[k] == '(' || stem[k] == ')')
      WinDrawChars("*", 1, x+w0, y - (3*LINE_H)/4);
    WinDrawChars(&(stem[k]), 1, x, y);
    x += FntCharWidth(stem[k]);
  }
  StrPrintF(buf, "2002"); // XXXXX fix this to be the "real" year
  WinDrawChars_ctr(buf, StrLen(buf), SCR_W/2, y-2*LINE_H);
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

void draw_topten()
{
  level_message("no score list yet -- hit any key to go on");  
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
    if (need_rip)
      draw_tombstone();
    else
      transition_to_topten();
    handled = true;
    break;

  case keyDownEvent: case penDownEvent:
    if (need_rip)
      transition_to_topten(); // msglog form will call draw_topten on exit.
    else {
      LeaveForm();
      FrmGotoForm(Chargen1Form); // xxx need to change this.  (I do?)
    }
    handled = true;
    break;

  default:
    break;
  }
  return handled;
}
