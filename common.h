/************************************************************************
* File:  common.h
*
* Purpose:
*   This include file is for common includes/defines.
*   Assumes utils.c is compiled/built.
*
* Notes:
*   Code should always exit using Unix convention:  exit(EXIT_SUCCESS) or exit(EXIT_FAILURE)
*
************************************************************************/
#ifndef	__common_h
#define	__common_h

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> 
#include <stdint.h>  /*brings in C99 types of uint32_t uint64_t  ...*/
#include <limits.h>  /*brings in limits such as LONG_MIN LLONG_MAX ... */
#include <math.h>    /* floor, ... */

#include <string.h>     
#include <errno.h>


#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <netinet/in.h> /* for in_addr */
#include <arpa/inet.h> /* for inet_addr ... */

#include <unistd.h>     /* for close() */
#include <fcntl.h>
#include <netdb.h>

#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/wait.h>




#if defined(__linux__) 

uint64_t htonll (uint64_t InAddr) ;
#elif defined(__APPLE__)
//htonll provided
#endif

int is_bigendian();
//void swapbytes(void *_object, size_t size);

#ifndef LINUX
#define INADDR_NONE 0xffffffff
#endif


//Definition, FALSE is 0,  TRUE is anything other
#define TRUE 1
#define FALSE 0

#define VALID 1
#define NOTVALID 0

//Defines max size temp buffer that any object might create
#define MAX_TMP_BUFFER 1024
#define MAXLINE 4096

/* default permissions for new files */
#define	FILE_MODE	(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
/* default permissions for new directories */
#define	DIR_MODE	(FILE_MODE | S_IXUSR | S_IXGRP | S_IXOTH)

/*
  Consistent with C++ and Unix conventions for success and failure of routines.

  If the method returns a valid rc, EXIT_FAILURE is <0
   Should use bool when the method has a context appropriate for returning T/F.   

  NOTE:  Always exit a routine with the unix standard: 
          exit(EXIT_SUCCESS) or exit(EXIT_FAILURE)

  NOTE   A NULL can be interpretted as an error
*/

#define SUCCESS 0
#define NOERROR 0

#define ERROR   -1
#define FAILURE -1
#define FAILED -1



#endif


