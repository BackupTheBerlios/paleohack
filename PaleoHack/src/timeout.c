/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
#include "paleohackRsc.h"

#define        SIZE(x) (int)(sizeof(x) / sizeof(x[0]))

static void stoned_dialogue() SEC_4;

void timeout()
{
  struct prop *upp;
  if (Stoned) stoned_dialogue();
  for (upp = you.uprops ; upp < you.uprops+SIZE(you.uprops) ; upp++)
    if ((upp->p_flags & TIMEOUT) && !--upp->p_flags) {
      if (false) ; //if (upp->p_tofn) (*upp->p_tofn)(); // XXXX not implemented yet!!!!!!
      else switch(upp - you.uprops) {
      case STONED:
	killer = "cockatrice";
	done("died");
	return;
	break;
      case SICK:
	message("You die because of food poisoning.");

	killer = oc_names + objects[you.usick_cause_otype].oc_name_offset;
	done("died");
	return;
	break;
      case FAST:
	message("You feel yourself slowing down.");
	break;
      case CONFUSION:
	message("You feel less confused now.");
	break;
      case BLIND:
	message("You can see again.");
	setsee();
	break;
      case INVIS:
	on_scr(you.ux, you.uy);
	message("You are no longer invisible.");
	break;
      case WOUNDED_LEGS:
	heal_legs();
	break;
      }
    }
}

/* Player is being petrified - dialogue by inmet!tower */
static const Char *stoned_texts[] = {
  "You are slowing down.",		/* 5 */
  "Your limbs are stiffening.",		/* 4 */
  "Your limbs have turned to stone.",	/* 3 */
  "You have turned to stone.",		/* 2 */
  "You are a statue."			/* 1 */
};

static void stoned_dialogue()
{
  Long i = (Stoned & TIMEOUT);

  if (i > 0 && i <= SIZE(stoned_texts))
    message(stoned_texts[SIZE(stoned_texts) - i]);
  if (i == 5)
    Fast = 0;
  if (i == 3)
    nomul(-3);
}
