/************************************************************************
* File:  utils.h
*
* Purpose:
*   This include file is for a set of utility functions 
*
* Notes:
*
************************************************************************/
#ifndef	__utils_h
#define	__utils_h

#include "common.h" 
#include  <stdarg.h>		/* ANSI C header file */
#include  <syslog.h>


#define	min(a,b)	((a) < (b) ? (a) : (b))
#define	max(a,b)	((a) > (b) ? (a) : (b))
void setVersion(double versionLevel);
char *getVersion();

double timestamp();
int getTime(struct timeval *callersTime);
double convertTimeval(struct timeval *t);
int nanodelay(int64_t ns);
int myDelay(double delayTime);
void sig_chld(int signo);

void die(const char *msg);
void DieWithError(char *errorMessage); 
ssize_t	 readline(int, void *, size_t);
ssize_t Readline(int fd, void *ptr, size_t maxlen);
void err_sys(const char *fmt, ...);

#endif


