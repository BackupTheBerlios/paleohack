/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * This file is copyright (C) 2001 Bridget Spitznagel                *
 *********************************************************************/
#include "paleohack.h" // needed for my_prefs.sound
//#include "palm.h"
#include <DLServer.h> /* needed for user name */
#ifdef I_AM_COLOR
#include <SystemMgr.h> /* needed for user name */
#else
#include <SystemMgr.rh> /* needed for user name */
#endif /* I_AM_COLOR */

/**********************************************************************
  Tell the Palm to act like the user tapped the given button...
  Return true if we did (a very few callers would like to know)
**********************************************************************/
Boolean hit_button_if_usable(FormPtr frm, Word btn_index)
{
  ControlPtr btn;
  btn = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, btn_index));
  if (btn->attr.usable)
    CtlHitControl(btn);
  return (btn->attr.usable);
}

/**********************************************************************
                       UPDATE_FIELD_SCROLLERS
 IN:
 frm, fld, up_scroller, down_scroller = various UI doodads
 OUT:
 nothing
 PURPOSE:
 Update the given scroller widgets (for the given field 
 (in the given form)), according to whether the field is scrollable
 in the "up" and "down" directions.
 **********************************************************************/
void update_field_scrollers(FormPtr frm, FieldPtr fld,
			    Word up_scroller, Word down_scroller) 
{
  Boolean u, d;
  u = FldScrollable(fld, winUp);
  d = FldScrollable(fld, winDown);
  FrmUpdateScrollers(frm, 
		     FrmGetObjectIndex(frm, up_scroller),
		     FrmGetObjectIndex(frm, down_scroller),
		     u, d);
  return;
}


/**********************************************************************
                       PAGE_SCROLL_FIELD
 IN:
 frm, fld = various UI doodads
 dir = whether to scroll 'fld' up or down
 OUT:
 PURPOSE:
 Call this to scroll the field 'fld' up/down by one "page".
 (The caller should call update_field_scrollers immediately afterward.)
 **********************************************************************/
#ifndef I_AM_COLOR
void page_scroll_field(FormPtr frm, FieldPtr fld, DirectionType dir)
#else /* I_AM_COLOR */
void page_scroll_field(FormPtr frm, FieldPtr fld, WinDirectionType dir)
#endif /* I_AM_COLOR */
{
  Word linesToScroll;

  /* how many lines can we scroll? */
  if (FldScrollable(fld, dir)) {
    linesToScroll = FldGetVisibleLines(fld) - 1;
    FldScrollField(fld, linesToScroll, dir);
  }

  return;
}


/* how to exit a popup form that you can enter from more than one place */
void LeaveForm()
{
   FormPtr frm;
   frm = FrmGetActiveForm();
   FrmEraseForm (frm);
   FrmDeleteForm (frm);
   FrmSetActiveForm (FrmGetFirstForm ());
}


/***************************************************************
                   md_malloc
 IN:
 n = size to allocate
 OUT:
 pointer to the locked chunk
 PURPOSE:
 Allocate and lock a moveable chunk of memory.
****************************************************************/
Char * md_malloc(Int n)
{
  VoidHand h;
  VoidPtr p;

  h = MemHandleNew((ULong) n); /* will this cast work??  apparently. */
  if (!h) {
    /* the caller might want to check this and die. */
    return NULL;
  }

  p = MemHandleLock(h);
  MemSet(p, n, 0); /* just to make really sure the memory is zeroed */
  return p;
}

void free_me(VoidPtr ptr)
{
  VoidHand h;
  h = MemPtrRecoverHandle((ptr)); // hm, I could do MemPtrFree instead maybe..
  if (h) MemHandleFree(h);
}



/***************************************************************
                   RND
 IN:
 N = upper bound (inclusive)
 OUT:
 a random number between 1 and N inclusive: [1,N]
 PURPOSE:
 duh
****************************************************************/
Int rnd(Int y)
{
  Int r;
  if (y <= 1) return 1;
  r = SysRandom(0);
  r = (r % y) + 1; // 0 to y-1... add 1... 1 to y.
  return r;
}
/***************************************************************
                   RUND
 IN:
 N = upper bound (exclusive)
 OUT:
 a random number between 0 and N-1 inclusive: [0,N)
 PURPOSE:
 duh
****************************************************************/
Int rund(Int y)
{
  Int r;
  if (y <= 1) return 0;
  r = SysRandom(0);
  r = r % y; // 0 to y-1...
  return r;
}

Short dice(Short n, Short x)
{
  Short i, total = 0;
  for (i = 0 ; i < n ; i++)
    total += (SysRandom(0) % x) + 1;
  return total;
}



/**********************************************************************
                       DO_FEEP
 IN:
 frq = frequency
 dur = duration
 OUT:
 nothing
 PURPOSE:
 This will produce a beep of the requested frequency and duration,
 if the pilot owner has "game sound" turned on in the Preferences app.
 (Try to keep it down to chirps...)
 **********************************************************************/
/* SoundLevelType: slOn, slOff
   gameSoundLevel found in SystemPreferencesType
 */
void do_feep(Long frq, UInt dur)
{
  SystemPreferencesChoice allgamesound;

  if (!my_prefs.sound) return;

#ifdef I_AM_OS_2
  allgamesound = prefGameSoundLevel;
#else
  allgamesound = prefGameSoundLevelV20;
#endif

  if (PrefGetPreference(allgamesound) != slOff) {
    /* click: 200, 9
       confirmation: 500, 70  */
    SndCommandType sndCmd;
    sndCmd.cmd = sndCmdFreqDurationAmp; /* "play a sound" */
    sndCmd.param1 = frq; /* frequency in Hz */
    sndCmd.param2 = dur; /* duration in milliseconds */
    sndCmd.param3 = sndDefaultAmp; /* amplitude (0 to sndMaxAmp) */
    SndDoCmd(0, &sndCmd, true);
  }
}



/***************************************************************
                   get_default_username
 IN:
 (buf     has at least max_len characters)
 max_len  how many characters of name you want
 OUT:
 writes a string to buf
 PURPOSE:
 prime the pump with hotsync name, truncated at whitespace
****************************************************************/

void get_default_username(Char *buf, Short max_len)
{
  Char *tmp, *first_wspace;
  VoidHand h;
  tmp = md_malloc(sizeof(Char) * (dlkMaxUserNameLength + 1));
  DlkGetSyncInfo(NULL, NULL, NULL, tmp, NULL, NULL);
  /* if it's too long, use the first name only */
  if (StrLen(tmp) > max_len-1) {
    first_wspace = StrChr(tmp, spaceChr);
    if (first_wspace)
      *(first_wspace) = '\0';
    else
      tmp[max_len-1] = '\0';
  }
  if (StrLen(tmp))
    StrNCopy(buf, tmp, max_len);
  else {
    Short t = rund(3);
    switch(t) {
    case 0:  StrPrintF(buf, "Noman"); break;
    case 1:  StrPrintF(buf, "Outis"); break; // "no man" in greek, says the web
    default: StrPrintF(buf, "Metis"); break; // "no one"/"cunning" pun in greek
    }
  }
  h = MemPtrRecoverHandle(tmp);
  if (h) MemHandleFree(h);  
}

/***************************************************************
                   strrchr  or  rindex
 IN:
 null-terminated string s, char c
 OUT:
 a pointer to the last occurrence of the character c in the string s
****************************************************************/
Char *my_rindex(Char *s, Char c)
{
  Char *lastc = NULL;
  Short i;
  for (i = 0 ; s[i] ; i++)
    if (s[i] == c)
      lastc = &(s[i]);
  return lastc;
}


/***************************************************************
                    wait_for_event
 PURPOSE:
 (suck up queued events)
 wait for the user to do something... anything!
 Useful if you want them to see the last message before dying.
****************************************************************/
void wait_for_event()
{
  EventType queued_event;
  do {
    EvtGetEvent(&queued_event, 0); // soak up any queued winclose events
  } while (queued_event.eType != nilEvent);
  EvtGetEvent(&queued_event, evtWaitForever);
  do {
    EvtGetEvent(&queued_event, 0); // soak up any accompanying queued events
  } while (queued_event.eType != nilEvent);
  // now you can LeaveForm() and FrmGotoForm(DeathForm) or whatever.
}




// Draw text centered around x_mid, at y.
Short WinDrawChars_ctr(Char *buf, Short len, Short x_mid, Short y)
{
  Short x, width;
  width = FntCharsWidth(buf, len);
  x = x_mid - width / 2;
  if (x < 0) x = 0;
  WinDrawChars(buf, len, x, y);
  return x;
}
