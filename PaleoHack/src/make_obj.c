/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"

#define is_alpha(c) (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z'))



// Alloc a new obj and put it at the HEAD of fobj.  And return it.
// (Wrapper of wrapper of mksobj.)
obj_t * mkobj_at(Short symbol, Short x, Short y)
{
  obj_t *otmp = mkobj(symbol);
  otmp->ox = x;
  otmp->oy = y;
  otmp->nobj = fobj;
  fobj = otmp;
  return otmp;
}

// Alloc a new obj and put it at the HEAD of fobj.
// (Wrapper of mksobj.)
void mksobj_at(Short otyp, Short x, Short y)
{
  obj_t *otmp = mksobj(otyp);
  otmp->ox = x;
  otmp->oy = y;
  otmp->nobj = fobj;
  fobj = otmp;
}

// mkobj could be static except that it's called in character initialization..
// (Wrapper of mksobj.)
static const Char mkobjstr[] = "))[[!!!!????%%%%/=**))[[!!!!????%%%%/=**(%";
#undef MYDEBUG
obj_t * mkobj(Short c)
{
  Short i, o_type;
  // If you pass in 0, it will select a random object w/ above distribution.
#ifdef MYDEBUG
  Short oldc = c;
#endif
  if (!c) {
    i = rund(sizeof(mkobjstr) - 1);
    c = mkobjstr[i];
  }

  if (is_alpha(c)) {
    /*    i = (c > 'Z') ? (c-'a'+'Z'-'@'+1) : (c-'@'); */
    if (c <= 'Z')      i = (c - 'A') + 1;  // map [A-Z] to 1..26
    else      i = ((c - 'a') + 1) + (('Z' - 'A') + 1); // map [a-z] to 27..52
    o_type = CORPSE + i;
  } else
    o_type = probtype(c);

#ifdef MYDEBUG
  {
    Char buf[40];
    StrPrintF(buf, "%d %c (%d %d) %d", i, c, c, oldc, o_type);
    // OK!  THE GLORKUM created by me killing a gnome was:
    // "38 * 0", i c o_type.
    // THE GLORKUM created by me killing a jackal was:
    // "18 * (42 0) 0", i c (c oldc) o_type.
    // basically I think *'s are not getting created right. (gems.)
    // THE GLORKUM created by level creation was:
    // "18 * (42 0) 0".
    // Trying to fix gemprobability has not fixed this yet.
    // I rigged the Test menu item to create a '*' at you,you.
    // It created:
    // "15 * (42 42) 0" (the first %d, 'i', is uninitialized.)
    // So o_type is STILL NOT BEING SET PROPERLY for gems.
    message(buf);
  }
#endif

  return mksobj(o_type);
}


Long moves; // this needs to be in lock-externs.h or something

// otyp MUST be something sensible.
// argh, this could be static if it were not for Wand of Wishing in obj_name.c
obj_t * mksobj(Short otyp)
{
  obj_t *otmp;
  Char c = objects[otyp].oc_olet;

  otmp = (obj_t *) md_malloc(sizeof(obj_t)); // Who will free me?
  // md_malloc thoughtfully zeroes the memory for us.  no worries.
  otmp->age = moves;
  otmp->o_id = flags.ident++;
  otmp->quantity = 1;
  otmp->olet = c;
  otmp->otype = otyp;
  if (!(c=='/' || c=='=' || c=='!' || c=='?' || c=='*'))
    otmp->bitflags |= O_IS_DESCKNOWN; // else this bit stays turned off.
  switch(c) {
  case WEAPON_SYM:
    otmp->quantity = (otmp->otype <= ROCK) ? (rund(6)+6) : 1;
    if (!rund(11)) otmp->spe = rnd(3);
    else if (!rund(10)) {
      otmp->bitflags |= O_IS_CURSED;
      otmp->spe = -rnd(3);
    }
    break;
  case FOOD_SYM:
    if (otmp->otype >= CORPSE) break;
    // Else fall into next case.
    /*
#ifdef NOT_YET_IMPLEMENTED
    // if tins are to be identified, need to adapt doname() etc
    if(otmp->otype == TIN)
      otmp->spe = rnd(...);
#endif NOT_YET_IMPLEMENTED
    */
    // It was on fire when I lay down on it (it was commented in Hack 1.0.3.)
    //
    /* fall into next case */
  case GEM_SYM:
    otmp->quantity = rund(6) ? 1 : 2;
    break;
  case TOOL_SYM:
  case CHAIN_SYM:
  case BALL_SYM:
  case ROCK_SYM:
  case POTION_SYM:
  case SCROLL_SYM:
  case AMULET_SYM:
    break;
  case ARMOR_SYM:
    // Hmmm, it could be cursed AND have a positive 'spe' ???
    if (!rund(8))  otmp->bitflags |= O_IS_CURSED;
    if (!rund(10)) otmp->spe = rnd(3);
    else if (!rund(9)) {
      otmp->spe = -rnd(3);
      otmp->bitflags |= O_IS_CURSED;
    }
    break;
  case WAND_SYM:
    if (otmp->otype == WAN_WISHING) otmp->spe = 3;
    else {
      otmp->spe = rund(5);
      otmp->spe += (objects[otmp->otype].bits & NODIR) ? 11 : 4;
    }
    break;
  case RING_SYM:
    if (objects[otmp->otype].bits & SPEC) {
      if (!rund(3)) {
	otmp->bitflags |= O_IS_CURSED;
	otmp->spe = -rnd(2);
      } else otmp->spe = rnd(2);
    } else if (otmp->otype == RIN_TELEPORTATION ||
	       otmp->otype == RIN_AGGRAVATE_MONSTER ||
	       otmp->otype == RIN_HUNGER || !rund(9))
      otmp->bitflags |= O_IS_CURSED;
    break;
  default:
    alert_message("DEBUG: impossible mkobj");
  }
  otmp->owt = weight(otmp);
  return otmp;
}



Short weight(obj_t *obj)
{
  Short wt;
  Short base = objects[obj->otype].oc_weight;
  if (base)
    wt = base * obj->quantity;
  else
    wt = (obj->quantity + 1) / 2; // default base weight is "half a unit".
  return wt;
}



void mkgold(Long num, Short x, Short y)
{
  gold_t *gold;
  Long amount = (num ? num : 1 + (rnd(dlevel+2) * rnd(30)));

  if ((gold = gold_at(x,y)))
    gold->amount += amount;
  else {
    gold = (gold_t *) md_malloc(sizeof(gold_t));// newgold();
    gold->ngold = fgold;
    gold->gx = x;
    gold->gy = y;
    gold->amount = amount;
    fgold = gold;
    /*  do sth with display?  */
  }
}

