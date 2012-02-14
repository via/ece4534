#ifndef __vtUtilitiesh
#define __vtUtilitiesh
#include "lpc_types.h"
#include "lpc17xx.h"
#include "core_cm3.h"

/* ************************************************************
   Organized way to handle the ITM debugging facility
   ************************************************************ */
// There are 31 ports (port 0 should be used for printf() [see below] we can use
// Instead of randomly using numbers (and getting mixed up regarding what code is using which port)
//   we define the uses here (and you should define/re-define them as you like.
// They are defined as macros to keep them lightweight.
// Note: You can selectively enable/disable each port in the debug settings in the Keil tools, so you
//       can leave your debug log statements in place w/o worrying about generating too much output.  Also,
//       the following line (if set to 0) will let you eliminate these from the compiled code.
#define vtITMEnabled 2
// Here is where you should define each port you are using to avoid conflicts: use values 1 through 31
#define vtITMPortI2C0IntHandler 1
#define vtITMPortLCD 2
#define vtITMPortIdle 3
#define vtITMPortI2CMsg 4
#define vtITMPortTempVals 5
#define vtITMPortI2C1IntHandler 6
#define vtITMPortLCDMsg 7 
// #define vtITMPort??? 31
// End of list of port definitions

#if vtITMEnabled == 1
// Basic ITM writing -- will not block but trace data can be lost (trace window will indicate if it is lost)
#define vtITMu8(port,val) ITM->PORT[port].u8 = (val)
#define vtITMu16(port,val) ITM->PORT[port].u16 = (val)
#define vtITMu32(port,val) ITM->PORT[port].u32 = (val)
#elif vtITMEnabled == 2
// Blocking writes to ITM -- ensures that trace data is never lost, but can slow down the program
static __INLINE void vtITMu8(uint8_t port,uint8_t ch)
{ 
  if ((CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)  &&     	   
      (ITM->TCR & ITM_TCR_ITMENA_Msk)                  &&     	  
      (ITM->TER & (1UL << port)        )                    )     
  {						
    while (ITM->PORT[port].u32 == 0);
    ITM->PORT[port].u8 = (uint8_t) ch;
  }			
}
static __INLINE void vtITMu16(uint8_t port,uint16_t ch)
{ 
  if ((CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)  &&     	   
      (ITM->TCR & ITM_TCR_ITMENA_Msk)                  &&     	  
      (ITM->TER & (1UL << port)        )                    )     
  {						
    while (ITM->PORT[port].u32 == 0);
    ITM->PORT[port].u16 = (uint16_t) ch;
  }			
}
static __INLINE void vtITMu32(uint8_t port,uint32_t ch)
{ 
  if ((CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)  &&     	   
      (ITM->TCR & ITM_TCR_ITMENA_Msk)                  &&     	  
      (ITM->TER & (1UL << port)        )                    )     
  {						
    while (ITM->PORT[port].u32 == 0);
    ITM->PORT[port].u32 = (uint32_t) ch;
  }			
}
#elif vtITMEnabled == 0
// Turn off all writes to ITM
#define vtITMu8(port,val) 
#define vtITMu16(port,val) 
#define vtITMu32(port,val) 
#else
Something is wrong
#endif

/* ************************************************************
   End of ITM macros
   ************************************************************ */

/* ************************************************************
   Definition of malloc()
   ************************************************************ */

// Decide which version of malloc() is going to be used
// NOTE: You should *really* think twice about directly using malloc()
#define MALLOC_VERSION 1

#if MALLOC_VERSION==1
// use the version built into the C libraries and supported by the heap allocation in syscalls.c
//   in this version, the size of the heap is defined and explained in syscalls.c and startup_LPC17xx.s
// FreeRTOS will be using heap_3.c in this case
#elif MALLOC_VERSION==2
// do not provide malloc()/free() to regular programs; tell FreeRTOS to use its own malloc()/free()
//   the size of the heap used by FreeRTOS is defined in freertosconfig.h
// If you are using this option *and* your code (or a routine it calls) makes a call to malloc(), then my
//   code in syscalls.c will catch this and bring execution to a halt to let you know what happened.
#else
Something is not right
#endif

/* ************************************************************
   End of definition of malloc()/free()
   ************************************************************ */


/* ************************************************************
   Definition of printf()
   ************************************************************ */
// Decide which (if any) version of printf() to use
// 0 indicates printf() does nothing	(consumes least space)
// 1: use the real, full printf() (consumes the most space)
// 2: use a limited version of printf()	(see VTutilities.c for the formatting that is supported) -- consumes not very much space

// Note that sprintf() is also space consuming -- option 2 and option 0 use the same, limited small version of sprintf()
#define PRINTF_VERSION 1

#if PRINTF_VERSION == 1
// Full printf()
// Requires that you have full malloc() installed [MALLOC_VERSION==1]
#if MALLOC_VERSION != 1
You must have MALLOC_VERSION==1 to use the full printf()
#endif
#elif PRINTF_VERSION == 0
// printf() does nothing
#elif PRINTF_VERSION == 2
// limited printf()
#else
Something is not right...
#endif

// Now indicate where the printf() output (if any) should go
// 0: Go nowhere
// 1: Go to USB (view via Hyperterminal or other similar tool)
// 2: Go to Port0 which can only be viewed in the Keil tools-- "View->Serial Windows->Debug (printf) Viewer"
//    You must enable "Trace" and "Port0" in the Debug setup options to view this output.
#define PRINTF_DESTINATION 2

#if PRINTF_DESTINATION==2
#define VTputchar(c) ITM_SendChar(c)
#elif PRINTF_DESTINATION==1
int VTputchar(int);
#elif PRINTF_DESTINATION==0
#define VTputchar(c) c
#else
Something is not right...
#endif
/* ************************************************************
   Enf of Definition of printf()
   ************************************************************ */

#define VT_HANDLE_FATAL_ERROR(code) vtHandleFatalError(code,__LINE__,__FILE__)
void vtHandleFatalError(int,int,char []);

void vtInitLED();
void vtLEDOff(uint8_t mask);
void vtLEDOn(uint8_t mask);
#endif