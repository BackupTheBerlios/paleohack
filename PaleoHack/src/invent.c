/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
#include "paleohackRsc.h" // for askchain
#include "bit.h"

#define NOINVSYM        '#'
static Int8 lastinvnr = 51;   /* 0 ... 51 */


static void assigninvlet(obj_t *otmp) SEC_2;
static Short obj_to_let(struct obj *obj) SEC_2;
static Char * xprname(obj_t *obj, Char let) SEC_2;
static Boolean is_mergy(Char olet) SEC_2;
static Boolean merged(obj_t *otmp, obj_t *obj, Boolean lose) SEC_2;


// Goal: Find an unused inventory-letter and assign it to otmp.
static void assigninvlet(obj_t *otmp)
{
#define NO_LETTER -1
  UChar inuse[7]; // 52/8 = 6.5     //  Boolean inuse[52];
  Short i;
  Char c;
  obj_t *obj;
  for (i = 0; i < 7; i++) inuse[i] = 0x00;

  for (obj = invent; obj; obj = obj->nobj)
    if (obj != otmp) {
      c = obj->invlet;
      i = NO_LETTER;
      if ('a' <= c && c <= 'z')
	i = c - 'a';
      else if ('A' <= c && c <= 'Z')
	i = c - 'A' + 26;
      if (i != NO_LETTER)
	BITSET(inuse, i);
      if (c == otmp->invlet) // hm, otmp is already using someone else's letter
	otmp->invlet = 0;
    }
  c = otmp->invlet;
  if (c && (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z')))
    return; // otmp is already using a letter that no one else is using: Fine.

  // Walk through the used-letter list (starting at wherever we left off
  // last time assigninvlet() was called!)
  for (i = lastinvnr + 1; i != lastinvnr; i++) {
    if (i == 52) i = -1; // wrap around.. til we get to where we started.
    else if (!BITTEST(inuse, i)) break; // found an unused letter!
  }
  // Ok, maybe we found a letter, maybe we didn't..
  otmp->invlet = ((BITTEST(inuse, i)) ? NOINVSYM :
		  (i < 26) ? ('a'+i) : ('A'+i-26));
  lastinvnr = i;
#undef NO_LETTER
}


// This will add obj to the "invent" list..
obj_t * addinv(obj_t *obj)
{
  obj_t *otmp;

  if (!invent) { // no inventory... add to head
    invent = obj;
    otmp = 0;
  } else  /* merge, or attach to end of chain */
    for (otmp = invent; /* otmp */; otmp = otmp->nobj) {
      if (merged(otmp, obj, false))
	return otmp;
      if (!otmp->nobj) {
	otmp->nobj = obj;
	break;
      }
    }
  obj->nobj = 0;

  if (flags.invlet_constant) { // perform magic..
    assigninvlet(obj);
    /*
     * The ordering of the chain is nowhere significant
     * so in case you prefer some other order than the
     * historical one, change the code below.
     */
    if (otmp) {	/* find proper place in chain */
      otmp->nobj = 0;
      if ((invent->invlet ^ 040) > (obj->invlet ^ 040)) {
	obj->nobj = invent;
	invent = obj;
      } else
	for (otmp = invent; ; otmp = otmp->nobj) {
	  if (!otmp->nobj ||
	      (otmp->nobj->invlet ^ 040) > (obj->invlet ^ 040)){
	    obj->nobj = otmp->nobj;
	    otmp->nobj = obj;
	    break;
	  }
	}
    }
  }

  return(obj);
}

void useup(obj_t *obj)
{
  if (obj->quantity > 1){
    obj->quantity--;
    obj->owt = weight(obj);
  } else {
    setnotworn(obj);
    unlink_inv(obj);
    free_obj(obj, NULL);
  }
}

void unlink_inv(obj_t *obj) // was freeinv, that's a bad name, doesn't free it.
{
  obj_t *otmp;

  if (obj == invent) invent = invent->nobj;
  else {
    for (otmp = invent; otmp->nobj != obj; otmp = otmp->nobj)
      //if (!otmp->nobj) { alert_message("error in unlink_inv"); return; }
      if (!otmp) { alert_message("error in unlink_inv"); return; }
    otmp->nobj = obj->nobj;
  }
}

/* destroy object in fobj chain (if unpaid, it remains on the bill) */
void delobj(obj_t *obj)
{
  unlink_obj(obj);
  unpobj(obj);
  free_obj(obj, NULL);
}

/* unlink obj from chain starting with fobj; does NOT free obj. */
//void freeobj(struct obj *obj) // This is a bad name.
void unlink_obj(obj_t *obj)
{
  obj_t *otmp;

  if (obj == fobj) fobj = fobj->nobj;
  else {
    for (otmp = fobj; otmp->nobj != obj; otmp = otmp->nobj)
      if (!otmp) { alert_message("error in unlink_obj"); return; }
    otmp->nobj = obj->nobj;
  }
}

void freegold(gold_t *gold)
{
  gold_t *gtmp;

  if (gold == fgold) fgold = gold->ngold;
  else {
    for (gtmp = fgold; gtmp->ngold != gold; gtmp = gtmp->ngold)
      if (!gtmp) { alert_message("error in freegold"); return; }
    gtmp->ngold = gold->ngold;
  }
  free_me((VoidPtr) gold); //  free((char *) gold);
}



trap_t *trap_at(Short x, Short y) // was t_at
{
  trap_t *trap = ftrap;
  for (trap = ftrap; trap; trap = trap->ntrap)
    if (trap->tx == x && trap->ty == y)
      return trap;
  return NULL;
}

wseg_t *m_atseg;
extern wseg_t *wsegs[MAX_WORM];	/* linked list, tail first */ // worm.c

monst_t *mon_at(Short x, Short y) // was m_at
{
  monst_t *mtmp;
#ifndef NOWORM
  wseg_t *wtmp;
#endif NOWORM

  m_atseg = NULL;
  for (mtmp = fmon ; mtmp ; mtmp = mtmp->nmon) {
    if (mtmp->mx == x && mtmp->my == y)
      return mtmp;
#ifndef NOWORM
    if (mtmp->wormno) {
      for (wtmp = wsegs[mtmp->wormno] ; wtmp ; wtmp = wtmp->nseg)
	if (wtmp->wx == x && wtmp->wy == y) {
	  m_atseg = wtmp;
	  return mtmp;
	}
    }
#endif NOWORM
  }
  return NULL;
}

obj_t * obj_at(Short x, Short y) // was o_at
{
  obj_t *otmp;

  for (otmp = fobj; otmp; otmp = otmp->nobj)
    if (otmp->ox == x && otmp->oy == y)
      return otmp;
  return NULL;
}

obj_t * sobj_at(Short n, Short x, Short y)
{
  struct obj *otmp;

  for (otmp = fobj; otmp; otmp = otmp->nobj)
    if (otmp->ox == x && otmp->oy == y && otmp->otype == n)
      return otmp;
  return NULL;
}

Boolean carried(obj_t *obj)
{
  obj_t *otmp;
  for (otmp = invent; otmp; otmp = otmp->nobj)
    if (otmp == obj) return true;
  return false;
}

Boolean carrying(Short type)
{
  obj_t *otmp;

  for (otmp = invent; otmp; otmp = otmp->nobj)
    if (otmp->otype == type) return true;
  return false;
}


gold_t * gold_at(Short x, Short y)
{
  gold_t *gold = fgold;
  for (gold = fgold; gold; gold = gold->ngold)
    if (gold->gx == x && gold->gy == y)
      return gold;
  return NULL;
}






/* also called by newmail() */ // which doesn't exist in paleohack.
// return a count of identifies
static Short identify(obj_t *otmp)
{
  BITSET(oc_name_known, otmp->otype);
  otmp->bitflags |= O_IS_KNOWN & O_IS_DESCKNOWN;
  prinv(otmp); // XXX
  return 1;
}
static Boolean ckunpaid(obj_t *otmp)
{
  return( otmp->bitflags & O_IS_UNPAID );
}
/* make dummy object structure containing gold - for temporary use only */
static obj_t * mkgoldobj(Long q)
{
  obj_t *otmp;

  otmp = (obj_t *) md_malloc(sizeof(obj_t));
  /* should set o_id etc. but otmp will be freed soon */
  otmp->olet = '$';
  you.ugold -= q;
  otmp->oextra = (Char *) q; // XXX dirty trick. needs to match drop().
  flags.botl |= BOTL_GOLD;
  return(otmp);
}

// ggetobj_start lives in form_objtype.c   ...
Short ggetobj_end(Char *olets, Boolean drop_not_identify,
		  Boolean allflag, Boolean unpaidflag)
{
  Boolean (*ckfn)() = NULL;
  Short (*fn)() = NULL;
  Boolean droppedgold = false;
  Short max_objs;

  if (drop_not_identify) {
    max_objs = 0;
    fn = drop; // XXX
  } else {
    max_objs = rund(5) ? 1 : rund(5);
    fn = identify;
  }

  if (drop_not_identify && olets[0] == '$') {
    if (you.ugold) {
      (*fn)(mkgoldobj(you.ugold)); // XXX
    } else
      message("You have no gold.");
    droppedgold = true;
  }

  if (unpaidflag) ckfn = ckunpaid;

  if (droppedgold && !olets[1])
    return 1;	/* he dropped gold (or at least tried to) */
  else
    return(askchain(invent, olets, 
		    (drop_not_identify ? "Drop" : "Identify"),
		    allflag, fn, ckfn, max_objs));

}


/*
 * Walk through the chain starting at objchn and ask for all objects
 * with olet in olets (if nonNULL) and satisfying ckfn (if nonNULL)
 * whether the action in question (i.e., fn) has to be performed.
 * If allflag then no questions are asked. Max gives the max nr of
 * objects to be treated. Return the number of objects treated.
 */
// I think I will try using function pointers but I am not sure if it
// will work, what with multigen and all.
// Keep it, callers, fn, and ckfn in the Default Section to be safe.
Short askchain(obj_t *objchn, Char *filter, Char *prompt, Boolean allflag,
	       Short (*fn)(), Boolean (*ckfn)(), Short max)
{
  struct obj *otmp, *otmp2;
  Char ilet;
  Short answer, cnt = 0;
  ilet = 'a' - 1;
  for (otmp = objchn ; otmp ; otmp = otmp2) {
    if (++ilet > 'z') ilet = 'A';
    otmp2 = otmp->nobj;
    if (filter && *filter && !StrChr(filter, otmp->olet)) continue;
    if (ckfn && !(*ckfn)(otmp)) continue;
    if (!allflag)
      answer = FrmCustomAlert(PickUpThisP, prompt, xprname(otmp, ilet), NULL);
    else answer = PICKUP_YES;

    switch(answer) {
    case PICKUP_QUIT: goto ret;
    case PICKUP_ALL:  allflag = true; // fall through
    case PICKUP_YES:
      cnt += (*fn)(otmp);
      if (--max == 0) goto ret;
      break;
    case PICKUP_NO: break;
    default:        break;
    }
  }
  //  message(cnt ? "That was all." : "No applicable objects.");
  if (cnt) {
    message("That was all.");
    //    show_messages(); // xxx ?
  }
 ret:
  show_messages(); // xxx ?  Makes the 'identify' msgs show up, but not 'That was all' for removing things from icebox...
  return cnt;
}



/* obj_to_let should of course only be called for things in invent */
#define        NOINVSYM        '#'
static Short obj_to_let(obj_t *obj)
{
  obj_t *otmp;
  Char ilet;

  if (flags.invlet_constant)
    return obj->invlet;
  ilet = 'a';
  for (otmp = invent; otmp && otmp != obj; otmp = otmp->nobj)
    if (++ilet > 'z') ilet = 'A';
  return (otmp ? ilet : NOINVSYM);
}

void prinv(obj_t *obj)
{
  message(xprname(obj, obj_to_let(obj)));
}


// BUFSZ is defined in some header file now.  do we really need 256???
// 
// would it be safe to use ScratchBuffer here instead of buf?
// I think so, since xprname is only called exactly within a call to message!!
static Char * xprname(obj_t *obj, Char let)
{
  //static Char buf[BUFSZ];

  StrPrintF(ScratchBuffer, "%c - %s.",
	    (flags.invlet_constant) ? obj->invlet : let,
	    doname(obj));
  return ScratchBuffer;
}



void stackobj(obj_t *obj)
{
  obj_t *otmp = fobj;
  // find every object in your location that isn't you, and try to merge w/it.
  for (otmp = fobj; otmp; otmp = otmp->nobj)
    if (otmp != obj && otmp->ox == obj->ox && otmp->oy == obj->oy)
      if (merged(obj, otmp, true)) return;
}


static Boolean is_mergy(Char olet)
{
  switch(olet) { case '%': case '*': case '?': case '!': return true; }
  return false;
}

/* try to merge obj with otmp, and FREE obj if merge is successful */
static Boolean merged(obj_t *otmp, obj_t *obj, Boolean lose)
{
  UChar O_IS_stuff = (O_IS_UNPAID | O_IS_DESCKNOWN | O_IS_CURSED);
  if (obj->otype == otmp->otype &&
      obj->spe == otmp->spe &&
      (obj->bitflags & O_IS_stuff) == (otmp->bitflags & O_IS_stuff) &&
      (is_mergy(obj->olet) ||
       ((obj->bitflags & O_IS_KNOWN) == (otmp->bitflags & O_IS_KNOWN) &&
	(obj->olet == WEAPON_SYM && obj->otype < BOOMERANG)) ) ) {
    otmp->quantity += obj->quantity;
    otmp->owt += obj->owt;
    if (lose) unlink_obj(obj); //freeobj(obj);
    free_obj(obj, NULL);
    return true;
  }
  return false;
}

/* split obj so that it gets size num;
 * remainder is put in the object structure delivered by this call */
obj_t * splitobj(obj_t *obj, Short num) // this was in "do.c"
{
  obj_t *otmp = NULL;
  otmp = (obj_t *) md_malloc(sizeof(obj_t));// newobj(0);
  *otmp = *obj;		/* copies whole structure */
  obj->quantity = num;
  obj->owt = weight(obj);
  otmp->o_id = flags.ident++;
  obj->nobj = otmp;
  do_name(otmp, NULL); // otmp->onamelth = 0; // Undo name (if any)
  otmp->quantity -= num;
  otmp->owt = weight(otmp); /* -= obj->owt ? */   //well, is mass !conserved?
  if (obj->bitflags & O_IS_UNPAID) splitbill(obj,otmp);
  return otmp;
}



/*  doinv:

    ilet = 'a'
    ct = 0

    for each item in inventory list,
      if invlet_constant
         ilet = otmp->invlet
      print "%c - %s", ilet, doname(obj)   // see objname.c
      if !invlet_constant
         ilet++
         if ilet > 'z'
            ilet = 'A'

 */


