/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/

/* Most of the stuff in this file is rewrite-from-scratch of things  *
 * that were in unix.c                                               *
 *********************************************************************/
#include "paleohack.h"
/*
 * The time is used for:
 *	- seed for random()
 *	- year on tombstone and yymmdd in record file
 *	- phase of the moon (various monsters react to NEW_MOON or FULL_MOON)
 *	- night and midnight (the undead are dangerous at midnight)
 */

Short getyear();
void getdate(Char *pstr); // Note - I changed the fn type
/* 0-7, with 0: new, 4: full
   moon period: 29.5306 days
   year: 365.2422 days */
Short phase_of_the_moon();
/* Is the time NOT between 0600 hours and 2159 hours. */
Boolean night();
/* Is the time between 0000 and 0059 hours. */
Boolean midnight();


Short getyear()
{
  DateTimeType datetime;
  TimSecondsToDateTime(TimGetSeconds(), &datetime);
  return datetime.year; // apparently no need to add 1904 to this one.
}

/* datestr must be of length dateStringLength or longDateStringLength */
void getdate(Char *datestr)
{
  // bah, reuse something from iRogue here.
  DateTimeType datetime;
  TimSecondsToDateTime(TimGetSeconds(), &datetime);
  DateToAscii(datetime.month, datetime.day, datetime.year,
              PrefGetPreference(prefDateFormat), datestr);
}

/* Return value: 0-7, with 0:= new, 4:= full

   This computation is taken from a page of the astronomy faq which cites
   as the original, _Winning Ways_ vol 2 by Conway, Guy, and Berlekamp.
   It seems ok although I have only tested it on 2001/07/05.
   see http://www.faqs.org/faqs/astronomy/faq/part3/section-15.html
   I should compare to the algorithm in unix.c ...
 */
// XXXX off-by-one error? (full moon msg is a day late.  Aug 24-27 + not 23.)
Short phase_of_the_moon()
{
  Short tmp;
  // month and day are 1-12 and 1-31, and year does not need 1904 added.
  DateTimeType datetime;

  TimSecondsToDateTime(TimGetSeconds(), &datetime);

  /*  "In the 20th century, calculate the remainder upon dividing the
   *  last two digits of the year by 19; if greater than 9, subtract
   *  19 from this to get a number between -9 and 9." */
  tmp = datetime.year % 100; // Apparently it comes with 1904 pre-added.
  tmp = tmp % 19;
  if (tmp > 9) tmp -= 19;
  /*  "Multiply the result by 11 and reduce modulo 30 to obtain a
   *  number between -29 and +29." */
  // oh - we are taking [-9,9] to [-99,99] and thence to [-29,29].
  // except my mod function will take it to [0,29] hmmm.
  tmp *= 11;
  tmp = tmp % 30; // ???
  /*  "Add the day of the month and the number of the month (except
   *  for Jan and Feb use 3 and 4 for the month number instead of
   *  1 and 2)." */
  tmp += datetime.day;
  tmp += datetime.month;
  if (datetime.month <= february) tmp += 2;
  /*  "Subtract 4.  ... In the 21st century, use -8.3 instead of -4." */
  // Gaah!  Floating point!
  tmp = (tmp * 10) - 83;
  tmp += 20; // Kludge added to make it work out right(?). Aug23,2002.
  /*  "Reduce modulo 30 to get a number between 0 and 29. This is
   *  the age of the Moon." */
  tmp = tmp % 300;
  // Ok but then I also need to get it from there to "between 0 and 7".
  tmp = (tmp * 2) / 75; // get it down to [0..7]

  /*  "Conway also gives refinements for the leap year cycle and also
   *  for the slight variations in the lengths of months; what I have
   *  given should be good to +/- a day or so." */

  return tmp;
}


/* Night: Is the time NOT between 0600 hours and 2159 hours? */
Boolean night()
{
  DateTimeType datetime;
  UChar hour; // hour is 0-23.
  TimSecondsToDateTime(TimGetSeconds(), &datetime);
  hour = datetime.hour;
  if (!(hour > 6 && hour < 22))
    return true;
  else
    return false;
}
/* Midnight: Is the time between 0000 and 0059 hours? */
Boolean midnight()
{
  DateTimeType datetime;
  UChar hour; // Hmmm I wonder if this is 0-23 or what.
  TimSecondsToDateTime(TimGetSeconds(), &datetime);
  hour = datetime.hour;
  if (hour == 0)
    return true;
  else
    return false;
  
}
