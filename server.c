/*********************************************************
*
* Module Name: UDP Echo server 
*
* File Name:    UDPEchoServer.c	
*
* Summary:
*  This file contains the echo server code
* 
* Program parameters:
*   server port number:
*   fileName for trace data - if not supplied, not created!!
*
* Design/implementation notes:
*     A set of timestamps are used to monitor 
*     the observed one way latency (OWD)
*     The OWD is the delay from the client to the server.
*     These are the time related variables used:
*        theTime1;
*        theTime2;
*        theTime3;
*        curRTT 
*     The server uses a single buffer to receive data- it reuses to send the echo.
*
* Revisions:
*  Last update: 2/16/2021
*
*********************************************************/
#include "UDPEcho.h"

void myUsage();
void CNTCCode();
void CatchAlarm(int ignored);
void exitProcessing(double curTime);

int sock = -1;                         /* Socket descriptor */
int bStop = 1;;
FILE *newFile = NULL;
double startTime = 0.0;
double endTime = 0.0;
double timeOfFirstArrival = -1.0;
double timeOfMostRecentArrival = -1.0;
unsigned int largestSeqRecv = 0;
unsigned int receivedCount = 0;

//Determines if an output data file is created
//Settings: 0 : do not create; 1:min output to file
//          2 : more details
unsigned int createDataFileFlag = 0;
//Set to 1 if a data file param is set

unsigned int numberLost = 0;  

unsigned int numberRTTSamples = 0;
unsigned int numberOfSocketsUsed = 0;
unsigned int numberOutOfOrder=0;

long avgPing = 0.0; /* final mean */
long totalPing = 0.0;
long minPing = 100000000;
long maxPing = 0;

struct timeval *theTime1;
struct timeval *theTime2;
struct timeval *theTime3;
struct timeval TV1, TV2, TV3;

double RTTSample = 0.0;
double sumOfRTTSamples = 0.0;
double meanRTTSample = 0.0; /* final mean */

//ONE WAY LATENCY as a double
unsigned int numberLATENCYSamples = 0;
double curLATENCY = 0.0;
double sumOfCurLATENCY = 0.0;
double meanCurLATENCY = 0.0; /* final mean */
double smoothedLATENCY = 0.0;

//Set to 1 for warnings, 3 for traces
unsigned int debugLevel = 3;
unsigned int errorCount = 0;  /* each loop iteration, any/all errors increment */
unsigned int  outOfOrderArrivals = 0;
double totalByteCount=0;

void myUsage()
{
  fprintf(stderr,"UDPEchoServer:(%s): usage: <UDP SERVER PORT> <trace File Name> \n", getVersion());
  fprintf(stderr,"   Example:  ./server 5000 server.dat \n");
}



int main(int argc, char *argv[])
{
    struct sockaddr_in echoServAddr; /* Local address */
    struct sockaddr_in echoClntAddr; /* Client address */
    unsigned int cliAddrLen;         /* Length of incoming message */
    char echoBuffer[ECHOMAX];        /* Buffer for echo string */
    unsigned short echoServPort;     /* Server port */
    int recvMsgSize;                 /* Size of received message */
    unsigned int loopCount = 0;
    unsigned int errorCount = 0;
    unsigned int RxSeqNumber = 0;
    int rc = 0;
    double curTime = 0.0;

    double curTime3 = 0.0;

    char traceFile[MAX_TMP_BUFFER];
    struct timeval *theTime2,*theTime3;
    struct timeval TV2, TV3;
    int *intSendBufPtr = (int *)echoBuffer;
    int *intRxBufPtr = (int *)echoBuffer;

    theTime2 = &TV2;  //The sending time that is placed in the AC
    theTime3 = &TV3;  //The time that is in the msg that arrived

    bStop = 0;
    startTime = timestamp();
    curTime = startTime;

    setVersion(VersionLevel);
    printf("%s(Version:%s) pid:%d Entered with %d arguements\n", argv[0], getVersion(),getpid(), argc);

    if (argc < 2)         /* Test for correct number of parameters */
    {
        myUsage();
        exit(1);
    }

    signal (SIGINT, CNTCCode);

    echoServPort = atoi(argv[1]);  /* First arg:  local port */
    printf("UDPEchoServer: Entered with %d args, port:%d \n", 
          argc, echoServPort);

    if (argc == 3) { 
      char *tmpptr = argv[2];
      int stringSize = strnlen(tmpptr,sizeof(traceFile));
      printf("UDPEchoServer: Entered with %d args, port:%d and traceFile: %s,  stringlength:%d \n", 
          argc, echoServPort,  tmpptr,(int) strnlen(tmpptr,stringSize));

       //dst src
      memcpy(traceFile, tmpptr, stringSize);
      createDataFileFlag = 1;
    }

    if (createDataFileFlag >0 ) {
      newFile = fopen(traceFile, "w");
      if ((newFile ) == NULL) {
        printf("UDPEchoServer(%s): HARD ERROR failed fopen of file %s,  errno:%d \n", 
             argv[0],traceFile,errno);
        exit(1);
      }
    }

    /* Create socket for sending/receiving datagrams */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
      errorCount++;
      printf("UDPEchoServer(%f): HARD ERROR : Failure on socket call errorCount:%d  errno:%d \n", 
              curTime,errorCount,  errno);
    }



    /* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(echoServPort);      /* Local port */

    rc = bind(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr));
    /* Bind to the local address */
    if (rc < 0) {
          errorCount++;
          printf("UDPEchoServer(%f): HARD ERROR : bind to port %d error, errorCount:%d  errno:%d \n", 
              curTime,echoServPort,errorCount,  errno);
          exit(1);
    }
  
    while ( bStop != 1) 
    {
      loopCount++;
      curTime = timestamp();
      /* Set the size of the in-out parameter */
      cliAddrLen = sizeof(echoClntAddr);

      /* Block until receive message from a client */
      recvMsgSize = recvfrom(sock, echoBuffer, ECHOMAX, 0, (struct sockaddr *) &echoClntAddr, &cliAddrLen);
      curTime = timestamp();
      if (recvMsgSize < 0)
      {
        errorCount++;
        printf("UDPEchoServer(%f)(%d) HARD ERROR : error recvfrom  errorCount%d  errno:%d \n", 
              curTime,loopCount, errorCount,  errno);

        continue;
      }

      receivedCount++;
      totalByteCount+= (double)recvMsgSize;
      timeOfMostRecentArrival = curTime;
      if (timeOfFirstArrival == -1.0)
       timeOfFirstArrival = curTime;


      intRxBufPtr = (int *)echoBuffer;
      //From the rx pkt, get the server side timestamp`

      //Time3 will store the timestamp of the arriving msg
      RxSeqNumber = (unsigned int)ntohl( (unsigned int)(*intRxBufPtr++));
      theTime3->tv_sec = (unsigned int)ntohl( (unsigned int)(*intRxBufPtr++));
      theTime3->tv_usec = (unsigned int)ntohl( (unsigned int)(*intRxBufPtr++));
      curTime3 = convertTimeval(theTime3);

      //curLATENCY is the one way latency estimate for this message. 
      //  IT is this host's wall clock time when the message arrives
      //   minus the sending wall clock time placed in the message by the sender.
      curLATENCY = curTime - curTime3;
      smoothedLATENCY  = 0.5 * smoothedLATENCY + 0.5*curLATENCY;
      sumOfCurLATENCY += curLATENCY;
      numberLATENCYSamples++;


      if (largestSeqRecv < RxSeqNumber)
        largestSeqRecv = RxSeqNumber;

      numberLost = largestSeqRecv - receivedCount;

      if (debugLevel > 3) {
        printf("UDPEchoServer(%f, %f, %f)  rxed %d bytes, RxSeqNumber:%d  errorCount:%d \n",
            curTime,curTime3, curLATENCY, recvMsgSize, RxSeqNumber, errorCount);
      }

      gettimeofday(theTime2, NULL);
      curTime = convertTimeval(theTime2);

      intSendBufPtr = ( (int *)echoBuffer + sizeof(RxSeqNumber));
      //*intSendBufPtr++ =  (unsigned int)htonl(seqNumber++); 
      *intSendBufPtr++ =  (unsigned int)htonl(theTime2->tv_sec);
      *intSendBufPtr++ =  (unsigned int)htonl(theTime2->tv_usec);


      /* Send received datagram back to the client */
/*
      rc = sendto(sock, echoBuffer, recvMsgSize, 0,  (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr));

      if ( rc != recvMsgSize) 
      {
          errorCount++;
          printf("UDPEchoServer(%f):(%d) HARD ERROR : error sending %d bytes to client %s, errorCount%d, rc:%d  errno:%d \n", 
              curTime,loopCount,recvMsgSize, inet_ntoa(echoClntAddr.sin_addr), errorCount, rc,  errno);
          continue;
      }
*/

      //Tries to be consistent with the client samples
      if (createDataFileFlag == 1 ) {
        fprintf(newFile,"%f %3.6f %3.6f %d %d %d \n",
             curTime, curLATENCY, smoothedLATENCY, RxSeqNumber, recvMsgSize, numberLost);
      }
      
      //more info is shown 
      if (createDataFileFlag == 2 ) {
        fprintf(newFile,"%f %3.6f %3.6f %d %d %9.0f %d \n",
             curTime, curLATENCY, smoothedLATENCY, RxSeqNumber, largestSeqRecv,  totalByteCount, numberLost);
      }
      if (debugLevel > 0) {
        printf("%f %3.6f %3.6f %d %d %d \n",
             curTime, curLATENCY, smoothedLATENCY, RxSeqNumber, recvMsgSize, numberLost);
      }

    }
    //If we get here, we are supposed to end....
    exitProcessing(curTime);
    exit(0);
}


void CNTCCode() {

  bStop = 1;
  exitProcessing(timestamp());
  exit(0);
}


void exitProcessing(double curTime) {
  //unsigned int numberLost=0;
  //double avgLoss = 0.0;
  double throughput=0.0;
  //double  duration = 0.0;
  double  duration1 = 0.0;

  endTime = curTime;
  //duration = endTime - startTime;
  duration1 = timeOfMostRecentArrival - timeOfFirstArrival;

  if (sock != -1) 
    close(sock);
  sock = -1;

  numberLost = largestSeqRecv - receivedCount;

  if (duration1 >  0)
    throughput =  (totalByteCount * 8.0) / duration1;
/*
  if ((numberLost > 0) && (largestSeqRecv > 0)) 
    avgLoss = ((double)numberLost*100)/largestSeqRecv;
*/

  if (numberLATENCYSamples > 0) {
     meanCurLATENCY =  sumOfCurLATENCY / numberLATENCYSamples;
  }

  if (debugLevel >0) {
    printf("\nExpTrafficGenServer:    %f\n",
       throughput);
  } 
  if (newFile != NULL) {
    fflush(newFile);
    fclose(newFile);
  }
//  exit(0);
}





