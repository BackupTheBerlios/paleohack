/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"

void qWinEraseRectangle(RectanglePtr r, Word cornerDiam)
{
  if (my_prefs.black_bg)
    WinDrawRectangle(r, cornerDiam);
  else
    WinEraseRectangle(r, cornerDiam);
}
void qWinDrawLine(short x1, short y1, short x2, short y2)
{
  if (my_prefs.black_bg)
    WinEraseLine(x1, y1, x2, y2);
  else
    WinDrawLine(x1, y1, x2, y2);
}
void qWinEraseLine(short x1, short y1, short x2, short y2)
{
  if (my_prefs.black_bg)
    WinDrawLine(x1, y1, x2, y2);
  else
    WinEraseLine(x1, y1, x2, y2);
}
void qWinDrawChars(CharPtr chars, Word len, SWord x, SWord y)
{
  if (my_prefs.black_bg)
    WinDrawInvertedChars(chars, len, x, y);
  else
    WinDrawChars(chars, len, x, y);
}
void qWinDrawInvertedChars(CharPtr chars, Word len, SWord x, SWord y)
{
  if (my_prefs.black_bg)
    WinDrawChars(chars, len, x, y);
  else
    WinDrawInvertedChars(chars, len, x, y);
}

