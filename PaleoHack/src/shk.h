/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
// This is a bill... each shopkeeper could have up to 200 of them.
// Used in shk.c
//
#define	BILLSZ	200
typedef struct bill_x {
  UShort bo_id;  // (this matches obj->o_id)
  Boolean useup; // I should make this take the sign bit of bquantity!
  UChar bquantity; // Only needs 7 bits.
  UShort price;    /* price per unit */ // Need a Long if armor AC bonus > 28..
} bill_t;
// A shopkeeper.  Used in mkshop.c and shk.c
//
typedef struct eshk {
  Long robbed;	/* amount stolen by most recent customer */
  Boolean following;	/* following customer since he owes us sth */
  Int8 shoproom;		/* index in rooms; set by inshop() */
  coord shk;		/* usual position shopkeeper */
  PointType shd;		/* position shop door */
  Short shoplevel;		/* level of his shop */
  Short billct;
  bill_t bill[BILLSZ];
  Short visitct;		/* nr of visits by most recent customer */
  Char customer[PL_NSIZ];	/* most recent customer */
  Char shknam[PL_NSIZ];
} eshk_t;

