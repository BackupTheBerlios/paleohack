#define MAX_OBJCLASS       217
#define MAX_OBJCLASS_NAME  203
#define MAX_OBJCLASS_DESCR 109
#define MAX_PERMONST      66
#define MAX_PERMONST_NAME 66

extern objclass_t *objects;
extern Char *oc_names;
extern Char *oc_descrs;
extern Short *oc_descr_offset;
extern permonst_t *mons;
extern Char *mon_names;

//#include "permonst_const.h" // bunch of pointers into the mons array...
// (that include has been moved earlier)
// and.. let's have some poking into the objects array...
#define ARM_BONUS(obj)  ((10 - objects[obj->otype].a_ac) + obj->spe)


// homeless globals:
extern struct flag flags;
// replacement for object class nameknown:
#define MAX_OC_NK 28
extern UChar oc_name_known[MAX_OC_NK];

// this is housed in display.c for convenience of StrPrintf->message()--users
#define BIGBUF 160
extern Char ScratchBuffer[BIGBUF];


// dynamic freaks that are here for no good reason:
extern obj_t   * fobj;
extern obj_t   * fcobj;
extern obj_t   * invent;
extern obj_t   * uwep, * uarm, * uarm2, * uarmh, * uarms, * uarmg;
extern obj_t   * uright, * uleft, * uchain, * uball;
extern monst_t * fmon;
extern monst_t * fallen_down;/* monsters that fell through a trapdoor.
				they will appear on the next level @ goes to,
				even if he goes up! */
extern monst_t * mydogs;
extern trap_t  * ftrap;
extern gold_t  * fgold;


// random globals.. oh goodie..
extern Long moves; // lives in make_obj.c for no good reason

extern Char *killer; // lives in end.c

extern Int8 dlevel; // wonder why this is not a part of you
extern Int8 maxdlevel; // ditto
extern Short xdnstair, xupstair, ydnstair, yupstair;


extern Char *nomovemsg;

extern HackPreferenceType my_prefs;
