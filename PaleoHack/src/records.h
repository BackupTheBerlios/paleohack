// if you mess with the order you must change lock.c.
#define REC_OBJCL          0x40
#define REC_OBJCL_NAME     0x41
#define REC_OBJCL_DESC     0x42
#define REC_PERMON         0x43
#define REC_PERMON_NAME    0x44
#define REC_TROBJ          0x45
#define REC_SHKNAM         0x46
#define REC_RUMORS         0x47
#define FIRST_CONST_REC    REC_OBJCL
#define NUM_CONST_RECS 8


#define REC_UNAME 0
#define REC_DESCS 1
// Allow 1 record for the character
// (it won't be deleted while the game is in progress, BUT it won't be
// updated either.. well, it might be checkpointed in case of crash..)
#define NUM_VOLATILE_RECS 2
#define REC_SAVECHAR NUM_VOLATILE_RECS
#define REC_LEVEL_START (REC_SAVECHAR+1)
