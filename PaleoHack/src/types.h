/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/

// The array of this is CONST and is found in the DATABASE:
typedef struct permonst {
  Short mname_offset;
  Char mlet;
  Char mlevel;
  Char mmove;
  Char ac;
  Char damn;
  Char damd;
  UShort pxlth; // maybe could get rid of this??
} permonst_t;

// The array of this is CONST and is found in the DATABASE:
typedef struct objclass {
  Short oc_name_offset;
  Short oc_descr_offset;
  UChar oc_uname_index; // <--  XXXX NO LONGER USED!  GET RID OF IT. XXXX
  UChar nameknown_merge; // note: nameknown is NOT const.  gar.
  Char oc_olet;
  Char oc_prob;  // this is sorta not const for gems.
  Char oc_delay;
  UChar oc_weight;
  Char oc_oc1;
  Char oc_oc2;
  Short oc_oi;
} objclass_t;
#define O_CL_MERGE     0x01
#define O_CL_NAMEKNOWN 0x02
#define nutrition       oc_oi   /* for foods */
#define a_ac            oc_oc1  /* for armors - only used in ARM_BONUS */
// ARM_BONUS is moved to another header file, after declaration of 'objects'.
#define a_can           oc_oc2  /* for armors */
#define bits            oc_oc1  /* for wands and rings */
#define wldam           oc_oc1  /* for weapons and PICK_AXE */
#define wsdam           oc_oc2  /* for weapons and PICK_AXE */
#define g_val           oc_oi   /* for gems: value on exit */
/* for wands, */
#define NODIR           1
#define IMMEDIATE       2
#define RAY             4
/* for rings, */
#define SPEC            1       /* +n is meaningful */



////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

/* INCOMPLETE */
#define FLG_MADEAMULET 0x01
typedef struct flag {
  UChar ident; // this is a uid counter! incremented when monsters created.

  // ...

  // hey there is only one of these structs, so for boolean values
  // that get used a lot, let them have a full boolean.
  // but, if they get used in like one place, give them just 1 bit.
  Boolean invlet_constant;// this should really only get 1 bit!  it's an option

  Boolean made_amulet; // this is used only in one function in make_maze.c !
  UChar moon_phase; // it only needs half of the bits though
  UChar no_of_wizards; // 0, 1, 2; can never decrease _back_ to 0 (?)

  Boolean beginner; // what's this for?


  // These are related to movement
  Boolean move;
  Boolean mv;
  Int8 run;         /* 0: h (etc), 1: H (etc), 2: fh (etc) */
                                /* 3: FH, 4: ff+, 5: ff-, 6: FF+, 7: FF- */
  Boolean nopick;      /* do not pickup objects */
  Boolean askpick;     /* ask before pickup objects */ // -- sprite!

  // tells you whether to redraw the stats. I might make it a masky thing.
  UChar botl;
} flags_t;
#define BOTL_NONE   0x00
#define BOTL_DLEVEL 0x01
#define BOTL_GOLD   0x02
#define BOTL_HP     0x04
//#define BOTL_HPMAX  0x08
#define BOTL_AC     0x10
#define BOTL_STR    0x20
#define BOTL_EXP    0x40
#define BOTL_HUNGER 0x80
#define BOTL_ALL    0xff


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

#define OUT_OF_BOUNDS(x,y) ((x) < 0 || (x) >= DCOLS || (y) < 0 || (y) >= DROWS)
// floor_symbol actually needs to be UChar so it can have EMDASH in-range!
extern UChar floor_symbol[DCOLS][DROWS];
extern UChar floor_info[DCOLS][DROWS];
// Use these for dissecting floor_info:
// it is a UChar nslttttt:   new, seen, lit, 5 bits of type.
#define NEW_CELL  0x80
#define SEEN_CELL 0x40
#define LIT_CELL  0x20
#define get_cell_new(a)   ((a) & NEW_CELL)
#define get_cell_seen(a)  ((a) & SEEN_CELL)
#define get_cell_lit(a)   ((a) & LIT_CELL)
#define get_cell_type(a)  ((a) & 0x1f)
#define set_cell_type(a,v) ((a) = (((a) & ~0x1f) | (v)))

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

typedef struct mkroom {
  Char lx;
  Char hx;
  Char ly;
  Char hy;    /* usually unsigned, but hx may be -1 */
  Char rtype;
  Boolean rlit;// I think that rlit is really a Boolean.
  Char door_ctr;
  Char fdoor;
} room_t; // 10 bytes?


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

#define SEEN_TRAP 0x40
#define ONCE_TRAP 0x20
#define get_trap_seen(a)  ((a) & SEEN_TRAP)
#define get_trap_once(a)  ((a) & ONCE_TRAP)
#define get_trap_type(a)  ((a) & 0x1f)
#define set_trap_type(a,v) ((a) = (((a) & ~0x1f) | (v)))
typedef struct trap {
  struct trap *ntrap;// Gaaah, trap is a list.  We do not like this!!
  Int8 tx;
  Int8 ty;
  UChar trap_info; // ttyp, tseen, once.
} trap_t;

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

typedef struct gold {
  struct gold *ngold;
  Int8 gx;
  Int8 gy;
  Long amount;
} gold_t;

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

// for bitflags:
#define O_IS_INVISIBLE   0x01
#define O_IS_DISPLAYED   0x02
#define O_IS_KNOWN       0x04
#define O_IS_DESCKNOWN   0x08
#define O_IS_CURSED      0x10
#define O_IS_UNPAID      0x20
#define O_IS_RUSTFREE    0x40
// for owornmask:
#define W_ARM   0x00000001L
#define W_ARM2  0x00000002L
#define W_ARMH  0x00000004L
#define W_ARMS  0x00000010L
#define W_ARMG  0x00000020L
#define W_ARMOR           (W_ARM | W_ARM2 | W_ARMH | W_ARMS | W_ARMG)
#define W_WEP   0x00001000L
#define W_BALL  0x00002000L
#define W_CHAIN 0x00004000L
#define W_RINGL 0x00010000L /* make W_RINGL = RING_LEFT (see uprop) */
#define W_RINGR 0x00020000L
#define W_RING            (W_RINGL | W_RINGR)
typedef struct obj {
  struct obj *nobj;
  UShort o_id;
  UShort o_container_id;
  Int8 ox, oy, odx, ody;
  UChar otype, owt;
  UChar quantity; // but use oextra instead for temporary gold objects?
  Int8 spe; // +/- of weap/arm/ring; #charges of want; special for uball,amulet
  Char olet, invlet;
  Long age;
  ULong owornmask; // more bit flags...  could this be a UShort instead???
  UChar bitflags; // oinvis, odispl, known, dknown, cursed, unpaid, rustfree.
  // again we have an odd number of chars, oh well.
  Char *oextra; // SIGH...
} obj_t; // size here is 32
#define ONAME(otmp) ((Char *) (otmp->oextra))


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

// for bitflags:
#define M_IS_MIMIC       0x0001
#define M_IS_DISPLAYED   0x0002
#define M_IS_INVISIBLE   0x0004
#define M_IS_CHAMELEON   0x0008
#define M_IS_HIDER       0x0010
#define M_IS_UNDETECTED  0x0020
#define M_IS_ASLEEP      0x0040
#define M_IS_FROZEN      0x0080
#define M_IS_CONFUSED    0x0100
#define M_IS_CANCELLED   0x0200
#define M_IS_TAME        0x0400
#define M_IS_PEACEFUL    0x0800
#define M_IS_SHOPKEEPER  0x1000
#define M_IS_GUARD       0x2000
#define M_IS_TRAPPED     0x4000
//
#define M_FLEEING        0x80 /* 0x8000 */
#define M_CAN_SEE        0x80 /* 0x8000 */
//
typedef struct {
  Int8 x;
  Int8 y;
} coord;// Would use PointType here but I think it's a Word apiece; wasteful.
#define MTRACK_SIZE 4
//
typedef struct monst {
  struct monst *nmon; // next monster in list
  permonst_t   *data;
  obj_t        *minvent;
  coord mtrack[MTRACK_SIZE];
  Int8 mx, my, mdx, mdy; /* if mdispl, then mdx,mdy is where last displayed */
  Int8 mhp, mhpmax;
  UShort m_id;            // unique id (the uid counter is in 'flags' struct)
  Long mlastmoved; // was "mlstmv" - I'd like to buy a vowel, Pat
  Long mgold;
  // Ok, here's my new policy: in this struct, I will do one-bit flags
  // with one another, and mostly leave larger ones to themselves.  mostly.
  UShort bitflags;/* mimic,mdispl, minvis,cham, mhide,mundetected, msleep,mfroz
		     mconf,mcan, mtame,mpeaceful, isshk,isgd, mtrapped! */
  UShort mtraps_seen;
  UChar mflee_and_time; // mflee (1) and mfleetim (7)
  UChar mcansee_and_blinded; // mcansee (1) and mblinded (7)
  UChar mspeed; // needs 2 bits.
  Char mappearance;
  // We have an odd number of chars, oh well..
  Int8 wormno; // ok, not an odd number anymore..
  //
  UShort extra_len; // needed for saving the 'extra' field
  //UChar mnamelth; // needs 6 bits.   ditto.  just check for name!=NULL.
  void *extra; // just remember to check this and free it....
  Char *name; // just remember to check this and free it....
} monst_t; // size == 48?


// Note to self - when saving the monst_t and obj_t structs to database,
// the 'data' in monst_t will have to be munged to a Short (index for 'mons').


//#ifndef NOWORM
typedef struct wseg { //was "def.wseg.h"
  struct wseg *nseg;
  Int8 wx,wy;
  Boolean wdispl;
} wseg_t;
#define MAX_WORM 32
//#endif NOWORM


typedef struct engr { // was in engrave.c along with DUST ENGRAVE and BURN.
  struct engr *nxt_engr;
  Char *engr_txt; // hmmmmm...
  //  Char engr_txt[MAX_ENGR_LEN];
  Long engr_time;	/* moment engraving was (will be) finished */
  Int8 engr_x, engr_y;
  //  UInt engr_lth;	/* for save & restore; not length of text */
  UChar engr_type;
} engr_t;
#define	DUST	1
#define	ENGRAVE	2
#define	BURN	3


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

#define LUCKMAX         10      /* on moonlit nights 11 */
#define LUCKMIN         (-10)
#define TT_BEARTRAP     0
#define TT_PIT          1
typedef struct prop {
  Long p_flags;
  // also, there was a function ptr, do these things work with multiple sec???
  // anyway, watch out for p_tofn!!!
} prop_t;
typedef struct {
  Int8 ux, uy;
  Int8 dx, dy, dz; // direction of move (or zap or ..)
  Int8 udisx, udisy;     /* last display pos */
  Char usym;              /* usually '@' */
  Int8 uluck;
  Int8 last_str_turn; // 0, 1, 2; +, -
  Boolean udispl;
  Int8 ulevel; // 1-14
  Int8 utrap; // trap timeout
  Boolean utraptype; // has value TT_BEARTRAP or TT_PIT
  Int8 uinshop; // uinshop-1 == roomno of shop you are in
  // ...
  UChar usick_cause_otype; // used in only one place.  sigh.
  prop_t uprops[LAST_RING+10]; // this worries me a bit.  see prop_t above.

  Boolean umconf; // are you able to confuse a monster
  Boolean uswallow; // are you currently in a monster
  Int8 uswallowedtime; // how long you have been
  Int8 uhs; // hunger state
  Int8 ustr;
  Int8 ustrmax;
  Int8 udaminc;
  Int8 uac;
  Short uhp;
  Short uhpmax;
  Long ugold;
  Long ugold0; // to remember your starting allowance.
  Long uexp;
  Long urexp;
  Short uhunger;  /* referenced only in eat.c and shk.c */
  Short uinvault;
  monst_t *ustuck; // ptr to monster you are in, if uswallow is true
  Int8 nr_killed[CMNUM+2];  /* used for experience bookkeeping */
  // ...
  // CMNUM+2 = 57
  Boolean dead;
  Int8 character_class;
  UChar reserved; // padding.  since (CMNUM+2)%4 = 57%4 = 1.
  ULong birthdate; // for tracking topten records.  'cause I expect SCUMMING.
} you_t;
extern you_t you; // lives in form_chargen.c right now for no good reason.

typedef struct prev_state_s {
  UChar mode;
  UChar dir_cmd; // e.g. DIR_LOOK
  Char cmd; // e.g. '?' to look
  //  UChar dir; // 1-8 or whatever // Now in you.dx, you.dy, you.dz....
  Short count; // "command_count"
  //  Short item; // index in inventory
  obj_t *item;
  monst_t *mon; // (nearly always NULL)
  Short spell; // for repeating 'm' and 'p'
} previous_state;


typedef struct HackPreferenceType_s
{
  /* ... */
  Short run_walk_border;
  Short walk_center_border;
  Short hardware[8]; // what bindings in dungeon screen
  Boolean use_hardware; // (buttons)
  Char name[PL_NSIZ];
  Boolean is_male;
  Boolean big_font;
  Boolean relative_move;
  Boolean sound;
  Boolean run;
  Boolean overlay_circles;
  //  Boolean auto_pickup;
  Boolean black_bg;
  Boolean color_on;
  Boolean no_animations;
} HackPreferenceType;
// Hack options:
// female = duh.  This and name should go in FIRST startup screen,
//                read them from prefs and write any changes to prefs.
//                (they should not be accessible elsewhere.)
// time = show "%ld moves" in the status bar
// nonews = whether to show contents of 'news' file        (kinda obsolete)
// standout = whether "--more--" and your score in top10  should be inverted.
// nonull = something to do with artificial delay
// no_rest_on_space = basically what you think.
// invlet_constant,= whether inv letters should stay constant when dropped
// notombstone, end_own, end_top, end_around = customizing post-death display.

#define NO_OP 0
#define GO_ON 1
#define DONE 2
typedef UInt tri_val_t;
