/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"


// Globals: some of these will need saving, some won't.
// Also, make sure that any that need reinitializing on quit/restart are
// getting reinitialized properly.

// These are in headers
// Anything marked CONST is already in the const database.
// Anything marked REC is already in the save database
// Anything marked SCRATCH isn't saved and doesn't need saving
// Anything marked OK currently gets saved and restored
// (some other things might also be scratch that aren't marked yet.)
// Ok
// Now consult the old save.c in the original hack, see what it saves.

/*
extern Boolean IsVGA;                       // scratch
extern Char *mon_names;                     // CONST
extern Char *oc_descrs;                     // CONST
extern Char *oc_names;                      // CONST
extern Char ScratchBuffer[BIGBUF];          // scratch
extern DmOpenRef       phDB;                // scratch
extern DmOpenRef       phSaveDB;            // scratch
extern FontID BigFont;                      // scratch
extern FontID SmallFont, MyFont;            // scratch
extern FontPtr oldFontSix;                  // scratch
extern Int8 dlevel;                         // OK
extern Long moves;                          // OK
extern Short *oc_descr_offset;              // REC
extern UChar floor_info[DCOLS][DROWS];      // OK
extern UChar floor_symbol[DCOLS][DROWS];    // OK
extern UChar oc_name_known[MAX_OC_NK];      // OK
extern gold_t  * fgold;                     // OK
extern monst_t * fallen_down;               // OK
extern monst_t * fmon;                      // OK
extern monst_t * mydogs;
extern obj_t   * fcobj;                     // OK
extern obj_t   * fobj;                      // OK
extern obj_t   * invent;                    // OK
extern obj_t   * uright, * uleft,
               * uchain, * uball;           // RECONSTRUCTED
extern obj_t   * uwep, * uarm, * uarm2,
               * uarmh, * uarms, * uarmg;
extern objclass_t *objects;                 // CONST
extern permonst_t *mons;                    // CONST
extern struct flag flags;                   // OK
extern trap_t  * ftrap;                     // OK
extern you_t you;                           // OK


// These are not in headers (yet) but appear as extern decls in .c files

extern Boolean came_from_generation;        // scratch
extern Boolean draw_directional_p;          // scratch
extern Boolean in_mklev;                    // scratch
*/
extern Boolean level_exists[MAXLEVEL+1];    // seems like I oughta save it..
/*
extern Boolean stop_occupation_now;         // scratch
extern Boolean undraw_directional_p;        // scratch
extern Char *old_messages[SAVED_MSGS];      // scratch
*/
extern Char fut_geno[];                     // OK
extern Char genocided[];                    // OK
/*
extern Char plname[PL_NSIZ];                            // XXXX NEEDS SAVING?
extern const Char shtypes[];                // N/A
extern Int8 engrave_type;                   // scratch
extern Int8 maxdlevel;                      // OK
extern PointType doors[MAX_DOORS];          // OK
extern Short      *rumors_start;            // CONST
extern Short *shknam_start;                 // CONST
extern Short *trobj_start;                  // CONST
*/
extern Short bases[];                       // OK
/*
extern Short engrave_or_what;               // scratch
extern Short msglog_mode;                   // scratch
extern Short multi;
extern Short nroom;
extern Short xdnstair, xupstair,            // OK
             ydnstair, yupstair;            // OK
extern UChar sense_what;                    // scratch
extern const Char *traps[TRAPNUM];          // N/A
extern const Char mlarge[];                 // N/A
extern const Int8 xdir[8], ydir[8];         // N/A
extern obj_t *sense_by_what;                // scratch
extern previous_state curr_state;           // scratch
extern room_t rooms[MAX_ROOMS];             // OK
extern wseg_t *m_atseg;
extern wseg_t *wsegs[MAX_WORM];                          // (needs saving)


// Things that AREN'T extern anywhere but need saving:
extern obj_t *billobjs;                     // OK
*/


/////////////////////////////////////////////////////////////////
Boolean dosave() // was dosave0
{
  Short offset = 0;
  VoidPtr p;
  VoidHand vh;
  UShort valid = 1, stamp = 0;
  //  Short y = 10;
  ULong rec_size = 0;
  UInt rec_i, max_rec;
  Short *recp, i;
  Boolean replace = false;

  level_message("Be seeing you ...");

  if (flags.moon_phase == FULL_MOON)	/* ut-sally!fletcher */
    you.uluck--;			/* and unido!ab */
  // "savelev" will write to a separate record!
  savelev(dlevel, true);

  rec_size += 2*sizeof(Int8) + 2*sizeof(UShort);
  rec_size += sizeof(you_t);
  rec_size += MAX_OC_NK * sizeof(UChar);
  rec_size += (NUM_OBJ_SYMBOLS+1) * sizeof(Short);
  rec_size += (MAXLEVEL+((MAXLEVEL & 0x1)?1:2)) * sizeof(Boolean);//EVEN
  rec_size +=  saveobjchn_size(invent);
  rec_size += saveobjchn_size(fcobj);
  rec_size += savemonchn_size(fallen_down);
  rec_size += sizeof(flags_t) + sizeof(Long);  
  if (you.ustuck) rec_size += sizeof(UShort);
  rec_size += 2 * MAX_GENOCIDE * sizeof(Char);
  rec_size += 4 * 3; // all of the "save_tag" nuggets
  // no need for old "savenames" since oc_unames already lives in database.
  
  // sample values:
  // ( 2 inv=276 fcobj=278 fallen_down=280 flags+you=524 !stuck=524 gen=644 )
  // ( 2     242       244             246           490        490     610 )
  

  // Figure out where to insert this record.  Delete one if this replaces it.
  max_rec = DmNumRecords(phSaveDB);
  for (i = REC_SAVECHAR, rec_i = 0 ; (i < max_rec) && !rec_i ; i++) {
    vh = DmQueryRecord(phSaveDB, i);
    recp = (Short *) MemHandleLock(vh);
    if (recp) {
      if (recp[0] >= stamp) // we'll insert new record at this index
	rec_i = i;
      if (recp[0] == stamp) // need to delete this record first
	replace = true;
    }
    MemHandleUnlock(vh);
  }
  if (!rec_i) rec_i = max_rec;
  else if (replace) DmRemoveRecord(phSaveDB, rec_i);
  vh = DmNewRecord(phSaveDB, &rec_i, rec_size);
  p = MemHandleLock(vh);

  // The newsgroup is unclear on the proper use of DmResizeRecord.
  // I think I'll just remove and recreate it.
  //   vh = DmResizeRecord(phSaveDB, REC_SAVECHAR, rec_size);
  //   if (!vh) level_message("ARRRRRGH");
  //   p = MemHandleLock(vh); // XXX sometimes I get " Null handle "


  // Ok!  Start writing!
  DmWrite(p, offset, &stamp, sizeof(UShort));
  offset += sizeof(UShort);
  DmWrite(p, offset, &valid, sizeof(UShort));
  offset += sizeof(UShort);
  DmWrite(p, offset, &dlevel, sizeof(Int8));
  offset += sizeof(Int8);
  DmWrite(p, offset, &maxdlevel, sizeof(Int8));
  offset += sizeof(Int8);
  // Note, I've moved "you" in front of invent,etc.
  DmWrite(p, offset, &you, sizeof(you_t));
  offset += sizeof(you_t);
  DmWrite(p, offset, oc_name_known, MAX_OC_NK * sizeof(UChar));
  offset += MAX_OC_NK * sizeof(UChar);
  DmWrite(p, offset, bases, (NUM_OBJ_SYMBOLS+1) * sizeof(Short));
  offset += (NUM_OBJ_SYMBOLS+1) * sizeof(Short);
  DmWrite(p, offset, level_exists, (MAXLEVEL+1) * sizeof(Boolean));
  offset += (MAXLEVEL+((MAXLEVEL & 0x1)?1:2)) * sizeof(Boolean);//EVEN

  save_tag(p, &offset, "yinv");
  offset = saveobjchn(p, offset, invent);
  save_tag(p, &offset, "fcob");
  offset = saveobjchn(p, offset, fcobj);
  save_tag(p, &offset, "fall");
  offset = savemonchn(p, offset, fallen_down);
  DmWrite(p, offset, &flags, sizeof(flags_t)); offset += sizeof(flags_t);
  DmWrite(p, offset, &moves, sizeof(Long));    offset += sizeof(Long);
  if (you.ustuck) {
    DmWrite(p, offset, &(you.ustuck->m_id), sizeof(UShort));
    offset += sizeof(UShort);
  }
  // note: pl_character is subsumed by you.character_class
  DmWrite(p, offset, genocided, MAX_GENOCIDE * sizeof(Char));
  offset += MAX_GENOCIDE * sizeof(Char);
  DmWrite(p, offset, fut_geno, MAX_GENOCIDE * sizeof(Char));
  offset += MAX_GENOCIDE * sizeof(Char);
  // no need for old "savenames" since oc_unames already lives in database.

  // Then there is a business of getlev/savelev for each saved level.
  // This can be skipped because for me they'll be in the right "file" already.
  MemHandleUnlock(vh);
  DmReleaseRecord(phSaveDB, REC_SAVECHAR, true);
  return true;
}


/////////////////////////////////////////////////////////////////
Boolean dorecover()
{
  VoidPtr pstart, p;
  VoidHand vh;
  Err err;
  UShort youstuck_mid = -1;
  //
  Boolean found;
  obj_t *otmp;
  UShort valid, stamp;

  if (DmNumRecords(phSaveDB) <= REC_SAVECHAR)
    return false;
  vh = DmQueryRecord(phSaveDB, REC_SAVECHAR);
  if (!vh) return false;
  p = pstart = MemHandleLock(vh);
  // Ok!  Start reading!
  stamp = *((UShort *) p);      p += sizeof(UShort);
  if (stamp != 0) return false; // Not a save file at all!  DB is screwed up!
  valid = *((UShort *) p);      p += sizeof(UShort);
  if (!valid) return false; // Not a valid save file! probably player is dead.
  dlevel = *((Int8 *) p);      p += sizeof(Int8);
  maxdlevel = *((Int8 *) p);   p += sizeof(Int8);
  you = *((you_t *) p);         p += sizeof(you_t); // XXX

  if (you.dead) return false;

  // Read in the dungeon level, from a different record......
  if (!getlev(dlevel, true)) return false;
  // Actually, if we can't load the level, we might still recover the character
  // ... hmmmmm.

  err = MemMove(oc_name_known, p, MAX_OC_NK * sizeof(UChar));
  if (err) { MemHandleUnlock(vh); return false; }
  p += MAX_OC_NK * sizeof(UChar);
  err = MemMove(bases, p, (NUM_OBJ_SYMBOLS+1) * sizeof(Short));
  if (err) { MemHandleUnlock(vh); return false; }
  p += (NUM_OBJ_SYMBOLS+1) * sizeof(Short);
  err = MemMove(level_exists, p, (MAXLEVEL+1) * sizeof(Boolean));
  if (err) { MemHandleUnlock(vh); return false; }
  p += (MAXLEVEL+((MAXLEVEL & 0x1)?1:2)) * sizeof(Boolean);//keep it EVEN

  check_tag(&p, "yinv");
  invent = restobjchn(&p); // XXXX Apparently, slightly screwy.
  check_tag(&p, "fcob");
  fcobj = restobjchn(&p);
  check_tag(&p, "fall");
  fallen_down = restmonchn(&p); // xxx

  flags = *((flags_t *) p);  p += sizeof(flags_t);
  moves = *((Long *) p);     p += sizeof(Long);


  if (you.ustuck) {
    youstuck_mid = *((UShort *) p);
    p += sizeof(UShort);
  }
  err = MemMove(genocided, p, MAX_GENOCIDE * sizeof(Char));
  if (err) { MemHandleUnlock(vh); return false; }
  p += MAX_GENOCIDE * sizeof(Char);
  err = MemMove(fut_geno, p, MAX_GENOCIDE * sizeof(Char));
  if (err) { MemHandleUnlock(vh); return false; }
  p += MAX_GENOCIDE * sizeof(Char);
  MemHandleUnlock(vh);
  // We're done reading things!  Just have to tidy up a bit now.

  // Make sure worn things are worn
  for (otmp = invent ; otmp ; otmp = otmp->nobj)
    if (otmp->owornmask)
      setworn(otmp, otmp->owornmask);

  // Make sure of the iron ball and chain
  if (Punished) {
    for (otmp = fobj, found = false ; otmp ; otmp = otmp->nobj) 
      if (otmp->olet == CHAIN_SYM) { found = true; break; }
    if (!found) {
      level_message("Cannot find the iron chain?");
      return false;
    }
    uchain = otmp;
    if (!uball) {
      for (otmp = fobj, found = false ; otmp ; otmp = otmp->nobj)
	if (otmp->olet == BALL_SYM && otmp->spe) { found = true; break; }
      if (!found) {
	level_message("Cannot find the iron ball?");
	return false;
      }
      uball = otmp;
    }
  }

  // Locate the ustuck monster (by id)
  if (you.ustuck) {
    monst_t *mtmp;
    for (mtmp = fmon, found = false ; mtmp ; mtmp = mtmp->nmon)
      if (mtmp->m_id == youstuck_mid) { found = true; break; }
    if (!found) {
      level_message("Cannot find the monster ustuck.");
      return false;
    }
    you.ustuck = mtmp;
  }

  SysRandom(TimGetSeconds()); // might as well
  oinit(); // rebuild the gem probabilities..
  setsee();  /* only to recompute seelx etc. - these weren't saved */
  //  docrt();
  return true;
}



