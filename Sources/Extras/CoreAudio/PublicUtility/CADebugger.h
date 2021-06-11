/*
     File: CADebugger.h
 Abstract: Part of CoreAudio Utility Classes
  Version: 1.1

*/
#if !defined(__CADebugger_h__)
#define __CADebugger_h__

//=============================================================================
//	Includes
//=============================================================================

#if !defined(__COREAUDIO_USE_FLAT_INCLUDES__)
	#include <CoreAudio/CoreAudioTypes.h>
#else
	#include <CoreAudioTypes.h>
#endif

//=============================================================================
//	CADebugger
//=============================================================================

#if	TARGET_API_MAC_OSX
	extern bool CAIsDebuggerAttached(void);
#endif
extern void	CADebuggerStop(void);

#endif
