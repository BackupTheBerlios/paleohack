/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
#include "paleohackRsc.h"

extern previous_state curr_state;
extern Short multi; // living in movesee.c right now..


//#define MAX_ENGR_LEN 40

engr_t *head_engr;

static engr_t * engr_at(Short x, Short y) SEC_4;
static void del_engr(engr_t *ep) SEC_4;


// returns engraving-struct at x,y if any such exists..
static engr_t * engr_at(Short x, Short y)
{
  engr_t *ep = head_engr;
  while (ep) {
    if (x == ep->engr_x && y == ep->engr_y)
      return ep;
    ep = ep->nxt_engr;
  }
  return NULL;
}

Boolean sengr_at(Char *s, Short x, Short y)
{
  engr_t *ep = engr_at(x,y);
  Char *t;
  Short n;
  if (ep && ep->engr_time <= moves) {
    t = ep->engr_txt;
    /*
      if (!strcmp(s,t)) return true;
    */
    n = StrLen(s);
    while (*t) {
      if (!StrNCompare(s,t,n)) return true;
      t++;
    }
  }
  return false;
}


void you_wipe_engr(Short cnt) // was u_wipe_engr
{
  if (!you.uswallow && !Levitation)
    wipe_engr_at(you.ux, you.uy, cnt);
}

void wipe_engr_at(Short x, Short y, Short cnt)
{
  engr_t *ep = engr_at(x,y);
  Short lth, pos;
  Char ch;
  if (!ep) return;
  if ((ep->engr_type != DUST) || Levitation) {
    cnt = rund(1 + 50/(cnt+1)) ? 0 : 1;
  }
  lth = StrLen(ep->engr_txt);
  if (lth && cnt > 0 ) {
    while (cnt--) {
      pos = rund(lth);
      if ((ch = ep->engr_txt[pos]) == ' ')
	continue;
      ep->engr_txt[pos] = (ch != '?') ? '?' : ' ';
    }
  }
  while (lth && ep->engr_txt[lth-1] == ' ')
    ep->engr_txt[--lth] = 0; // delete spaces at end
  //  while (ep->engr_txt[0] == ' ')
  //    ep->engr_txt++; // delete spaces at front
  //  if (!ep->engr_txt[0]) del_engr(ep);
  for (pos = 0 ; pos < StrLen(ep->engr_txt) ; pos++)
    if (ep->engr_txt[pos] != ' ')
      return;
  del_engr(ep);
}

void read_engr_at(Short x, Short y)
{
  engr_t *ep = engr_at(x,y);
  Short skip;
  if (ep && ep->engr_txt && ep->engr_txt[0]) {
    switch(ep->engr_type) {
    case DUST:
      message("Something is written here in the dust.");
      break;
    case ENGRAVE:
      message("Something is engraved here on the floor.");
      break;
    case BURN:
      message("Some text has been burned here in the floor.");
      break;
    default: // BUG if this happens...
      message("Something is written in a very strange way.");
    }
    for (skip = 0 ; skip < StrLen(ep->engr_txt) ; skip++)
      if (ep->engr_txt[skip] != ' ') break;
    StrPrintF(ScratchBuffer,"You read: \"%s\".", ep->engr_txt + skip);
    message(ScratchBuffer);
  }
}


// so, like, when you write an engraving you need to alloc both the
// engraving AND STRING, and when you free it, remember to get BOTH!!
void make_engr_at(Short x, Short y, Char *s)
{
  engr_t *ep;

  if ((ep = engr_at(x,y)))
    del_engr(ep);
  ep = (engr_t *) md_malloc(sizeof(engr_t));
  ep->nxt_engr = head_engr;
  head_engr = ep;
  ep->engr_x = x;
  ep->engr_y = y;
  ep->engr_txt = (Char *) md_malloc(StrLen(s) + 1);//(Char *)(ep + 1);
  StrCopy(ep->engr_txt, s);
  ep->engr_time = 0;
  ep->engr_type = DUST;
  //  ep->engr_lth = StrLen(s) + 1;
}




// Call this BEFORE prompting the user for a string.
// Return false if you don't take a turn, true if you do.
Int8 engrave_type;
extern Short engrave_or_what;

Boolean check_do_engrave()
{
  Int8 type;
  obj_t *otmp = curr_state.item;
  engr_t *oep = engr_at(you.ux, you.uy);
  // If engrave is accessible outside of inventory list,
  // check this BEFORE popping up your inventory.
  if (you.uswallow) {
    message("You're joking. Hahaha!");	/* riv05!a3 */
    return false;
  }
  // Ok, the rest of this must be done AFTER selecting 'otmp' from inventory.

  /* one may write with finger, weapon or wand */
  //  if (!otmp) return false;
  // //  if (otmp == &zeroobj) otmp = NULL; // XXX
  if (otmp && otmp->otype == WAN_FIRE && otmp->spe) {
    type = BURN;
    otmp->spe--;
  } else {
    /* first, wield otmp */
    if (otmp != uwep) {
      if (uwep && (uwep->bitflags & O_IS_CURSED)) {
	/* Andreas Bormann */
	message("Since your weapon is welded to your hand,");
	StrPrintF(ScratchBuffer, "you use the %s.", aobjnam(uwep, NULL));
	message(ScratchBuffer);
	otmp = uwep;
      } else {
	if (!otmp)
	  message("You are now empty-handed.");
	else if (otmp->bitflags & O_IS_CURSED) {
	  StrPrintF(ScratchBuffer, "The %s %s to your hand!",
		    aobjnam(otmp, "weld"),
		    (otmp->quantity == 1) ? "itself" : "themselves");
	  message(ScratchBuffer);
	} else {
	  StrPrintF(ScratchBuffer, "You now wield %s.", doname(otmp));
	  message(ScratchBuffer);
	}
	setuwep(otmp);
      }
    }

    if (!otmp)
      type = DUST;
    else if (otmp->otype == DAGGER || otmp->otype == TWO_HANDED_SWORD ||
	     otmp->otype == CRYSKNIFE ||
	     otmp->otype == LONG_SWORD || otmp->otype == AXE) {
      type = ENGRAVE;
      if ((Short)otmp->spe <= -3) {
	type = DUST;
	StrPrintF(ScratchBuffer, "Your %s too dull for engraving.",
		  aobjnam(otmp, "are"));
	message(ScratchBuffer);
	if (oep && oep->engr_type != DUST) return true;
	// xxx this message is not shown until next turn??
      }
    } else
      type = DUST;
  }
  if (Levitation && type != BURN) {		/* riv05!a3 */
    message("You can't reach the floor!");
    return true;
  }
  if (oep && oep->engr_type == DUST) {
    message("You wipe out the message that was written here.");
    del_engr(oep);
    oep = 0;
  }
  if (type == DUST && oep) {
    StrPrintF(ScratchBuffer,
	      "You cannot wipe out the message that is %s in the rock.",
	      (oep->engr_type == BURN) ? "burned" : "engraved");
    message(ScratchBuffer);
    return true;
  }

  // If you get this far with no return,
  // we must pop up the form with a field in it to get a String.
  //  StrPrintF(ScratchBuffer, "What do you want to %s on the floor here? ",
  //	    (type == ENGRAVE) ? "engrave" : (type == BURN) ? "burn" : "write");
  //  message(ScratchBuffer);
  // .... Could allow cancelling from this form, and if you do cancel,
  // then (1) it does not take time and (2) "if (type == BURN) otmp->spe++;"

  curr_state.item = otmp;
  engrave_type = type;
  engrave_or_what = ACT_ENGRAVE;
  FrmPopupForm(EngraveForm);
  // Don't forget! :
  // We need to remember "otmp" and "type" for do_engrave.....
  return false; // we don't want to take a turn YET; take one AFTER the string.
}

// Return true if action took time.
Boolean do_engrave(obj_t *otmp, Char *buf, Int8 type)
{
  Short len;
  Char *sp;
  engr_t *ep, *oep = engr_at(you.ux, you.uy);
  //  Char buf[BUFSZ];
  Short spct;		/* number of leading spaces */
  multi = 0; // XXX

  // We have gotten the object-to-engrave-with via inventory list.
  // We have gotten the message to engrave, via form with editable field in it.
  // Now do the dirty work.
  spct = 0;
  sp = buf;
  while (*sp == ' ') { spct++; sp++; }
  len = StrLen(sp);
  if (!len /* || *buf == '\033'*/) {
    if (type == BURN) otmp->spe++;
    return false;
  }
	
  switch(type) {
  case DUST:
  case BURN:
    if (len > 15) {
      multi = -(len/10); // XXX
      spin_multi("You finished writing.");
    }
    break;
  case ENGRAVE:		/* here otmp != 0 */
    {
      Short len2 = (otmp->spe + 3) * 2 + 1;

      StrPrintF(ScratchBuffer, "Your %s dull.", aobjnam(otmp, "get"));
      message(ScratchBuffer);
      if (len2 < len) {
	len = len2;
	sp[len] = 0;
	otmp->spe = -3; // XXX
	multi = -len;
	spin_multi("You cannot engrave more.");
      } else {
	otmp->spe -= len/2; // XXX
	multi = -len;
	spin_multi("You finished engraving.");
      }
    }
    break;
  }
  if (oep) len += StrLen(oep->engr_txt) + spct;
  ep = (engr_t *) md_malloc(sizeof(engr_t));
  ep->nxt_engr = head_engr;
  head_engr = ep;
  ep->engr_x = you.ux;
  ep->engr_y = you.uy;
  //  sp = (Char *)(ep + 1);	/* (char *)ep + sizeof(struct engr) */
  ep->engr_txt = (Char *) md_malloc(len + 1); //sp;
  sp = ep->engr_txt;
  if (oep) {
    StrCopy(sp, oep->engr_txt);
    StrCat(sp, buf);
    del_engr(oep);
  } else
    StrCopy(sp, buf);
  //  ep->engr_lth = len+1;
  ep->engr_type = type;
  //  ep->engr_time = moves - multi; // XXXXXXXX need this; don't have multi?

  /* kludge to protect pline against excessively long texts */
  if (len > BUFSZ-20) sp[BUFSZ-20] = 0;

  return true;
}

// SAVE ENGRAVINGS
#define CHAIN_TERMINATOR -1
// How much space will we need
Short save_engravings_size() // needs to match save_engravings "offset" calc.
{
  Short offset = 0, len;
  engr_t *ep = head_engr;
  while (ep) {
    if (ep->engr_txt && ep->engr_txt[0]) {
      len = StrLen(ep->engr_txt);
      if (len & 0x1) len++; // keep it EVEN.
      offset += sizeof(Short);
      offset += sizeof(engr_t);
      offset += len * sizeof(Char);
    }
    ep = ep->nxt_engr;
  }
  offset += sizeof(Short);
  return offset;
}
// Do the actual saving, and free them at the same time
Short save_engravings(VoidPtr p, Short offset)
{
  Short len, tmp;
  engr_t *ep = head_engr, *next_ep;
  while (ep) {
    next_ep = ep->nxt_engr;
    if (ep->engr_txt) {
      if (ep->engr_txt[0]) {
	len = StrLen(ep->engr_txt);
	if (len & 0x1) len++; // keep it EVEN.
	DmWrite(p, offset, &len, sizeof(Short));  offset += sizeof(Short);
	DmWrite(p, offset, ep, sizeof(engr_t));   offset += sizeof(engr_t);
	DmWrite(p, offset, ep->engr_txt, sizeof(Char) * StrLen(ep->engr_txt));
	offset += len * sizeof(Char);
      }
      free_me((VoidPtr) ep->engr_txt); // Originally wasn't being freed?!
    }
    free_me((VoidPtr) ep);             // Originally wasn't being freed?!
    ep = next_ep;
  }
  head_engr = NULL;
  tmp = CHAIN_TERMINATOR;
  DmWrite(p, offset, &tmp, sizeof(Short));
  offset += sizeof(Short);
  return offset;
}

// restore saved engravings.  they'll end up in reverse order.  no big deal.
void rest_engravings(VoidPtr *p)
{
  engr_t *ep;
  Short len = 0;
  head_engr = NULL;
  while (true) {
    len = *((Short *) *p);
    *p += sizeof(Short);
    if (len == 0 || len == CHAIN_TERMINATOR) return;

    ep = (engr_t *) md_malloc(sizeof(engr_t));
    *ep = *((engr_t *)(*p));
    *p += sizeof(engr_t);

    ep->engr_txt = (Char *) md_malloc(len + 1);
    StrNCopy(ep->engr_txt, (Char *)(*p), len);
    ep->engr_txt[len-1] = '\0';
    *p += sizeof(Char) * len;

    ep->nxt_engr = head_engr;
    head_engr = ep;
  }
}


static void del_engr(engr_t *ep)
{
  engr_t *ept;
  if (ep == head_engr) {
    head_engr = ep->nxt_engr;
    if (ep->engr_txt) free_me((VoidPtr) ep->engr_txt);
    free_me((VoidPtr) ep);
    return;
  }
  for (ept = head_engr ; ept ; ept = ept->nxt_engr) {
    if (ept->nxt_engr == ep) {
      ept->nxt_engr = ep->nxt_engr;
      if (ep->engr_txt) free_me((VoidPtr) ep->engr_txt);
      free_me((VoidPtr) ep);
      return;
    }
  }
  message("BUG: Error in del_engr?");
  return;
}
