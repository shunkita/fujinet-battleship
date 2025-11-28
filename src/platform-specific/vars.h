/*
  Platform specific vars.
*/
#ifndef VARS_H
#define VARS_H

// Default vars (covers most playforms)
#define ESCAPE "ESCAPE"
#define ESC "ESC"

// Include platform specific vars - this is defined in the Makefile as "../$(PLATFORM)/vars.h"
#include PLATFORM_VARS

#endif /* VARS_H */