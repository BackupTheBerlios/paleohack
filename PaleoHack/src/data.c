/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
#include "paleohackRsc.h"

void specify_what(UChar c)
{
  // ATTENTION:
  // Thou shalt not mess with the space (or lack of space) at the end of the
  // strings.  It indicates whether a large "help string" exists for this item.
  Char *buf;
  Short len;
  Boolean more_info = false;
  switch(c) {
  case '@':
    buf = "human (or you)";
    break;
  case '-': case HWALL_SYM:
    buf = "a wall";
    break;
  case '|':
    buf = "a wall";
    break;
  case '+':
    buf = "a door";
    break;
  case '.':
    buf = "the floor of a room";
    break;
  case ' ':
    buf = "a dark part of a room";
    break;
  case '#':
    buf = "a corridor";
    break;
  case '}':
    buf = "water filled area";
    break;
  case '<':
    buf = "the staircase to the previous level";
    break;
  case '>':
    buf = "the staircase to the next level";
    break;
  case '^':
    buf = "a trap";
    break;
  case '$':
    buf = "a pile, pot or chest of gold";
    break;
  case '%':
    buf = "a piece of food";
    break;
  case '!':
    buf = "a potion";
    break;
  case '*':
    buf = "a gem";
    break;
  case '?':
    buf = "a scroll";
    break;
  case '=':
    buf = "a ring";
    break;
  case '/':
    buf = "a wand";
    break;
  case '[':
    buf = "a suit of armor";
    break;
  case ')':
    buf = "a weapon";
    break;
  case '(':
    buf = "a useful item (camera, key, rope etc.)";
    break;
  case '0':
    buf = "an iron ball";
    break;
  case '_':
    buf = "an iron chain";
    break;
  case '`':
    buf = "an enormous rock";
    break;
  case '"':
    buf = "an amulet";
    break;
  case ',':
    buf = "a trapper";
    break;
  case ':':
    buf = "a chameleon";
    break;
  case ';':
    buf = "a giant eel";
    break;
  case '\'':
    buf = "a lurker above";
    break;
  case '&':
    buf = "a demon";
    break;
  case 'A':
    buf = "a giant ant";
    break;
  case 'B':
    buf = "a giant bat";
    break;
  case 'C':
    buf = "a centaur ";
    break;
  case 'D':
    buf = "a dragon ";
    break;
  case 'E':
    buf = "a floating eye";
    break;
  case 'F':
    buf = "a freezing sphere";
    break;
  case 'G':
    buf = "a gnome ";
    break;
  case 'H':
    buf = "a hobgoblin ";
    break;
  case 'I':
    buf = "an invisible stalker";
    break;
  case 'J':
    buf = "a jackal";
    break;
  case 'K':
    buf = "a kobold";
    break;
  case 'L':
    buf = "a leprechaun ";
    break;
  case 'M':
    buf = "a mimic";
    break;
  case 'N':
    buf = "a nymph";
    break;
  case 'O':
    buf = "an orc";
    break;
  case 'P':
    buf = "a purple worm";
    break;
  case 'Q':
    buf = "a quasit";
    break;
  case 'R':
    buf = "a rust monster";
    break;
  case 'S':
    buf = "a snake";
    break;
  case 'T':
    buf = "a troll";
    break;
  case 'U':
    buf = "an umber hulk";
    break;
  case 'V':
    buf = "a vampire";
    break;
  case 'W':
    buf = "a wraith";
    break;
  case 'X':
    buf = "a xorn";
    break;
  case 'Y':
    buf = "a yeti";
    break;
  case 'Z':
    buf = "a zombie";
    break;
  case 'a':
    buf = "an acid blob";
    break;
  case 'b':
    buf = "a giant beetle";
    break;
  case 'c':
    buf = "a cockatrice ";
    break;
  case 'd':
    buf = "a dog";
    break;
  case 'e':
    buf = "an ettin";
    break;
  case 'f':
    buf = "a fog cloud";
    break;
  case 'g':
    buf = "a gelatinous cube";
    break;
  case 'h':
    buf = "a homunculus";
    break;
  case 'i':
    buf = "an imp ";
    break;
  case 'j':
    buf = "a jaguar";
    break;
  case 'k':
    buf = "a killer bee";
    break;
  case 'l':
    buf = "a leocrotta";
    break;
  case 'm':
    buf = "a minotaur";
    break;
  case 'n':
    buf = "a nurse";
    break;
  case 'o':
    buf = "an owlbear";
    break;
  case 'p':
    buf = "a piercer";
    break;
  case 'q':
    buf = "a quivering blob";
    break;
  case 'r':
    buf = "a giant rat";
    break;
  case 's':
    buf = "a scorpion";
    break;
  case 't':
    buf = "a tengu ";
    break;
  case 'u':
    buf = "a unicorn ";
    break;
  case 'v':
    buf = "a violet fungi";
    break;
  case 'w':
    buf = "a long worm ";
    break;
  case '~':
    buf = "the tail of a long worm";
    break;
  case 'x':
    buf = "a xan ";
    break;
  case 'y':
    buf = "a yellow light";
    break;
  case 'z':
    buf = "a zruty ";
    break;
  case '1':
    buf = "The wizard of Yendor";
    break;
  case '2':
    buf = "The mail daemon";
    break;
  default:
    buf = "I've never heard of such things.";
    break;
  }   
  len = StrLen(buf);
  more_info = (buf[len-1] == ' ');
  StrPrintF(ScratchBuffer, "%c     %s", c, buf);
  if (more_info) {
    StrPrintF(ScratchBuffer+StrLen(ScratchBuffer)-1, ";  More info?");
    message(ScratchBuffer);
    show_messages();
    if (0 == FrmCustomAlert(MonsterInfoP, buf, NULL, NULL))
      FrmHelp(MAGIC_STRING_NUMBER + c); // XXX DANGEROUS KLUDGE-MAGIC.
    // (If someone messes up the space-at-end-of-'buf' markers and
    // PaleoHack hallucinates that there is a Magic String where none
    // actually exists, all that will happen is that you will get
    // an *empty* "Tips" window.  I tested it.  Non-fatal.  But please avoid.)
  } else {
    message(ScratchBuffer);
  }

}
