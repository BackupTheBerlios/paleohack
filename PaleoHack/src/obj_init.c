/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
#include "bit.h"

const Char obj_symbols[NUM_OBJ_SYMBOLS+1] = {
  ILLOBJ_SYM, AMULET_SYM, FOOD_SYM, WEAPON_SYM, TOOL_SYM,
  BALL_SYM, CHAIN_SYM, ROCK_SYM, ARMOR_SYM, POTION_SYM, SCROLL_SYM,
  WAND_SYM, RING_SYM, GEM_SYM, 0 }; // 15 bytes.

Short bases[NUM_OBJ_SYMBOLS+1]; // 15*2 = 30 bytes.  affordable.
static UChar gem_probability[MAX_GEMS]; // 19 bytes.  tolerable..

static void setgemprobs() SEC_1;

Short letindex(Char let)
{
  Short i = 0;
  Char ch;
  while ((ch = obj_symbols[i++]) != 0)
    if (ch == let)
      return i;
  return 0;
}


// This is called when a new character is created.
void init_objects()
{
  Short i, j, first, last, end;
  Char c;
  Short tmp;
  
  //initialize known-ness of object classes... zero all bytes, set some bits.
  for (j = 0 ; j < MAX_OC_NK ; j++)
    oc_name_known[j] = 0;
  for (i = 0 ; i < MAX_OBJCLASS ; i++)
    if (objects[i].nameknown_merge & O_CL_NAMEKNOWN) // initial values...
      BITSET(oc_name_known, i);

  /* init base; if probs given check that they add up to 100, 
     otherwise compute probs; shuffle descriptions */
  for (i=0 ; i < sizeof(obj_symbols) ; i++)
    bases[i] = 0;
  first = 0;
  end = MAX_OBJCLASS;  //  end = SIZE(objects);
  rec_ocdescr_init(); 
  rec_uname_init(); // <-- erase all "call"s.
  while ( first < end ) {
    c = objects[first].oc_olet;
    // Set 'last' to the next object with a different olet (or a NULL name).
    last = first+1;
    while (last < end &&
	   objects[last].oc_olet == c &&
	   objects[last].oc_name_offset >= 0)
      last++;
    i = letindex(c);
    if ((!i && c != ILLOBJ_SYM) || bases[i] != 0) {
      alert_message("initialization error");
      return;
    }
    bases[i] = first;

    if (c == GEM_SYM)
      setgemprobs();

    // Make sure that the probabilities for this letter add up to 100.
    // Actually, the database-generator is now doing that!  Just trust it.

    if (objects[first].oc_descr_offset >= 0 && c != TOOL_SYM) {
      /* shuffle, also some additional descriptions */
      while (last < end && objects[last].oc_olet == c)
	last++;
      j = last;
      while (--j > first) {
	i = first + rund(j+1-first);
	tmp = oc_descr_offset[j];
	rec_ocdescr_set(j, oc_descr_offset[i]);
	rec_ocdescr_set(i, tmp);
      }
    }
    first = last;
  }
}


// we'll special-case this for gems, since their probability changes
// per level and I don't feel like writing to the const database.
Short probtype(Char c)
{
  Short i = bases[letindex(c)];
  Short prob = rund(100);
  if (c == GEM_SYM) {
    Short j = 0;
    while ((prob -= gem_probability[j++]) >= 0)
      i++;
  } else {
    while ((prob -= objects[i].oc_prob) >= 0)
      i++;
  }
  if ((objects[i].oc_olet != c) || (objects[i].oc_name_offset < 0)) {
    //panic("probtype(%c) error, i=%d", c, i);
    alert_message("DEBUG: error in probtype");
    return 0;
  }
  return i;
}

//static void printgemprobs();
// Aieee.   ok.. stay calm... there are only about 20 gems anyway.
// Let's just give them their own array, eh.
// And RECOMPUTE the fscker when you re-enter paleohack - no need to save it.
static void setgemprobs()
{
  Short j,first, skip;

  first = bases[letindex(GEM_SYM)];

  // we're actually not going to touch the probability of the last 4
  // (the worthless chunks of glass are 20,20,20,20);
  // we'll divide the reamining 20% among all of the worthful gems.
  for (j = 0 ; j < MAX_GEMS ; j++) 
    gem_probability[j] = objects[first+j].oc_prob;

  //  printgemprobs(); // now: 10 1's, 5 2's, and 4 20's.  CORRECT.

  // good gems are in front.. their probability is 0 until you get deeper..
  for (j = 0; j < 9-dlevel/3; j++)
    gem_probability[j] = 0;
  skip = j;//  first += j;

  //  printgemprobs(); // now: 9 0's, etc.  CORRECT.

  // well, I trust this will not happen, since there are more than 9 gems:
  if (first+skip >= LAST_GEM || first+skip >= MAX_OBJCLASS /*SIZE(objects)*/ ||
      objects[first+skip].oc_olet != GEM_SYM ||
      objects[first+skip].oc_name_offset < 0)
    alert_message("DEBUG: Not enough gems?");

  // The 20 there is because we are divvying-up 20%, not 100%, of probability.
  // (Hmmm.  We erased at most 10% of the original 100% so it's going to
  // add up to more than 100: last gem gets less likely as dlevel increases.
  // On the other hand, this is integer divide, so we're losing some.)
  for (j = skip; j+first < LAST_GEM; j++)
    gem_probability[j] = (20+j)/(LAST_GEM-(j+first));
  //  printgemprobs(); // now: 9 0's; 4, 6, 7, 10, 16, 34; 20, 20, 20, 20
                   // (/ 29 6) ... (/ 34 1).
}
/*
static void printgemprobs()
{
  Short j;
  Char buf[80], tmp[10];
  buf[0] = '\0';
  for (j = 0 ; j < MAX_GEMS ; j++) {
    StrPrintF(tmp, "%d, ", gem_probability[j]);
    StrCat(buf, tmp);
  }
  message(buf);
}
*/



// Ok, I decree that this must be called whenever you enter paleohack
// with an existing character.  It will just rebuild gem_probability,
// which is not saved.
void oinit()                    /* level dependent initialization */
{
  setgemprobs();
}



// savenames, restnames ...  dodiscovered ...




Boolean dodiscovered()
{
  // ... This will need a form that we dump some text to.
  // basically like the messagelog form.
  return false;
}

// needed for dodiscovered.
//static Boolean interesting_to_discover(Short i)
Boolean interesting_to_discover(Short i)
{
  if (oc_has_uname(i))
    return true; // something the PLAYER has named
  if (BITTEST(oc_name_known, i) &&
      (oc_descr_offset[i] >= 0))
    return true; // something with a description
  return false;
}
