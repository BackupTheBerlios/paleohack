/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
// these are added for saving/restoring:
#include "records.h"


Boolean in_mklev = false; // set by mklev(), consulted by make_mon.c...

Boolean level_exists[MAXLEVEL+1];

#define CHAIN_TERMINATOR -1
// 0 might also work. basically a value that a vaild pointer could never assume

static VoidHand start_savelev(Short lev, UInt *rec_i, Short start_i,
			      DmOpenRef db, Boolean delete_old) SEC_5;
static Short savegoldchn_size(gold_t *gold) SEC_5;
static Short savegoldchn(VoidPtr p, Short offset, gold_t *gold) SEC_5;
static gold_t *restgoldchn(VoidPtr *p) SEC_5;
static Short savetrapchn_size(trap_t *trap) SEC_5;
static Short savetrapchn(VoidPtr p, Short offset, trap_t *trap) SEC_5;
static trap_t *resttrapchn(VoidPtr *p) SEC_5;
static Short saveworms_size() SEC_5;
static Short saveworms(VoidPtr p, Short offset) SEC_5; // XXX not impl yet
static void restworms(VoidPtr *p) SEC_5; // XXX not impl yet
static void regenerate_mons(Long old_moves) SEC_5;


// Things that are declared extern but not in a header file.... lame...
extern room_t rooms[MAX_ROOMS];
extern PointType doors[MAX_DOORS];
extern Char genocided[];
#ifndef NOWORM
extern wseg_t *wsegs[MAX_WORM];
#endif NOWORM
// Things that AREN'T extern anywhere but need saving:
extern obj_t *billobjs;
#ifndef NOWORM
Long wgrowtime[MAX_WORM];
#endif NOWORM


void mklev()
{
  if (getbones()) return;

  in_mklev = true;
  makelevel();
  in_mklev = false;
}



void savelev(Short lev, Boolean not_bones)
{
  VoidPtr p;
  VoidHand vh;
  UInt rec_i;
  Short offset;

  if (lev < 0 || lev > MAXLEVEL) return;

  if (not_bones) { // open record to write a gamesave level
    vh = start_savelev(lev, &rec_i, REC_LEVEL_START, phSaveDB, false);
    if (vh) level_exists[lev] = true;
    else {
      // some old debugging stuff....
      //    Err err = DmGetLastErr();
      //    StrPrintF(ScratchBuffer, "%d", err); // I get: 0x201.  dmErrorClass
      // see: Core/System/DataMgr.h   #define dmErrMemError
      // whatever that means....
      //    WinDrawChars(ScratchBuffer, StrLen(ScratchBuffer), 40, 40);
      return;
    }
  } else { // open record to write a BONES level
    vh = start_savelev(lev, &rec_i, 0, phBonesDB, true);
    if (!vh) return; // a bones level already exists for this level. so, don't.
  }
	
  p = MemHandleLock(vh);

  offset = 0;
  //  bwrite(fd,(char *) &lev,sizeof(lev));
  DmWrite(p, offset, &lev, sizeof(Short));
  offset += sizeof(Short);
  //  bwrite(fd,(char *) levl,sizeof(levl)); // == floor_symbol, floor_info.
  save_tag(p, &offset, "flor");
  DmWrite(p, offset, floor_symbol, sizeof(UChar) * DCOLS * DROWS);
  offset += sizeof(UChar) * DCOLS * DROWS; // 80 * 22 = 1760 bytes
  DmWrite(p, offset, floor_info, sizeof(UChar) * DCOLS * DROWS);
  offset += sizeof(UChar) * DCOLS * DROWS;
  DmWrite(p, offset, &moves,   sizeof(Long));  offset += sizeof(Long);
  DmWrite(p, offset, &xupstair,sizeof(Short)); offset += sizeof(Short);
  DmWrite(p, offset, &yupstair,sizeof(Short)); offset += sizeof(Short);
  DmWrite(p, offset, &xdnstair,sizeof(Short)); offset += sizeof(Short);
  DmWrite(p, offset, &ydnstair,sizeof(Short)); offset += sizeof(Short);
  // So far, offset = 1 Long + 5 Short + 2 * 80 * 22 Char   == 3534 bytes.
  // I've moved rooms/doors in front of the chains, they're also constant size:
  save_tag(p, &offset, "room");
  DmWrite(p, offset, rooms, sizeof(room_t) * MAX_ROOMS);
  offset += sizeof(room_t) * MAX_ROOMS;
  save_tag(p, &offset, "door");
  DmWrite(p, offset, doors, sizeof(PointType) * MAX_DOORS);
  offset += sizeof(PointType) * MAX_DOORS;
  
  // Everything else is variable length.
  // Note, save[mumble]chn will FREE the list elements that it's called on!
  save_tag(p, &offset, "fmon");
  offset = savemonchn (p, offset, fmon);
  save_tag(p, &offset, "fgld");
  offset = savegoldchn(p, offset, fgold);
  save_tag(p, &offset, "ftrp");
  offset = savetrapchn(p, offset, ftrap);
  save_tag(p, &offset, "fobj");
  offset = saveobjchn (p, offset, fobj);
  save_tag(p, &offset, "bill");
  offset = saveobjchn (p, offset, billobjs);
  save_tag(p, &offset, "ENGR");
  offset = save_engravings(p, offset); // sets head_engr to NULL.
  billobjs = NULL;
  fgold = NULL;
  ftrap = NULL;
  fmon = NULL;
  fobj = NULL;
#ifndef NOWORM
  //  save_tag(p, &offset, "worm");
  offset = saveworms(p, offset);
#endif NOWORM

  MemHandleUnlock(vh);
  DmReleaseRecord( (not_bones ? phSaveDB : phBonesDB), rec_i, true);

}

static VoidHand start_savelev(Short lev, UInt *rec_i, Short start_i,
			      DmOpenRef db, Boolean delete_old)
{
  VoidHand vh;
  UInt max_rec;
  Short i, *recp;
  Boolean found = false, replace = false;
  ULong rec_size = 0;

  // Figure out where to insert this record.  Delete one if this replaces it.
  max_rec = DmNumRecords(db);
  for (i = start_i ; i < max_rec && !found ; i++) {
    vh = DmQueryRecord(db, i);
    recp = (Short *) MemHandleLock(vh);
    if (recp) {
      if (recp[0] >= lev) { // we'll insert new record at this index
	found = true;
	*rec_i = i;
	if (recp[0] == lev) replace = true; // need to delete this record first
      }
    }
    MemHandleUnlock(vh);
  }
  if (!found) *rec_i = max_rec;
  else if (replace) {
    if (delete_old) DmRemoveRecord(db, *rec_i);
    else return NULL;
  }

  // Allocate space for the level.  I wonder how much we need.
  // Make sure this stays consistent with what we're actually
  // writing to the record (compare it to increments of "offset".)
  rec_size += sizeof(Short) + 2 * sizeof(UChar) * DCOLS * DROWS;
  rec_size += sizeof(Long) + 4 * sizeof(Short);
  rec_size += sizeof(room_t) * MAX_ROOMS + sizeof(PointType) * MAX_DOORS;
  // now we get into the variable length stuff
  rec_size += savemonchn_size (fmon);
  rec_size += savegoldchn_size(fgold);
  rec_size += savetrapchn_size(ftrap);
  rec_size += saveobjchn_size (fobj);
  rec_size += saveobjchn_size (billobjs);
  rec_size += save_engravings_size();
  rec_size += saveworms_size();
  rec_size += 4 * 10; // all of the "save_tag" nuggets
  vh = DmNewRecord(db, rec_i, rec_size); // XXX FAILS!  Why??
  return vh;
}


//void bwrite(Short fd, Char *loc, UInt num)
//{
//  /* lint wants the 3rd arg of write to be an int; lint -p an unsigned */
//  if (write(fd, loc, (int) num) != num)
//    panic("cannot write %u bytes to file #%d", num, fd);
//}

// Predict how much space "saveobjchn" will use.
Short saveobjchn_size(obj_t *otmp)
{
  Short offset = 0, extra_len;
  while (otmp) {
    extra_len = (otmp->oextra ? StrLen(otmp->oextra) : 0);
    offset += sizeof(Short);
    offset += sizeof(obj_t);
    offset += sizeof(Char)*extra_len;
    if (extra_len & 0x1) offset++;
    otmp = otmp->nobj;
  }
  offset += sizeof(Short);
  return offset;
}
// Save the objects in a list.  Doesn't call anything else exciting.
Short saveobjchn(VoidPtr p, Short offset, obj_t *otmp)
{
  obj_t *next_otmp;
  Short extra_len, tmp;

  while (otmp) {
    next_otmp = otmp->nobj;
    extra_len = (otmp->oextra ? StrLen(otmp->oextra) : 0);
    if (extra_len & 0x1) extra_len++; // keep it EVEN
    DmWrite(p, offset, &extra_len, sizeof(Short));
    offset += sizeof(Short); // could write this in place of "obj->nobj" Long..
    DmWrite(p, offset, otmp, sizeof(obj_t));
    offset += sizeof(obj_t);
    if (extra_len) {
      DmWrite(p, offset, otmp->oextra, sizeof(Char)*StrLen(otmp->oextra));
      free_me((VoidPtr) otmp->oextra);
      offset += sizeof(Char)*extra_len;
    }
    free_me((VoidPtr) otmp);
    otmp = next_otmp;
  }
  tmp = CHAIN_TERMINATOR;
  DmWrite(p, offset, &tmp, sizeof(Short));
  offset += sizeof(Short);
  return offset;
}

obj_t * restobjchn(VoidPtr *p)
{
  obj_t *otmp, *prev_otmp = NULL;/* suppress "used before set" warning */
  obj_t *first = NULL;
  Short extra_len;

  while (true) {
    extra_len = *((Short *)(*p));
    *p += sizeof(Short);
    if (extra_len == CHAIN_TERMINATOR) break;

    otmp = (obj_t *) md_malloc(sizeof(obj_t));
    if (!first) first = otmp;
    else prev_otmp->nobj = otmp;

    *otmp = *((obj_t *)(*p));
    *p += sizeof(obj_t);
    if (extra_len) {
      otmp->oextra = (Char *) md_malloc((extra_len+1) * sizeof(Char));
      StrNCopy(otmp->oextra, (Char *)(*p), extra_len);
      otmp->oextra[extra_len-1] = '\0';
      *p += sizeof(Char) * extra_len;
    } else otmp->oextra = NULL;
    if (!otmp->o_id) otmp->o_id = flags.ident++;
    prev_otmp = otmp;
  }
  if (first && prev_otmp->nobj) {
    // more like, whoever wrote objchn screwed up.
    level_message("Restobjchn: error reading objchn.");
    prev_otmp->nobj = NULL;
  }
  return first;
}

// Predict how much space savemonchn will take up.
Short savemonchn_size(monst_t *mtmp)
{
  Short name_len;
  Short offset = 0;

  while (mtmp) {
    name_len = (mtmp->name ? StrLen(mtmp->name) : 0);
    offset += sizeof(Short);
    offset += sizeof(Short);
    offset += sizeof(monst_t);
    if (mtmp->extra_len)
      offset += mtmp->extra_len;
    // extra_len is a struct, so it's probably packed to be even already?
    if (name_len)
      offset += sizeof(Char)*name_len;
    if (name_len & 0x1) offset++; // keep it EVEN
    if (mtmp->minvent) {
      offset += saveobjchn_size(mtmp->minvent); // get size of inventory...
      offset += 4; // for the tag
    }
    mtmp = mtmp->nmon;
    offset += 4; // for the tag
  }
  offset += sizeof(Short);
  return offset;
}
// Save the monsters in a list.  May call saveobjchn for any monster.
Short savemonchn(VoidPtr p, Short offset, monst_t *mtmp)
{
  monst_t *next_mtmp;
  Short name_len;
  Short tmp;
  permonst_t *monbegin = &mons[0];
  ULong diff;

  while (mtmp) {
    next_mtmp = mtmp->nmon;
    diff = mtmp->data - monbegin; // we'll write INDEX instead of pointer...
    mtmp->data = (permonst_t *) diff; // ...do some evil type-clobbering.
    name_len = (mtmp->name ? StrLen(mtmp->name) : 0);
    if (name_len & 0x1) name_len++; // keep it EVEN
    //    xl = mtmp->mxlth + mtmp->mnamelth;
    DmWrite(p, offset, &(mtmp->extra_len), sizeof(Short));
    offset += sizeof(Short);
    DmWrite(p, offset, &name_len, sizeof(Short));
    offset += sizeof(Short);
    //    bwrite(fd, (char *) &xl, sizeof(int));
    DmWrite(p, offset, mtmp, sizeof(monst_t));
    //    DmWrite(p, offset+sizeof(monst_t*), &diff, sizeof(Long));//OVERWRITE 'data'
    offset += sizeof(monst_t);
    if (mtmp->extra_len) {
      DmWrite(p, offset, mtmp->extra, mtmp->extra_len);
      free_me((VoidPtr) mtmp->extra);
      offset += mtmp->extra_len;
    }
    if (name_len) {
      DmWrite(p, offset, mtmp->name, sizeof(Char)*StrLen(mtmp->name));
      free_me((VoidPtr) mtmp->name);
      offset += sizeof(Char)*name_len;
    }
    if (mtmp->minvent) {
      save_tag(p, &offset, "minv");
      offset += saveobjchn(p, offset, mtmp->minvent); //frees mon inventory too
    }
    free_me((VoidPtr) mtmp);
    mtmp = next_mtmp;
    save_tag(p, &offset, "/mon");    
  }
  tmp = CHAIN_TERMINATOR;
  DmWrite(p, offset, &tmp, sizeof(Short));
  offset += sizeof(Short);
  return offset;
}

// quite similar to restobjchn
monst_t * restmonchn(VoidPtr *p)
{
  monst_t *mtmp, *prev_mtmp;
  monst_t *first = NULL;
  Short name_len, extra_len;
  ULong diff;
  permonst_t *monbegin = &mons[0];

  /* suppress "used before set" warning from lint */
  prev_mtmp = NULL;
  while (true) {
    extra_len = *((Short *)(*p));    *p += sizeof(Short);
    if (extra_len == CHAIN_TERMINATOR) break;
    name_len = *((Short *)(*p));     *p += sizeof(Short);

    mtmp = (monst_t *) md_malloc(sizeof(monst_t));
    if (!first) first = mtmp;
    else prev_mtmp->nmon = mtmp;

    *mtmp = *((monst_t *)(*p));
    *p += sizeof(monst_t);

    if (extra_len) {
      mtmp->extra = (UChar *) md_malloc(extra_len * sizeof(UChar));
      /*err =*/ MemMove(mtmp->extra, *p, extra_len * sizeof(UChar));
      *p += sizeof(UChar) * extra_len;
    } else mtmp->extra = NULL;

    if (name_len) {
      mtmp->name = (Char *) md_malloc((name_len+1) * sizeof(Char));
      StrNCopy(mtmp->name, (Char *)(*p), name_len);
      mtmp->name[name_len-1] = '\0';
      *p += sizeof(Char) * name_len;
    } else mtmp->name = NULL;

    if (!mtmp->m_id)
      mtmp->m_id = flags.ident++;
    diff = (Long) mtmp->data; // CONVERT FROM INDEX TO POINTER.  heh.
    mtmp->data = monbegin + diff;
    if (mtmp->minvent) {
      WinDrawChars("BOO!", 4, 10, 10);
      check_tag(p, "minv");
      mtmp->minvent = restobjchn(p); // xxx
    }
    prev_mtmp = mtmp;
    check_tag(p, "/mon");
  }
  if (first && prev_mtmp->nmon) {
    level_message("Restmonchn: error reading monchn.");
    prev_mtmp->nmon = NULL;
  }
  return first;
}

static Short savegoldchn_size(gold_t *gold)
{
  Short offset = 0;
  while (gold) {
    offset += sizeof(gold_t);
    gold = gold->ngold;
  }
  offset += sizeof(Short);
  return offset;
}
static Short savegoldchn(VoidPtr p, Short offset, gold_t *gold)
{
  Short tmp;
  gold_t *next_gold;
  while (gold) {
    next_gold = gold->ngold;
    DmWrite(p, offset, gold, sizeof(gold_t));
    offset += sizeof(gold_t);
    free_me((VoidPtr) gold);
    gold = next_gold;
  }
  //  bwrite(fd, nul, sizeof(struct gold));
  tmp = CHAIN_TERMINATOR;
  DmWrite(p, offset, &tmp, sizeof(Short));
  offset += sizeof(Short);
  return offset;
}

static gold_t *restgoldchn(VoidPtr *p)
{
  gold_t *gold, *prev_gold = NULL;
  gold_t *first = NULL;
  while (true) {
    if (*((Short *) *p) == CHAIN_TERMINATOR) { *p += sizeof(Short); break; }

    gold = (gold_t *) md_malloc(sizeof(gold_t));
    if (!first) first = gold;
    else prev_gold->ngold = gold;

    *gold = *((gold_t *)(*p));
    *p += sizeof(gold_t);
    prev_gold = gold;
  }
  if (first && prev_gold->ngold) {
    level_message("Restgoldchn: error reading goldchn.");
    prev_gold->ngold = NULL;
  }
  return first;
}

// Traps will be exactly like gold...... just M-x replace-string.
static Short savetrapchn_size(trap_t *trap)
{
  Short offset = 0;
  while (trap) {
    offset += sizeof(trap_t);
    trap = trap->ntrap;
  }
  offset += sizeof(Short);
  return offset;
}

static Short savetrapchn(VoidPtr p, Short offset, trap_t *trap)
{
  Short tmp;
  trap_t *next_trap;
  while (trap) {
    next_trap = trap->ntrap;
    DmWrite(p, offset, trap, sizeof(trap_t));
    offset += sizeof(trap_t);
    free_me((VoidPtr) trap);
    trap = next_trap;
  }
  //  bwrite(fd, nul, sizeof(struct trap));
  tmp = CHAIN_TERMINATOR;
  DmWrite(p, offset, &tmp, sizeof(Short));
  offset += sizeof(Short);
  return offset;
}

static trap_t *resttrapchn(VoidPtr *p)
{
  trap_t *trap, *prev_trap = NULL;
  trap_t *first = NULL;
  while (true) {
    if (*((Short *) *p) == CHAIN_TERMINATOR) { *p += sizeof(Short); break; }

    trap = (trap_t *) md_malloc(sizeof(trap_t));
    if (!first) first = trap;
    else prev_trap->ntrap = trap;

    *trap = *((trap_t *)(*p));
    *p += sizeof(trap_t);
    prev_trap = trap;
  }
  if (first && prev_trap->ntrap) {
    level_message("Resttrapchn: error reading trapchn.");
    prev_trap->ntrap = NULL;
  }
  return first;
}

static Short saveworms_size()
{
  return 0; // XXXXXX saving worms is not implemented yet
}
static Short saveworms(VoidPtr p, Short offset)
{
  Short tmp;
#ifndef NOWORM
  wseg_t *wtmp, *next_wtmp;
  return 0; // XXXXXX saving worms is not implemented yet
  // Something seems screwy in here.  Try to understand it later.
  DmWrite(p, offset, wsegs, MAX_WORM * sizeof(wseg_t));
  offset += MAX_WORM * sizeof(wseg_t);
  for (tmp = 1 ; tmp < MAX_WORM ; tmp++) { // xxxx why 1, why not "tmp = 0"?
    for (wtmp = wsegs[tmp] ; wtmp ; wtmp = next_wtmp) {
      next_wtmp = wtmp->nseg;
      //      bwrite(fd,(char *) wtmp,sizeof(struct wseg));
      DmWrite(p, offset, wtmp, sizeof(wseg_t));
      offset += sizeof(wseg_t);
      // xxxxxx HEY, don't we need to FREE wtmp???? (IF it's not a wsegs[i])
    }
    wsegs[tmp] = NULL;
  }
  DmWrite(p, offset, wgrowtime, MAX_WORM * sizeof(Long));
  offset += MAX_WORM * sizeof(Long);
#endif NOWORM
  return offset;
}

static void restworms(VoidPtr *p)
{
  return; // XXXXXX saving worms is not implemented yet, so restoring isn't
  /*
#ifndef NOWORM
  mread(fd, (Char *)wsegs, sizeof(wsegs));
  for (tmp = 1; tmp < 32; tmp++) if (wsegs[tmp]){
    wheads[tmp] = wsegs[tmp] = wtmp = newseg();
    while (true) {
      mread(fd, (Char *)wtmp, sizeof(struct wseg));
      if (!wtmp->nseg) break;
      wheads[tmp]->nseg = wtmp = newseg();
      wheads[tmp] = wtmp;
    }
  }
  mread(fd, (Char *)wgrowtime, sizeof(wgrowtime));
#endif NOWORM
  */
}


// Sanity check.
void save_tag(VoidPtr p, Short *offset, Char *tag)
{
  DmWrite(p, *offset, tag, sizeof(Char)*4);
  *offset += 4;
}
void check_tag(VoidPtr *p, Char *tag)
{
  static Short y = 2;
  static Short dx = 2;
  Short x = 3;
  if (0 != StrNCompare(tag, (Char *)(*p), 4))
    x = 80; // We didn't find the right tag..
  WinDrawChars(tag, 4, x+dx, y);  y += 11;
  if (y > 160-11) {
    y = 2; dx = 40;
  }
  *p += 4;
}

//////////////////////////////////////////////////////////////////////////

// This will return false if we can't find the file.
Boolean getlev(UChar lev, Boolean not_bones)
{
  //  gold_t *gold;
  //  trap_t *trap;
  //#ifndef NOWORM
  //  wseg_t *wtmp;
  //#endif NOWORM
  //  Short tmp;
  Long old_moves;
  //  Short hpid;
  //  UChar dlvl;
  VoidPtr p;
  VoidHand vh = NULL;
  Err err;
  Short rec_i, max_rec, *recp = NULL, i;
  DmOpenRef db = (not_bones ? phSaveDB : phBonesDB);
  Short start_i = (not_bones ? REC_LEVEL_START : 0);

  // Need to figure out which record to open.
  // Open it for read.     (Remember to close it again afterward.)

  // Figure out where to insert this record.  Delete one if this replaces it.
  max_rec = DmNumRecords(db);
  for (i = start_i, rec_i = 0 ; (i < max_rec) && !rec_i ; i++) {
    vh = DmQueryRecord(db, i);
    recp = (Short *) MemHandleLock(vh);
    if (recp && recp[0] == lev) {
      rec_i = i; // keep this around in case we decide to delete it after.
      break;
    }
    MemHandleUnlock(vh);
  }
  if (!rec_i) return false; // caller should simply REGENERATE level, quietly.
  p = (VoidPtr) recp;
  // Ok, read some stuff.

  fgold = NULL;
  ftrap = NULL;
  p += sizeof(Short); // (we already read 'lev')

  check_tag(&p, "flor");
  err = MemMove(floor_symbol, p, sizeof(UChar) * DCOLS * DROWS);
  if (err) { MemHandleUnlock(vh); return false; }
  p += sizeof(UChar) * DCOLS * DROWS;
  err = MemMove(floor_info, p, sizeof(UChar) * DCOLS * DROWS);
  if (err) { MemHandleUnlock(vh); return false; }
  p += sizeof(UChar) * DCOLS * DROWS;

  old_moves = *((Long *) p);     p += sizeof(Long);
  xupstair = *((Short *) p);     p += sizeof(Short);
  yupstair = *((Short *) p);     p += sizeof(Short);
  xdnstair = *((Short *) p);     p += sizeof(Short);
  ydnstair = *((Short *) p);     p += sizeof(Short);
  // I've moved rooms/doors in front of the chains, they're also constant size:
  check_tag(&p, "room");
  err = MemMove(rooms, p, sizeof(room_t) * MAX_ROOMS);
  if (err) { MemHandleUnlock(vh); return false; }
  p += sizeof(room_t) * MAX_ROOMS;
  check_tag(&p, "door");
  err = MemMove(doors, p, sizeof(PointType) * MAX_DOORS);
  if (err) { MemHandleUnlock(vh); return false; }
  p += sizeof(PointType) * MAX_DOORS;

  // Everything else is variable length:
  // fmon, fgold, ftrap, fobj, billobjs, engravings, worms.

  check_tag(&p, "fmon");
  fmon = restmonchn(&p);
  regenerate_mons(old_moves); // monsters heal while you're gone!
  setgd(); // figure out whether a vault guard exists.

  check_tag(&p, "fgld");
  fgold = restgoldchn(&p);
  check_tag(&p, "ftrp");
  ftrap = resttrapchn(&p);
  check_tag(&p, "fobj");
  fobj = restobjchn(&p);
  check_tag(&p, "bill");
  billobjs = restobjchn(&p); // XXX
  check_tag(&p, "ENGR");
  rest_engravings(&p);

#ifndef NOWORM
  //  check_tag(&p, "worm");
  restworms(&p);
#endif NOWORM

  MemHandleUnlock(vh);

  if (!not_bones) // The bones file should be deleted after it is used.
#ifdef WIZARD
    if (!wizard)
#endif WiZARD
      DmRemoveRecord(phBonesDB, rec_i);

  return true;
}

// Kill monsters that have been genocided since we left the level.
// Make tame monsters untame and unpeaceful if abandoned too long.
// Cause monsters' health to regenerate.
static void regenerate_mons(Long old_moves)
{
  Long tmoves = (moves > old_moves) ? moves-old_moves : 0;
  monst_t *mtmp, *next_mtmp;

  for (mtmp = fmon ; mtmp ; mtmp = next_mtmp) {
    Long newhp;		/* tmoves may be very large */
    Boolean fast_regen;

    next_mtmp = mtmp->nmon;
    if (StrChr(genocided, mtmp->data->mlet)) {
      mondead(mtmp);
      continue;
    }
    if ((mtmp->bitflags & M_IS_TAME) && tmoves > 250)
      mtmp->bitflags &= ~(M_IS_TAME | M_IS_PEACEFUL);

    fast_regen = (0 != StrChr(MREGEN, mtmp->data->mlet));
    newhp = mtmp->mhp + (fast_regen ? tmoves : tmoves/20);
    mtmp->mhp = (newhp > mtmp->mhpmax) ? mtmp->mhpmax : newhp;
  }
}


/*
extern Boolean restoring;
void mread(Short fd, Char *buf, UInt len)
{
  Short rlen;

  rlen = read(fd, buf, (int) len);
  if (rlen != len){
    pline("Read %d instead of %u bytes.\n", rlen, len);
    if (restoring) {
      (void) unlink(SAVEF);
      error("Error restoring old game.");
    }
    panic("Error reading level file.");
  }
}
*/
