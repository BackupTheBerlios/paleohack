/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"

extern Char plname[PL_NSIZ];

static Char * xmonnam(monst_t *mtmp, Short vb);// SEC_1;

void do_mname(monst_t *mtmp, Char *new_name)
{
  Short len;
  if (!mtmp) return;
  if (mtmp->name != NULL) {
    free_me((VoidPtr) mtmp->name);
    mtmp->name = NULL;
  }
  if (!new_name) return;
  len = StrLen(new_name) + 1;
  if (len <= 1) return;
  if (len > MAX_MNAME) len = MAX_MNAME;
  mtmp->name = (Char *) md_malloc(len * sizeof(Char));
  StrNCopy(mtmp->name, new_name, len);
  mtmp->name[len-1] = '\0';
}

static void do_gname(monst_t *mtmp, Char *new_name) // ghost name
{
  Short len;
  if (!mtmp) return;
  if (mtmp->extra != NULL) return; // bug if this happens
  if (!new_name) return; // bug if this happens
  len = StrLen(new_name) + 1;
  if (len <= 1) return;
  if (len > MAX_MNAME) len = MAX_MNAME;
  mtmp->extra = (Char *) md_malloc(len * sizeof(Char));
  StrNCopy(mtmp->extra, new_name, len);
  ((Char *) (mtmp->extra))[len-1] = '\0';
}

void do_name(obj_t *otmp, Char *new_name)
{
  Short len;
  if (!otmp) return;
  if (ONAME(otmp) != NULL) {
    free_me((VoidPtr) otmp->oextra);
    otmp->oextra = NULL;
  }
  if (!new_name) return;
  len = StrLen(new_name) + 1;
  if (len <= 1) return;
  if (len > MAX_ONAME) len = MAX_ONAME;
  otmp->oextra = (Char *) md_malloc(len * sizeof(Char));
  StrNCopy(otmp->oextra, new_name, len);
  otmp->oextra[len-1] = '\0';
}
// For do_call, we need a canonical mapping from
// object class index to  an index from 0 to CALLABLE_OBJS-1.
// I think that potion/scroll/wand/ring are consecutive.  cool.
// So I just need the index of "first potion", and we subtract, maybe.
void do_call(obj_t *otmp, Char *new_name)
{
  Short i;
  if (!otmp) return;
  if (otmp->otype < FIRST_CALLABLE || otmp->otype > LAST_CALLABLE) return;
  i = otmp->otype - FIRST_CALLABLE;
  rec_uname_edit(new_name, i);
  // don't need to set oc_uname_index thingy anymore.
}

// Return true if this object type has a uname
Boolean oc_has_uname(Short otype)
{
  Char *name;
  if (otype < FIRST_CALLABLE || otype > LAST_CALLABLE) return false;
  name = rec_uname_get(otype - FIRST_CALLABLE);
  return (name != NULL);
}
Char *oc_get_uname(Short otype)
{
  if (otype < FIRST_CALLABLE || otype > LAST_CALLABLE) return NULL;
  return rec_uname_get(otype - FIRST_CALLABLE);
}


// How big could a monster name possibly get?  They're 17 characters
// in the database, right?
// 17 + strlen("the invisible ") + strlen(" called ") + max called length...
// say 17+23 = 40.  so 80 should be plenty.
#define MAX_XMONNAM 80
Char *ghostnames[] = {	/* these names should have length < PL_NSIZ */
  "adri",   "andries", "andreas", "bert", "david",
  "dirk",   "emile",   "frans",   "fred", "greg",
  "hether", "jay",     "john",    "jon",  "kay",
  "kenny",  "maud",    "michiel", "mike", "peter",
  "robert", "ron",     "tom",     "wilmar"
};
static Char * xmonnam(monst_t *mtmp, Short vb)
{
  static Char buf[MAX_XMONNAM];

  if (mtmp->name && !vb) {
    StrCopy(buf, mtmp->name);
    return buf;
  }
  switch(mtmp->data->mlet) {
  case ' ':
    {
      Char *gn = (Char *) mtmp->extra;
      if (!gn || !*gn) {		// might also look in scorefile
	gn = ghostnames[rund( sizeof(ghostnames)/sizeof(Char *) )];
	if (!rund(2)) { // strange but true.
	  do_gname(mtmp, (!rund(5) ? plname : gn));
	}
      }
      StrPrintF(buf, "%s's ghost", (gn ? gn : "A BUG"));
    }
    break;
  case '@':
    if (mtmp->bitflags & M_IS_SHOPKEEPER) {
      StrCopy(buf, shkname(mtmp));
      break;
    }
    // fall into next case
  default:

  StrPrintF(buf, "the%s%s",
	    (mtmp->bitflags & M_IS_INVISIBLE) ? " invisible " : " ",
	    mon_names + mtmp->data->mname_offset);
  }
  if (vb && mtmp->name) {
    StrCat(buf, " called ");
    StrCat(buf, mtmp->name);
  }
  return(buf);
}

Char * monnam(monst_t *mtmp)
{
  return xmonnam(mtmp, 0);
}

void lmonnam(monst_t *mtmp, Char *buf)
{
  StrPrintF(buf, "What do you want to call %s?", xmonnam(mtmp, 1));
}

// This is just a capitalized monnam(mtmp) .... heh.
Char * Monnam(monst_t *mtmp)
{
  Char *bp = monnam(mtmp);
  if ('a' <= *bp && *bp <= 'z') *bp += ('A' - 'a');
  return(bp);
}

Char * amonnam(monst_t *mtmp, Char *adj)
{
  Char *bp = monnam(mtmp);
  static Char buf[BUFSZ];		/* %% */ // xxx sigh

  if (!StrNCompare(bp, "the ", 4)) bp += 4;
  StrPrintF(buf, "the %s %s", adj, bp);
  return(buf);
}

Char * Amonnam(monst_t *mtmp, Char *adj)
{
  Char *bp = amonnam(mtmp,adj);

  *bp = 'T';
  return(bp);
}

Char * Xmonnam(monst_t *mtmp)
{
  Char *bp = Monnam(mtmp);
  if (!StrNCompare(bp, "The ", 4)) {
    bp += 2;
    *bp = 'A';
  }
  return(bp);
}
