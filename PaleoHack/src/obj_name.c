/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
#include "bit.h"

#define PREFIX    15
// BUFSZ is defined in some header file now.  do we really need 256???
static void singularize(Char *bp) SEC_4;
static obj_t *create_wish_obj(Short typ, Short heavy, Short cnt,
			      Short spe, Short spesgn) SEC_4;
extern Short bases[]; // need for readobjnam

Boolean is_vowel(Char v)
{
  return (v=='a' || v=='e' || v=='i' || v=='o' || v=='u');
}


static Char * strprepend(Char *s, Char *pref)
{
  Short i = StrLen(pref);
  if (i > PREFIX) {
    alert_message("DEBUG: xname prefix space too short.");
    return s;
  }
  s -= i;
  StrNCopy(s, pref, i);	/* do not copy trailing 0 */
  return s;
}

// Char * typename(Short otyp) : is ONLY used for "what have I discovered?".
// the caller won't use ScratchBuffer, so it's ok to use it in here:
Char * typename(Short otyp)
{
  //  static Char buf[BUFSZ];
  Char *buf = ScratchBuffer;
  Char *an = NULL; //ocl->oc_name;
  Char *dn = NULL; //ocl->oc_descr;
  Char *un = NULL; //ocl->oc_uname;
  Short name_offset = objects[otyp].oc_name_offset;
  Short descr_offset = oc_descr_offset[otyp];
  Boolean nn = BITTEST(oc_name_known, otyp);
  //Short nn = ocl->oc_name_known;
  if (name_offset >= 0)    an = oc_names + name_offset;
  if (descr_offset >= 0)   dn = oc_descrs + descr_offset; 
  if (oc_has_uname(otyp))  un = oc_get_uname(otyp);
  switch(objects[otyp].oc_olet) {
  case POTION_SYM:   StrCopy(buf, "potion");   break;
  case SCROLL_SYM:   StrCopy(buf, "scroll");   break;
  case WAND_SYM:     StrCopy(buf, "wand");     break;
  case RING_SYM:     StrCopy(buf, "ring");     break;
  default:
    if (nn) {
      StrCopy(buf, an);
      if (otyp >= TURQUOISE && otyp <= JADE)  StrCat(buf, " stone");
      if (un)                  StrPrintF(buf+StrLen(buf), " called %s", un);
      if (dn)                  StrPrintF(buf+StrLen(buf), " (%s)", dn);
    } else {
      StrCopy(buf, dn ? dn : an);
      if (objects[otyp].oc_olet == GEM_SYM)   StrCat(buf, " gem");
      if (un)                  StrPrintF(buf+StrLen(buf), " called %s", un);
    }
    StrCat(buf, "\n");
    return(buf);
  }
  /* here for ring/scroll/potion/wand */
  if (nn && an)    StrPrintF(buf+StrLen(buf), " of %s", an);
  if (un)    StrPrintF(buf+StrLen(buf), " called %s", un);
  if (dn)    StrPrintF(buf+StrLen(buf), " (%s)", dn);
  StrCat(buf, "\n");
  return(buf);
}
// Call before you call typename if you want to know how long it'll be.
Short typename_len(Short otyp)
{
  Short len = 1;
  Char *an = NULL; //ocl->oc_name;
  Char *dn = NULL; //ocl->oc_descr;
  Char *un = NULL; //ocl->oc_uname;
  Short name_offset = objects[otyp].oc_name_offset;
  Short descr_offset = oc_descr_offset[otyp];
  Boolean nn = BITTEST(oc_name_known, otyp);
  //Short nn = ocl->oc_name_known;
  if (name_offset >= 0)    an = oc_names + name_offset;
  if (descr_offset >= 0)   dn = oc_descrs + descr_offset; 
  if (oc_has_uname(otyp))  un = oc_get_uname(otyp);
  switch(objects[otyp].oc_olet) {
  case POTION_SYM:   
  case SCROLL_SYM:   len += 6;   break;
  case WAND_SYM:     
  case RING_SYM:     len += 4;   break;
  default:
    if (nn) {
      len += StrLen(an);
      if (otyp >= TURQUOISE && otyp <= JADE)  len += 6;
      if (un)                                 len += 8 + StrLen(un);
      if (dn)                                 len += 3 + StrLen(dn);
    } else {
      len += (dn ? StrLen(dn) : StrLen(an));
      if (objects[otyp].oc_olet == GEM_SYM)   len += 4;
      if (un)                                 len += 8 + StrLen(un);
    }
    return ++len;
  }
  /* here for ring/scroll/potion/wand */
  if (nn && an)    len += 4 + StrLen(an);
  if (un)    len += 8 + StrLen(un);
  if (dn)    len += 3 + StrLen(dn);
  return ++len;
}


static Char xname_bufr[BUFSZ];
Char * xname(obj_t *obj)
{
  Char *buf = &(xname_bufr[PREFIX]);	/* leave room for "17 -3 " */
  Char *an = NULL;
  Char *dn = NULL;
  Char *un = NULL;
  Boolean plural = (obj->quantity != 1);
  Short name_offset = objects[obj->otype].oc_name_offset;
  Short descr_offset = oc_descr_offset[obj->otype];
  Boolean nn = BITTEST(oc_name_known, obj->otype);

  if (name_offset >= 0)    an = oc_names + name_offset;
  if (descr_offset >= 0)   dn = oc_descrs + descr_offset; 
  if (oc_has_uname(obj->otype))  un = oc_get_uname(obj->otype);

  if (!(obj->bitflags & O_IS_DESCKNOWN) && !Blind)
    obj->bitflags |= O_IS_DESCKNOWN; /* %% doesnt belong here */

  switch(obj->olet) {
  case AMULET_SYM:
    StrCopy(buf, ((obj->spe < 0) && (obj->bitflags & O_IS_KNOWN))
	   ? "cheap plastic imitation of the " : "");
    StrCat(buf,"Amulet of Yendor");
    break; // 
  case TOOL_SYM:
    if (!nn)  StrCopy(buf, dn);
    else      StrCopy(buf, an);
    break;
  case FOOD_SYM:
    if (obj->otype == DEAD_HOMUNCULUS && plural) {
      plural = false;
      StrCopy(buf, "dead homunculi");
      break;
    }
    /* fungis ? */
    /* fall into next case */
  case WEAPON_SYM:
    if (obj->otype == WORM_TOOTH && plural) {
      plural = false;
      StrCopy(buf, "worm teeth");
      break;
    }
    if (obj->otype == CRYSKNIFE && plural) {
      plural = false;
      StrCopy(buf, "crysknives");
      break;
    }
    /* fall into next case */
  case ARMOR_SYM:
  case CHAIN_SYM:
  case ROCK_SYM:
    StrCopy(buf, an);
    break;
  case BALL_SYM:
    if (obj->owt > objects[obj->otype].oc_weight)
      StrCopy(buf, "very heavy iron ball");
    else
      StrCopy(buf, "heavy iron ball");
    break;
  case POTION_SYM:
    // (third condition is not impossible if blind)
    if (nn || un || !(obj->bitflags & O_IS_DESCKNOWN)) {
      StrCopy(buf, "potion");
      if (plural) {
	plural = false;
	StrCat(buf, "s");
      }
      if (!(obj->bitflags & O_IS_DESCKNOWN)) break;
      if (un) {
	StrCat(buf, " called ");
	StrCat(buf, un);
      } else {
	StrCat(buf, " of ");
	StrCat(buf, an);
      }
    } else {
      StrCopy(buf, dn);
      StrCat(buf, " potion");
    }
    break;
  case SCROLL_SYM:
    StrCopy(buf, "scroll");
    if (plural) {
      plural = false;
      StrCat(buf, "s");
    }
    if (!(obj->bitflags & O_IS_DESCKNOWN)) break;
    if (nn) {
      StrCat(buf, " of ");
      StrCat(buf, an);
    } else if (un) {
      StrCat(buf, " called ");
      StrCat(buf, un);
    } else {
      StrCat(buf, " labeled ");
      StrCat(buf, dn);
    }
    break;
  case WAND_SYM:
    if (!(obj->bitflags & O_IS_DESCKNOWN))
      StrPrintF(buf, "wand");
    else if (nn)
      StrPrintF(buf, "wand of %s", an);
    else if (un)
      StrPrintF(buf, "wand called %s", un);
    else
      StrPrintF(buf, "%s wand", dn);
    break;
  case RING_SYM:
    if (!(obj->bitflags & O_IS_DESCKNOWN))
      StrPrintF(buf, "ring");
    else if (nn)
      StrPrintF(buf, "ring of %s", an);
    else if (un)
      StrPrintF(buf, "ring called %s", un);
    else
      StrPrintF(buf, "%s ring", dn);
    break;
  case GEM_SYM:
    if (!(obj->bitflags & O_IS_DESCKNOWN)) {
      StrCopy(buf, "gem");
      break;
    }
    if (!nn) {
      StrPrintF(buf, "%s gem", dn);
      break;
    }
    StrCopy(buf, an);
    if (obj->otype >= TURQUOISE && obj->otype <= JADE)
      StrCat(buf, " stone");
    break;
  default: // bug if we reach this
    StrPrintF(buf,"glorkum %c (%d) %u %d",
	    obj->olet, obj->olet, obj->otype, obj->spe);
  }

  if (plural) {
    Char *p;
    Boolean fixed = false;

    for (p = buf ; *p && !fixed ; p++) {
      if (!StrNCompare(" of ", p, 4)) {
				/* pieces of, cloves of, lumps of */
	Short c1, c2 = 's';

	do { // scoot everything down...
	  c1 = c2; c2 = *p; *p++ = c1;
	} while (c1);
	fixed = true;
      }
    }
    if (!fixed) {
      Char v;
      p = buf + (StrLen(buf)-1); // last character; so *(p+1) is the null
      v = *(p-1); // next-to-last character, maybe a vowel.
      if (*p == 's' || *p == 'z' || *p == 'x' ||
	  (*p == 'h' && p[-1] == 's'))
	StrCat(buf, "es");	/* boxes */
      else if (*p == 'y' && !is_vowel(v))
	StrCopy(p, "ies");	/* rubies, zruties */
      else
	StrCat(buf, "s");
    }
  } // ok we pluralized it.

  if (ONAME(obj) != NULL) {
    StrCat(buf, " named ");
    StrCat(buf, ONAME(obj));  // Not tested yet.
  }
  return buf;
}


Char * doname(obj_t *obj)
{
  Char prefix[PREFIX];
  Char *bp = xname(obj); // returns a pointer into xname_bufr

  if (obj->quantity != 1)
    StrPrintF(prefix, "%u ", obj->quantity);
  else
    StrCopy(prefix, "a ");
  switch(obj->olet) {
  case AMULET_SYM:
    if (0 != StrNCompare(bp, "cheap ", 6))
      StrCopy(prefix, "the ");
    break;
  case ARMOR_SYM:
    if (obj->owornmask & W_ARMOR)
      StrCat(bp, " (being worn)");
    /* fall into next case */
  case WEAPON_SYM:
    if (obj->bitflags & O_IS_KNOWN) {
      StrPrintF(prefix + StrLen(prefix),
		(obj->spe < 0) ? "%d " : "+%d ", obj->spe);
      //      StrCat(prefix, sitoa(obj->spe));
      //      StrCat(prefix, " ");
    }
    break;
  case WAND_SYM:
    if (obj->bitflags & O_IS_KNOWN)
      StrPrintF(bp + StrLen(bp), " (%d)", obj->spe);
    break;
  case RING_SYM:
    if (obj->owornmask & W_RINGR) StrCat(bp, " (on right hand)");
    if (obj->owornmask & W_RINGL) StrCat(bp, " (on left hand)");
    if ((obj->bitflags & O_IS_KNOWN) && (objects[obj->otype].bits & SPEC)) {
      StrPrintF(prefix + StrLen(prefix),
		(obj->spe < 0) ? "%d " : "+%d ", obj->spe);
      //      StrCat(prefix, sitoa(obj->spe));
      //      StrCat(prefix, " ");
    }
    break;
  }
  if (obj->owornmask & W_WEP)
    StrCat(bp, " (weapon in hand)");
  if (obj->bitflags & O_IS_UNPAID)
    StrCat(bp, " (unpaid)");
  if (!StrCompare(prefix, "a ") && is_vowel(*bp))
    StrCopy(prefix, "an ");
  bp = strprepend(bp, prefix);
  return bp;
}


// Note that this has a bit of verb-grammar hardcoded in it.
Char * aobjnam(obj_t *otmp, Char *verb)
{
  Char *bp = xname(otmp);
  Char prefix[PREFIX];
  if (otmp->quantity != 1) {
    StrPrintF(prefix, "%u ", otmp->quantity);
    bp = strprepend(bp, prefix);
  }

  if (verb) {
    /* verb is given in plural (i.e., without trailing s) */
    StrCat(bp, " ");
    if (otmp->quantity != 1)
      StrCat(bp, verb);
    else if (0==StrCompare(verb, "are"))
      StrCat(bp, "is");
    else {
      StrCat(bp, verb);
      StrCat(bp, "s");
    }
  }
  return bp;
}


// This is just a capitalized doname(obj) .... heh.
Char * Doname(obj_t *obj) // see also: Monnam
{
  Char *bp = doname(obj);
  if ('a' <= *bp && *bp <= 'z') *bp += ('A' - 'a');
  return(bp);
}


// obj_t * readobjnam(Char *bp) : is used only in zap.c to Wish For An Object.
Char *wrp[] = { "wand", "ring", "potion", "scroll", "gem" };
Char wrpsym[] = { WAND_SYM, RING_SYM, POTION_SYM, SCROLL_SYM, GEM_SYM };
#define digit(c) ((c) >= '0' && (c) <= '9')
obj_t * readobjnam(Char *bp)
{
  Char *p;
  Short i;
  Short cnt, spe, spesgn, typ, heavy;
  Char let;
  Char *un, *dn, *an;
  /* int the = 0; char *oname = 0; */
  cnt = spe = spesgn = typ = heavy = 0;
  let = 0;
  an = dn = un = 0;

  // First: Make it lower case.
  for (p = bp ; *p ; p++) if ('A' <= *p && *p <= 'Z') *p += 'a'-'A';
  // Second: Figure out how many to make.
  if (!StrNCompare(bp, "the ", 4))   {/* the=1; */ bp += 4; }
  else if (!StrNCompare(bp, "an ", 3)) { cnt = 1;  bp += 3; }
  else if (!StrNCompare(bp, "a ", 2))  { cnt = 1;  bp += 2; }
  if (!cnt && digit(*bp)) {
    cnt = StrAToI(bp); // hope it works like atoi.
    while (digit(*bp)) bp++;
    while (*bp == ' ') bp++;
  }
  if (!cnt) cnt = 1;		/* %% what with "gems" etc. ? */

  // Third: Check for +%d || -%d at the front, or (+%d) || (-%d) at the end.
  if (*bp == '+' || *bp == '-') {
    spesgn = (*bp++ == '+') ? 1 : -1;
    spe = StrAToI(bp);
    while (digit(*bp)) bp++;
    while (*bp == ' ') bp++;
  } else {
    p = my_rindex(bp, '(');
    if (p) {
      if (p > bp && p[-1] == ' ') p[-1] = '\0';
      else *p = '\0';
      p++;
      spe = StrAToI(p);
      while (digit(*p)) p++;
      if (StrCompare(p, ")")) spe = 0; // no close parenthesis, no bonus.
      else spesgn = 1;
    }
  }
  /* now we have the actual name, as delivered by xname, say
     green potions called whisky
     scrolls labeled "QWERTY"
     egg
     dead zruties
     fortune cookies
     very heavy iron ball named hoei
     wand of wishing
     elven cloak
  */
  for (p = bp; *p; p++)  if (!StrNCompare(p, " named ", 7)) {
    *p = 0;
    /*		oname = p+7; */  // Guess we don't care about "name".
  }
  for (p = bp; *p; p++)  if (!StrNCompare(p, " called ", 8)) {
    *p = 0;
    un = p+8;
  }
  for (p = bp; *p; p++)  if (!StrNCompare(p, " labeled ", 9)) {
    *p = 0;
    dn = p+9;
  }

  /* first, change to singular if necessary */
  if (cnt != 1)
    singularize(bp);

  if (!StrCompare(bp, "amulet of yendor")) {
    typ = AMULET_OF_YENDOR;
    return create_wish_obj(typ, heavy, cnt, spe, spesgn);
  }
  p = bp+StrLen(bp); // was eos(bp)
  if (!StrCompare(p-5, " mail")) { /* Note: ring mail is not a ring ! */
    let = ARMOR_SYM;
    an = bp;
    goto srch;
  }
  for (i = 0; i < sizeof(wrpsym); i++) {
    Short j = StrLen(wrp[i]);
    if (!StrNCompare(bp, wrp[i], j)) {
      let = wrpsym[i];
      bp += j;
      if (!StrNCompare(bp, " of ", 4)) an = bp+4;
      /* else  if (*bp) ?? */
      goto srch;
    }
    if (!StrCompare(p-j, wrp[i])) {
      let = wrpsym[i];
      p -= j;
      *p = 0;
      if (p[-1] == ' ') p[-1] = 0;
      dn = bp;
      goto srch;
    }
  }
  if (!StrCompare(p-6, " stone")) {
    p[-6] = 0;
    let = GEM_SYM;
    an = bp;
    goto srch;
  }
  if (!StrCompare(bp, "very heavy iron ball")) {
    heavy = 1;
    typ = HEAVY_IRON_BALL;
    return create_wish_obj(typ, heavy, cnt, spe, spesgn);
  }
  an = bp;

 srch:
  if (an || dn || un) {
    i = 1;
    if (let) i = bases[letindex(let)];
    for ( ; i <= NROFOBJECTS && (!let || objects[i].oc_olet == let) ; i++) {
      Short offset;
      Char *zn = NULL;
      if ((offset = objects[i].oc_name_offset) >= 0) zn = oc_names + offset;
      if (!zn) continue;
      if (an && StrCompare(an, zn))
	continue;
      if (dn) {
	zn = NULL;
	if ((offset = objects[i].oc_descr_offset) >= 0) zn = oc_descrs+offset;
	if (!zn || StrCompare(dn, zn))
	  continue;
      }
      if (un) {
	zn = NULL;
	if (oc_has_uname(i)) zn = oc_get_uname(i);
	if (!zn || StrCompare(un, zn))
	  continue;
      }
      typ = i;
      return create_wish_obj(typ, heavy, cnt, spe, spesgn);
    }
  }
  if (!let) let = wrpsym[rund(sizeof(wrpsym))];
  typ = probtype(let);

  return create_wish_obj(typ, heavy, cnt, spe, spesgn);
  // Hey...
  // You can wish for a dead cockatrice, and NOT turn into stone...
  // until you drop it and pick it up.  heh.
}

//////////////////////////
// Remove plural suffixes: -ies, -es, -s, knives, boxes, homunculi, teeth.
static void singularize(Char *bp)
{
  Char *p;
  /* find "cloves of garlic", "worthless pieces of blue glass" */
  for (p = bp; *p; p++)  if (!StrNCompare(p, "s of ", 5)) {
    while ((*p = p[1])) p++; // erase the "s" by shifting everything back.
    return;
  }
  /* remove -s or -es (boxes) or -ies (rubies, zruties) */
  p = bp+StrLen(bp); // was eos(bp)
  if (p[-1] == 's') {
    // "-s" plural suffixes:
    if (p[-2] == 'e') {
      // "-es" plural suffixes:
      if (p[-3] == 'i') {
	if (!StrCompare(p-7, "cookies"))
	  p[-1] = '\0';
	else StrCopy(p-3, "y");
	return;
      } // fi (-ies)
				/* note: cloves / knives from clove / knife */
      if (!StrCompare(p-6, "knives")) {
	StrCopy(p-3, "fe");
	return;
      } // fi (knives)
				/* note: nurses, axes but boxes */
      if (!StrCompare(p-5, "boxes")) {
	p[-2] = 0;
	return;
      } // fi (boxes)
    } else {
      // "-s" but not "-es"
      p[-1] = '\0';
    }
  } else {
    // Non-"-s" plural suffixes:
    if (!StrCompare(p-9, "homunculi")) {
      StrCopy(p-1, "us"); /* !! makes string longer */
      return;
    }
    if (!StrCompare(p-5, "teeth")) {
      StrCopy(p-5, "tooth");
      return;
    }
    /* here we cannot find the plural suffix */
  }
}


static obj_t *create_wish_obj(Short typ, Short heavy, Short cnt,
			      Short spe, Short spesgn)
{
  Char let;
  obj_t *otmp;
  let = objects[typ].oc_olet;
  otmp = mksobj(typ);
  if (heavy)
    otmp->owt += 15;
  if (cnt > 0 && StrChr("%?!*)", let) &&
      (cnt < 4 || (let == WEAPON_SYM && typ <= ROCK && cnt < 20)))
    otmp->quantity = cnt;

  if (spe > 3 && spe > otmp->spe)
    spe = 0;
  else  if (let == WAND_SYM)
    spe = otmp->spe;
  if (spe == 3 && you.uluck < 0)
    spesgn = -1;
  if (let != WAND_SYM && spesgn == -1)
    spe = -spe;
  if (let == BALL_SYM)
    spe = 0;
  else  if (let == AMULET_SYM)
    spe = -1;
  else  if (typ == WAN_WISHING && rund(10))
    spe = (rund(10) ? -1 : 0);
  otmp->spe = spe;

  if (spesgn == -1)
    otmp->bitflags |= O_IS_CURSED; // otmp->cursed = 1;

  return(otmp);
}
