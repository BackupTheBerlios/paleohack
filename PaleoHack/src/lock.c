/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"

// these hosers are dynamically allocated lists,
// they are only placed here for convenience:
obj_t   * fobj;
obj_t   * fcobj;
obj_t   * invent;
obj_t   * uwep, * uarm, * uarm2, * uarmh, * uarms, * uarmg;
obj_t   * uright, * uleft, * uchain, * uball;
monst_t * fmon;
monst_t * fallen_down;
monst_t * mydogs;
trap_t  * ftrap;
gold_t  * fgold;

// more homeless globals (these not dynamically allocated):
struct flag flags;

// this is a replacement for the oc_name_known field of object class:
// there are 217 items in the objclass array... that means 28 bytes; 224 bits.
UChar oc_name_known[MAX_OC_NK];


// the CONST puppies, in order of their record UIDs:
objclass_t *objects;
Char       *oc_names;
Char       *oc_descrs;
permonst_t *mons;
Char       *mon_names;
Short      *trobj_start;
Short      *shknam_start;
Short      *rumors_start;

/*
 *  Call when the application is entered, to lock records in the
 *  read-only database.
 */
// warning: this relies on record ordering. be sure to create it happily.
static VoidHand vh_const[NUM_CONST_RECS];
void lock_const_recs()
{
  //  ULong uniqueID;
  //  UInt index;
  VoidPtr p[NUM_CONST_RECS];
  UInt i;
  for (i = 0 ; i < NUM_CONST_RECS; i++) vh_const[i] = NULL;
  for (i = 0 ; i < NUM_CONST_RECS; i++) {
    //    uniqueID = FIRST_CONST_REC + i;
    //    if (0!=DmFindRecordByID(phDB, uniqueID, &index)) return;
    //    vh_const[i] = DmQueryRecord(phDB, index);
    vh_const[i] = DmQueryRecord(phDB, i);
    p[i] = MemHandleLock(vh_const[i]);
  }
  i = 0;
  objects      = (objclass_t *) p[i++];
  oc_names     = (Char *)       p[i++];
  oc_descrs    = (Char *)       p[i++];
  mons         = (permonst_t *) p[i++];
  mon_names    = (Char *)       p[i++];
  trobj_start  = (Short *)      p[i++];
  shknam_start = (Short *)      p[i++];
  rumors_start = (Short *)      p[i++];
}
/*
 *  Call when the application is exited, to unlock records in the
 *  read-only database.
 */
void unlock_const_recs()
{
  Short i;
  for (i = 0 ; i < NUM_CONST_RECS; i++)
    if (vh_const[i])
      MemHandleUnlock(vh_const[i]);
  objects      = NULL;
  oc_names     = NULL;
  oc_descrs    = NULL;
  mons         = NULL;
  mon_names    = NULL;
  trobj_start  = NULL;
  shknam_start = NULL;
  rumors_start = NULL;
}


/***********************************************************************/

// Routines for saving (totally rewriting) the obj_uname record...
// (strings that classes of potions/scrolls/wands/rings are "called")

// The record has this format:
// 1 short = number of strings in it = n
// n shorts = offsets to the strings
// n strings

ULong volatile_size[NUM_VOLATILE_RECS] = {
  (CALLABLE_OBJS+1)*2 + CALLABLE_OBJS*1,
  MAX_OBJCLASS*2
};

Short      *uname_start;
Short      *oc_descr_offset;

static VoidHand vh_volatile[NUM_VOLATILE_RECS];

//static void rec_uname_init(VoidPtr p);

void lock_volatile_recs()
{
  VoidPtr p[NUM_VOLATILE_RECS];
  UInt i;
  for (i = 0 ; i < NUM_VOLATILE_RECS; i++) vh_volatile[i] = NULL;
  for (i = 0 ; i < NUM_VOLATILE_RECS; i++) {
    if (i < DmNumRecords(phSaveDB)) {
      vh_volatile[i] = DmGetRecord(phSaveDB, i);
      p[i] = MemHandleLock(vh_volatile[i]);
    } else {
      vh_volatile[i] = DmNewRecord(phSaveDB, &i, volatile_size[i]);
      p[i] = MemHandleLock(vh_volatile[i]);
      if (i == REC_UNAME) {
	uname_start = (Short *) p[0];
	rec_uname_init();
      } else //if (p[i])
	DmSet(p[i], 0, volatile_size[i], 0);
      MemHandleUnlock(vh_volatile[i]);
      DmReleaseRecord(phSaveDB, i, true);
      vh_volatile[i] = DmGetRecord(phSaveDB, i);
      p[i] = MemHandleLock(vh_volatile[i]);
    }
  }
  uname_start     = (Short *) p[0];
  oc_descr_offset = (Short *) p[1];
}

/*
 *  Call when the application is exited, to unlock records in the
 *  save database.
 */
void unlock_volatile_recs()
{
  Short i;
  for (i = 0 ; i < NUM_VOLATILE_RECS; i++)
    if (vh_volatile[i]) {
      MemHandleUnlock(vh_volatile[i]); // ?
      DmReleaseRecord(phSaveDB, i, true);
    }
  uname_start      = NULL;
  oc_descr_offset  = NULL;
}


/***********************************************************************/

// Routines for writing to volatile records during a game session


/// No no no no, this won't work!
// we have stuff referring to these indices, we can't delete them
// because then everything that refers to them will be shifted.
// Maybe I will have to go back to the plan of:
// the record starts with 74 (or whatever) shorts
// one for each of the things that CAN be 'call'ed.
// (plus one more short that says where the last string ends!)
// and each one points to its own string (which might be only '\0')
// So, initially, we have
// 75*sizeof(Short), prev+sizeof(Char), prev+sizeof(Char), ...,
// \0, \0, \0, \0 ... \0.
// total record size is 74*sizeof(Short) + 74*sizeof(Char).

void rec_uname_init()
{
  Short i, offset = (CALLABLE_OBJS+1)*sizeof(Short);
  VoidPtr p = (VoidPtr) uname_start;
  DmSet(p, 0, volatile_size[REC_UNAME], 0);
  for (i = 0 ; i < (CALLABLE_OBJS+1) ; i++) {
    DmWrite(p, i*sizeof(Short), &offset, sizeof(Short));
    offset += sizeof(Char);
  }
}

void rec_uname_edit(Char *buf, Short i)
{
  Short len, old_len, j, delta, tmp;
  Char *uname;
  Char tmpc = '\0';
  ULong rec_size, old_rec_size;
  VoidPtr p;
  if (i < 0 || i >= CALLABLE_OBJS) return;
  if (buf == NULL) buf = &tmpc;
  // Hey, if the new string fits in the old space, edit it in place.
  uname = ((Char *) uname_start) + uname_start[i];
  old_len = uname_start[i+1] - uname_start[i]; // don't just do strlen.
  len = StrLen(buf) + 1;
  if (len <= old_len) {
    // edit in place, everyone goes home happy (although maybe a bit bloated.)
    DmWrite(uname_start, uname_start[i], buf, len);
    return;
  }
  // Rats, we have to shove everything over in order to get it to work out.
  // Maybe I can just resize the record, and copy things from the end on back.
  // (Don't forget to update the offsets.)
  
  uname_start = NULL;
  MemHandleUnlock(vh_volatile[REC_UNAME]);
  DmReleaseRecord(phSaveDB, REC_UNAME, true);
  vh_volatile[REC_UNAME] = DmGetRecord(phSaveDB, REC_UNAME); // DmQuery?xxx
  old_rec_size = MemHandleSize(vh_volatile[REC_UNAME]);
  delta = len - old_len;
  rec_size = old_rec_size + delta;
  // Some people say that you can resize a record whether it's locked or not.
  // Other people say you should unlock it first.
  // Other people say that OS 4.0 issues a warning if you don't
  // call DmGetRecord beforehand.  shrug.
  vh_volatile[REC_UNAME] = DmResizeRecord(phSaveDB, REC_UNAME, rec_size);

  // ...if vh_volatile[REC_UNAME] is NULL, we are in trouble.
  p = MemHandleLock(vh_volatile[REC_UNAME]);
  uname_start  = (Short *) p;
  uname = (Char *) uname_start;
  // Ok, we have unlocked, resized, and relocked the record... now, shift..
  // Start at old_rec_size and work back to uname_start[i+1].
  for (j = old_rec_size-1 ; j >= uname_start[i+1] ; j--)
    DmWrite(uname_start, j+delta, &(uname[j]), sizeof(Char));
  // now update the offsets... just need to add 'delta' to each from i+1 on.
  for (j = i+1 ; j < CALLABLE_OBJS+1 ; j++) {
    tmp = uname_start[j] + delta;
    DmWrite(uname_start, j*sizeof(Short), &tmp, sizeof(Short));
  }
  // whew ok finally let's write the new string
  DmWrite(uname_start, uname_start[i], buf, len);

}



// Look up an index in the uname record...
// will return a pointer to string (DO NOT fold spindle or mutilate), or NULL.
Char *rec_uname_get(Short i)
{
  Short offset;
  Char *p;
  if (i < 0 || i >= CALLABLE_OBJS)
    return NULL;
  offset = uname_start[i];
  p = (Char *) uname_start;
  p += offset;
  if (p[0] == '\0') return NULL;
  else return p;
}


void rec_ocdescr_init()
{
  Short i;
  for (i = 0 ; i < MAX_OBJCLASS ; i++)
    DmWrite(oc_descr_offset, i*sizeof(Short),
	    &(objects[i].oc_descr_offset), sizeof(Short));
}

void rec_ocdescr_set(Short i, Short val)
{
  if (i < 0 || i >= MAX_OBJCLASS) return;
  DmWrite(oc_descr_offset, i*sizeof(Short), &val, sizeof(Short));
}


/*
If you used DmRemoveRecord, the record is removed and the records that
were behind it has their indexes decremented by one. So if you had
records 1-4 and you deleted record 3, you would have records 1,2 and 3
where 3 is the index to the previous record 4.

If you used DmDeleteRecord, the entry for the record is not
removed. The memory block for the record is freed and the records
delete bit is set.  
*/
