/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 *********************************************************************/
#ifndef __PALEOHACK_H__
#define __PALEOHACK_H__

#include "palm.h"
#ifndef I_AM_OS_2
#include "Handera/Vga.h" /* some need this too */
#endif

// I have not registered this crid yet.  See Makefile, paleohack.def
#define phAppType 'PlHk'
#define phDBType  'Data'
#define phSaveDBType  'Data'
#define phBonesDBType  'Data'
#define phAppID   'PlHk'
extern DmOpenRef       phDB;
extern DmOpenRef       phSaveDB;
extern DmOpenRef       phBonesDB;
#define phDBName "PaleoHackDB"
#define phSaveDBName "PaleoHackSaveDB"
#define phBonesDBName "PaleoHackBonesDB"
#define phColorDBVersion 0x01
#define phSaveDBVersion 0x01
#define phBonesDBVersion 0x01

#define phAppPrefID 0x00
#define phAppPrefVersion 0x01


// also do some includes, perhaps.
#include "sections.h"
#include "constant.h"
#include "permonst_const.h"
#include "types.h"
#include "prototypes.h"
#include "records.h"
#include "lock-externs.h"


#endif
