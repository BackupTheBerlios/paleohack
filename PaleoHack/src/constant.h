// Size of dungeon
#define DCOLS 80
#define DROWS 22
// DCOLS and DROWS aka COLNO and ROWNO
// What we set command-counter to, when player asks to run:
#define MAX_RUN DCOLS

// This can affect your luck and how bitchy the undead are.
#define NEW_MOON        0
#define FULL_MOON       4



// Most symbols are more hardcoded..  (ha, not after I get through with them)
// Note - HWALL_SYM needs to change, to the emdash!!
#define EMDASH 151
#define HWALL_SYM     EMDASH
#define VWALL_SYM     '|'
#define SCORR_SYM     ' '
#define DOOR_SYM      '+'
#define ROOM_SYM      '.'
#define GOLD_SYM      '$'
#define DOWNSTAIR_SYM '>'
#define UPSTAIR_SYM   '<'
#define TRAP_SYM      '^'
//
#define CORR_SYM        '#'
#define POOL_SYM        '}'
#define ERRCHAR         '{'
/* definitions of all object-symbols */
#define ILLOBJ_SYM      '\\'
#define AMULET_SYM      '"'
#define FOOD_SYM        '%'
#define WEAPON_SYM      ')'
#define TOOL_SYM        '('
#define BALL_SYM        '0'
#define CHAIN_SYM       '_'
#define ROCK_SYM        '`'
#define ARMOR_SYM       '['
#define POTION_SYM      '!'
#define SCROLL_SYM      '?'
#define WAND_SYM        '/'
#define RING_SYM        '='
#define GEM_SYM         '*'


// These are values for dungeon 'cell' type:
#define HWALL 1
#define VWALL 2
#define IS_WALL(typ)    ((typ) <= VWALL)
#define SDOOR 3
#define SCORR 4
#define LDOOR 5
#define IS_ROCK(typ)    ((typ) < POOL)
#define POOL  6 // reputedly, not yet fully implemented, should be ~~like lit
#define ACCESSIBLE(typ) ((typ) >= DOOR)
#define DOOR  7
#define CORR  8
#define IS_ROOM(typ)    ((typ) >= ROOM)
#define ROOM  9
#define ZAP_POS(typ)            ((typ) > DOOR)
#define STAIRS 10



/* various kinds of traps */
#define BEAR_TRAP	0
#define	ARROW_TRAP	1
#define	DART_TRAP	2
#define TRAPDOOR	3
#define	TELEP_TRAP	4
#define PIT 		5
#define SLP_GAS_TRAP	6
#define	PIERC		7
#define	MIMIC		8	/* used only in mklev.c */
#define TRAPNUM 	9	/* if not less than 32, change sizeof(ttyp) */
				/* see also mtrapseen (bit map) */

// kinds of special rooms
/* 0: ordinary room; 8-15: various shops */
/* Note: some code assumes that >= 8 means shop, so be careful when adding
   new roomtypes */
#define SWAMP     3
#define VAULT     4
#define BEEHIVE   5
#define MORGUE    6
#define ZOO       7
#define SHOPBASE  8
#define WANDSHOP  9
#define GENERAL  15




// from eat.c
#define SATIATED        0
#define NOT_HUNGRY      1
#define HUNGRY          2
#define WEAK            3
#define FAINTING        4
#define FAINTED         5
#define STARVED         6



// from def.monst.h
/* values for mspeed. 0 is normal speed */
#define MSLOW 1 /* slow monster */
#define MFAST 2 /* speeded monster */



// from hack.h:
#define MAX_CARR_CAP    120     /* so that boulders can be heavier */
#define MAXLEVEL        40
#define FAR     (DCOLS+2)       /* position outside screen */

#define MAX_ROOMS 16 // was in make_level.c
#define MAX_DOORS 100 // ditto
#define NUM_OBJ_SYMBOLS 14

// from hack.mfndpos.h:
#define	ALLOW_TRAPS	0777
#define	ALLOW_U		01000
#define	ALLOW_M		02000
#define	ALLOW_TM	04000
#define	ALLOW_ALL	(ALLOW_U | ALLOW_M | ALLOW_TM | ALLOW_TRAPS)
#define	ALLOW_SSM	010000
#define	ALLOW_ROCK	020000
#define	NOTONL		040000
#define	NOGARLIC	0100000

// from def.monst.h:
#define     MTSZ    4   /* "monster track size' */

// some stuff from me!:
#define ACT_NONE   0
#define ACT_APPLY  1
#define ACT_EAT    2
#define ACT_QUAFF  3
#define ACT_READ   4
#define ACT_AWEAR  5
#define ACT_AOFF   6
#define ACT_RWEAR  7
#define ACT_ROFF   8
#define ACT_WIELD  9
#define ACT_PUTUP  10
#define ACT_ZAP    11
#define ACT_CALL    12
#define ACT_NAME    13
#define ACT_DROP    14
#define ACT_DROPALL 15
#define ACT_DIP     16
#define ACT_THROW   17
#define ACT_ENGRAVE 18
#define ACT_CHRISTEN 19
#define ACT_REFRIGERATE   20 /* but.. this command doesn't have a letter.. */

// the Engrave form is also used to get strings for these magic effects:
#define GET_WISH     42
#define GET_GENOCIDE 43
#define GET_VAULT    44
#define GET_LTELE    45
#define ACT_CALL_2   46


#define CLASS_TOURIST      0
#define CLASS_SPELEOLOGIST 1
#define CLASS_FIGHTER      2
#define CLASS_KNIGHT       3
#define CLASS_CAVEPERSON   4
#define CLASS_WIZARD       5
#define SENSE_NONE          0
#define SENSE_MONSTERS      1
#define SENSE_OBJECTS       2
#define SENSE_GOLD          3
#define SENSE_GOLD_CONFUSED 4
#define SENSE_FOOD          5
#define SENSE_FOOD_CONFUSED 6


// from elsewhere:
#define  BUFSZ   256     /* for strings with object names in them */
// holy cow!  do we really need 256 there??

#define  PL_NSIZ 32      /* name of player, ghost, shopkeeper */

#define MAX_ENGR_LEN 40
#define MAX_MNAME 63
#define MAX_ONAME 40
// I added an arbitrary MAX_ONAME; it used to be unlimited...
#define CALLABLE_OBJS ((LAST_CALLABLE - FIRST_CALLABLE) + 1)
#define MAX_GENOCIDE 60
#define MAX_CLASS 6

// for remembering mode of main screen
#define MODE_DEFAULT     0
//#define MODE_SHOWINVIS   1
#define MODE_MORE        2
#define MODE_NAMEMON     3
#define MODE_GETCELL     4
#define MODE_DIRECTIONAL 5
#define MODE_AGAIN       6


#define SAVED_MSGS 16
#define SHOW_MSGLOG     0
#define SHOW_DISCOVERED 1
#define SHOW_DEAD       2
#define SHOW_LHELP      3
#define SHOW_SHELP      4
// ... values for msglog_mode.

// for beaming...
#define MY_MIMETYPE "application/x-paleohack-data"

// what font ID I use
#define USERFONT 129


// hardware button bindings for main form.
// most of these need to change...
#define HWB_NOOP    0
#define HWB_N       1
#define HWB_S       2
#define HWB_E       3
#define HWB_W       4 
#define HWB_MAP     5
#define HWB_SEARCH  6
#define HWB_THROW   7
#define HWB_INV     8
#define HWB_APPLY   9
//#define HWB_ZAP
//#define HWB_REST
//#define HWB_WHAT
//#define HWB_SECTOR
#define HWB_AGAIN   10
#define HWB_FONT    11
#define HWB_SHIFT   12
#define HWB_first   1
#define HWB_last    12
#define HWB_FONT_CMD '!'
#define HWB_SHIFT_CMD '#'

// we switch(e->data.keyDown.chr) to make this more tidy
#define HW_hard1Chr    0
#define HW_hard2Chr    1
#define HW_hard3Chr    2
#define HW_hard4Chr    3
#define HW_pageUpChr   4
#define HW_pageDownChr 5
#define HW_calcChr     6
#define HW_findChr     7


#include "hack.onames.h" // hack.onames.h is untouched by me.
