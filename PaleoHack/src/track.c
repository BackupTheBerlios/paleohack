/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
#include "paleohackRsc.h"

#define	UTSZ	50 /* "you track size" */

coord utrack[UTSZ];
Short utcnt = 0;
Short utpnt = 0;

void init_track() // was initrack
{
  utcnt = utpnt = 0;
}

/* add to track */
void set_track() // was settrack
{
  if (utcnt < UTSZ) utcnt++;
  if (utpnt == UTSZ) utpnt = 0;
  utrack[utpnt].x = you.ux;
  utrack[utpnt].y = you.uy;
  utpnt++;
}

coord * get_track(Short x, Short y) // was gettrack
{
  Short i, cnt, dist;
  coord tc;
  cnt = utcnt;
  for (i = utpnt - 1 ; cnt-- ; i--) {
    if (i == -1) i = UTSZ - 1;
    tc = utrack[i];
    dist = (x-tc.x)*(x-tc.x) + (y-tc.y)*(y-tc.y);
    if (dist < 3)
      return (dist ? &(utrack[i]) : NULL);
  }
  return NULL;
}
