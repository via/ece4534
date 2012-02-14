/****************************************************************************
*  Copyright (c) 2009 by Michael Fischer. All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without 
*  modification, are permitted provided that the following conditions 
*  are met:
*  
*  1. Redistributions of source code must retain the above copyright 
*     notice, this list of conditions and the following disclaimer.
*  2. Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in the 
*     documentation and/or other materials provided with the distribution.
*  3. Neither the name of the author nor the names of its contributors may 
*     be used to endorse or promote products derived from this software 
*     without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL 
*  THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS 
*  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
*  AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
*  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF 
*  THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
*  SUCH DAMAGE.
*
****************************************************************************
*  History:
*
*  28.03.09  mifi   First Version, based on the original syscall.c from
*                   newlib version 1.17.0
****************************************************************************/

// typical library includes
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

// FreeRTOS includes
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

// Other includes
#include "extUSB.h"
#include "vtUtilities.h"

// Mutex used to protect malloc()
static xSemaphoreHandle xMallocMutex = NULL;
#if PRINTF_VERSION == 1
// Mutex used to protect write to USB output
static xSemaphoreHandle xWriteMutex = NULL;
// Mutex used to protect read from USB input
static xSemaphoreHandle xReadMutex = NULL;
#endif

// Must be called before anything else
void init_syscalls()
{
	xMallocMutex = xSemaphoreCreateRecursiveMutex();
#if PRINTF_VERSION == 1 
	xWriteMutex = xSemaphoreCreateRecursiveMutex();
	xReadMutex = xSemaphoreCreateRecursiveMutex();
#endif
}

// Called by malloc to make its own operation thread-safe
void __malloc_lock(struct _reent *r)
{
	if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
		if (xSemaphoreTakeRecursive(xMallocMutex,portMAX_DELAY) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}
	}
}
// Called by malloc to make things thread-safe
void __malloc_unlock(struct _reent *r)
{
	if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
		if (xSemaphoreGiveRecursive(xMallocMutex) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}
	}
}

/***************************************************************************/

int _read_r (struct _reent *r, int file, char * ptr, int len)
{

  switch(file) {
  	case 0: { // stdin (only currently defined if using USB and full printf() defined)
		#if ((PRINTF_DESTINATION == 1) && (PRINTF_VERSION == 1))
		int count = 0;
        int i = len;
		if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
			if (xSemaphoreTakeRecursive(xReadMutex,portMAX_DELAY)) {
			} else {
				__errno_r(r)  = EIO;
				return(-1);
			}
		} else {
			// return because there no way to write to USB yet
			__errno_r(r)  = EIO;
			return(-1);
		}

		for (i=0;i<len;i++) {
			ptr[count] = readUSBInputBuffer();
			count++;
			if ((ptr[count-1] == '\n') || (ptr[count-1] == '\r')) break;
		}
		if (xSemaphoreGiveRecursive(xReadMutex)) {
		}
		return(count);
		break;
		#else
		__errno_r(r) = EBADF;
		return -1;
		#endif
	}
	default: {
		__errno_r(r) = EBADF;
		return -1;
	}
  }
}

/***************************************************************************/
/* Not actually implemented */
int _lseek_r (struct _reent *r, int file, int ptr, int dir)
{
  r = r;
  file = file;
  ptr = ptr;
  dir = dir;
  
  return 0;
}

/***************************************************************************/
int _write_r (struct _reent *r, int file, char * ptr, int len)
{ 

  // Only do for file descriptors 1 & 2  (stdout & stderr)
  if ((file >= 1) && (file <= 2)) {
    #if PRINTF_VERSION == 1
  	int index;
	int retval;
 	if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
		if (xSemaphoreTakeRecursive(xWriteMutex,portMAX_DELAY)) {
		} else {
			__errno_r(r)  = EIO;
			return(0);
		}
	} else {
		// return because there no way to write to USB yet
		__errno_r(r)  = EIO;
		return(0);
	}
 	 for(index=0; index<len; index++) {  
		retval = VTputchar(ptr[index]);
		if (retval == EOF) {
			__errno_r(r)  = EIO;
			if (xSemaphoreGiveRecursive(xWriteMutex)) {
			}
			return(index);
		}
 	 }
	 if (xSemaphoreGiveRecursive(xWriteMutex)) {
	 } 
	 return len;
	 #else
	 VT_HANDLE_FATAL_ERROR(0);
	 return(0);
  	 #endif
  }
 
  return 0;
}

/***************************************************************************/
/* Not actually implemented */
int _close_r (struct _reent *r, int file)
{
  return 0;
}

/***************************************************************************/
/* Configured to first use the heap allocated in the 32K RAM block and then */
/*   to use the heap allocated in the RAM2 (RAM1 is used by the ETH controller */
caddr_t _sbrk_r (struct _reent *r, int incr)
{
#if MALLOC_VERSION != 1
  VT_HANDLE_FATAL_ERROR(0);
  return(0);
#else
  extern char   *h_end __asm ("__cs3_heap_end"); /* Defined by the linker.  */
  extern char   *h_start __asm ("__cs3_heap_start"); /* Defined by the linker.  */
  extern char   *h_end2 __asm ("__cs3_heap_end2"); /* Defined by the linker.  */
  extern char   *h_start2 __asm ("__cs3_heap_start2"); /* Defined by the linker.  */
  static char * heap_end;
  static char	at_ram2 = 0;
  char *        prev_heap_end;

  taskDISABLE_INTERRUPTS();	 // may not be necessary given malloc() lock

  if (heap_end == NULL)
    heap_end = (char *)&h_start;
  
  prev_heap_end = heap_end;
  
  if (heap_end + incr > (char *)&h_end)
  {
  	  /* Now we can try the second block of RAM since we have run out of the first */
	  if (!at_ram2) {
	  	heap_end = (char *) &h_start2;
		prev_heap_end = heap_end;
		at_ram2 = 1;
	  }
	  if (heap_end + incr > (char *)&h_end2) {
     	 __errno_r(r)  = ENOMEM;
		 taskENABLE_INTERRUPTS();
     	 return (caddr_t) -1;
	  }
  }
  
  heap_end += incr;
  taskENABLE_INTERRUPTS();

  return (caddr_t) prev_heap_end;
#endif
}


/***************************************************************************/
/* Not actually implemented */
int _fstat_r (struct _reent *r, int file, struct stat * st)
{
  r = r; 
  file = file;
   
  memset (st, 0, sizeof (* st));
  st->st_mode = S_IFCHR;
  return 0;
}

/***************************************************************************/
/* Not actually implemented */
int _isatty_r(struct _reent *r, int fd)
{
  r = r;
  fd = fd;
   
  return 1;
}

/*** EOF ***/


