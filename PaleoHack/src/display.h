extern Boolean IsVGA; // like IsColor.  tells you if you're on a Handera 330.
#ifdef I_AM_OS_2
extern FontPtr oldFontSix;
#else
extern FontID SmallFont, MyFont;
extern FontID BigFont;
//extern FontType *fontPtr; // was in StartApplication
#endif
