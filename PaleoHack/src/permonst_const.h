#define PM_ACID_BLOB    &mons[7]
#define PM_ZOMBIE       &mons[13]
#define PM_PIERCER      &mons[17]
#define PM_KILLER_BEE   &mons[26]
#define PM_WRAITH       &mons[33]
#define PM_MIMIC        &mons[37]
#define PM_VAMPIRE      &mons[43]
#define PM_CHAMELEON    &mons[47]
#define PM_DEMON        &mons[54]
#define PM_MINOTAUR     &mons[55]       /* last in mons array */
#define PM_SHK          &mons[56]       /* very last */
#define PM_GHOST        &mons[57] /*  &pm_ghost */
#define PM_WIZARD       &mons[58] /*  &pm_wizard */
#define PM_EEL          &mons[60] /*  &pm_eel */

#define CMNUM           55              /* number of common monsters */
/* Well, CMNUM is unchanged, but excludes several monsters that used to
   be locally defined but which I have chucked into the database record
   right after shopkeeper:
   ghost, wizard, mail, eel, hellhound, guard, little dog, dog, large dog.
   So, PM_SHK is not really the "very last" and CMNUM is not length(mons). */

// For convenience,

#define PM_MAILD        &mons[59]
#define PM_HELLHOUND    &mons[61]
#define PM_GUARD        &mons[62]
#define PM_SM_DOG       &mons[63]
#define PM_MD_DOG       &mons[64]
#define PM_LG_DOG       &mons[65]


// this was in def.monst.h.  they get used with StrChr.
#define MREGEN          "TVi1"
#define UNDEAD          "ZVW "
