// Palm changed the name of header files for SDK 3.5 ....
#ifndef I_AM_COLOR

#include <Pilot.h>
// up,down are in Core/UI/Window.h
#define winUp up
#define winDown down

#define Int8 SByte

#else /* I_AM_COLOR */

#include <PalmOS.h>
#include <PalmCompatibility.h>
// winUp,winDown are in Core/System/Window.h and replace up,down

#endif /* I_AM_COLOR */
