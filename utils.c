/*********************************************************
* Module Name:  Tools common routines 
*
* File Name:    utils.c
*
*
*********************************************************/
#include "utils.h"

#define TRACEME 0

char Version[128];
int		daemon_proc;		/* set nonzero by daemon_init() */

/***********************************************************
* Function: void setVersion(double versionLevel) {
*
* Explanation:  This must be called by the main program
*               to set the Version. 
* inputs:   The version number 
*
* outputs:
*
* notes: 
*
**************************************************/
void setVersion(double versionLevel) {

char *versionPtr = Version;
  sprintf(versionPtr,"Version %1.2f",versionLevel);
}

/***********************************************************
* Function: char *getVersion() {
*
* Explanation: Returns a ptr to the version string.
* inputs:    
*
* outputs: Returns a ptr to the global string
*
* notes: 
*
**************************************************/
char *getVersion() {
   return (Version);
}

/***********************************************************
* Function: double timestamp() 
*
* Explanation:  This returns the current tiem
*               as a double. The value is therefore the
*               Unix time - the number of seconds.microseconds
*               since the beginning of the Universe (according to Unix)
*
* inputs: 
*
* outputs:
*        returns the timestamp.
*
* notes: 
*
***********************************************************/
double timestamp() 
{
  struct timeval tv;
  if (gettimeofday(&tv, NULL) < 0) { 
    printf("utils:timestampt: gettimeofday failed, errno: %d \n",errno); 
  }
  return ((double)tv.tv_sec + ((double)tv.tv_usec / 1000000));
}

/***********************************************************
* Function: int getTime(struct timeval *callersTime);
*
* Explanation:  This returns the current time
*               using the standard timeval format.
*
* inputs: 
*  struct timeval *callersTime - the caller's ptr to hold the time
*
* outputs:
*        returns SUCCESS or ERROR.
*
* notes: 
*
***********************************************************/
int getTime(struct timeval *callersTime)
{
int rc = SUCCESS;

  if (gettimeofday(callersTime, NULL) < 0) { 
     rc = ERROR;
     printf("utils:timestampt: gettimeofday failed, errno: %d \n",errno); 
  } 
  return rc;
}


void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(EXIT_FAILURE);
}

//Returns timeval in seconds with usecond precision
double convertTimeval(struct timeval *t) 
{
  return ( t->tv_sec + ( (double) t->tv_usec)/1000000 );
}


void die(const char *msg) {
  if (errno == 0) {
    /* Just the specified message--no error code */
    puts(msg);
  } else {
    /* Message WITH error code/name */
    perror(msg);
  }
  printf("Die message: %s \n", msg);
  
  /* DIE */
  exit(EXIT_FAILURE);
}


/***********************************************************
* Function: int nanodelay(int64_t ns)
*
* Explanation:  This delays the specified number of nanoseconds
*
* inputs: 
*    int64_t ns : number of ns to delay 
*
* outputs:
*        returns FAILURE or SUCCESS
*
* notes: 
*
***********************************************************/
int nanodelay(int64_t ns)
{
struct timespec req, rem;
int rc = SUCCESS;

  req.tv_sec = 0;

  while (ns >= 1000000000L) {
        ns -= 1000000000L;
        req.tv_sec += 1;
  }

  req.tv_nsec = ns;

  while (nanosleep(&req, &rem) == -1) {
        if (EINTR == errno)
            memcpy(&req, &rem, sizeof(rem));
        else {
          rc = FAILURE;
          printf("delay: nanosleep return in error, rem.t_sec:%d t_nsec:%d,  errno: %d \n",(int) rem.tv_sec,(int) rem.tv_nsec,errno);
          // return -1;
        }
  }

  return  rc;
}


/*************************************************************
* Function: int myDelay(double delayTime)
* 
* Summary: Delays the specified amount of time 
*
* Inputs:   double delayTime:  the time to delay in seconds
*
* outputs:  
*   returns  ERROR or SUCCESS
*
* Set CHECK_SLEEP_TIME to measure actual time
*
*************************************************************/
int myDelay(double delayTime)
{
  int rc = SUCCESS;

  int64_t ns = (int64_t) (delayTime * 1000000000.0); 
//  printf("myDelay:  delayTime:%f   %ld \n",delayTime, ns);

  rc = nanodelay(ns);
  return rc;
}


static ssize_t my_read(int fd, char *ptr)
{
static int	read_cnt = 0;
static char	*read_ptr;
static char	read_buf[MAXLINE];

  if (read_cnt <= 0) {
again:
    if ( (read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0) 
    {
      if (errno == EINTR)
        goto again;
      return(-1);
    } else if (read_cnt == 0)
      return(0);

    read_ptr = read_buf;
  }

  read_cnt--;
  *ptr = *read_ptr++;
  return(1);
}

ssize_t readline(int fd, void *vptr, size_t maxlen)
{
int n, rc;
char c, *ptr;

ptr = (char *)vptr;
  for (n = 1; n < (int)maxlen; n++) {
    if ( (rc = my_read(fd, &c)) == 1) 
    {
      *ptr++ = c;
      if (c == '\n')
        break;	/* newline is stored, like fgets() */
    } else if (rc == 0) 
    {
      if (n == 1)
         return(0);	/* EOF, no data read */
      else
        break;		/* EOF, some data was read */
    } else
      return(-1);		/* error, errno set by read() */
  }

  *ptr = 0;	/* null terminate like fgets() */
  return(n);
}
/* end readline */

ssize_t Readline(int fd, void *ptr, size_t maxlen)
{
ssize_t	n;

  if ( (n = readline(fd, ptr, maxlen)) < 0) {
    printf("Readline:  readline error\n");
    exit(EXIT_FAILURE);
  }
  return(n);
}

void err_sys(const char *fmt, ...)
{
  exit(1);
}



void sig_chld(int signo)
{
pid_t	pid;
int stat;

  printf("sigchldwaitpid(%d): Entered\n", (int) getpid());
  while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0) {
  }
  printf("sigchldwaitpid(%d): Exiting with pid:%d and stat:%d \n", (int) getpid(),pid, stat);
  return;
}

