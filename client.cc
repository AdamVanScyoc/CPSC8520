/*********************************************************
* Module Name: UDP Echo client source 
*
* File Name:    UDPEchoClient2.c
*
* Summary:
*  This file contains the client portion of a client/server
*         UDP-based performance tool.
*  
* Usage :   client  
*             <Server IP> 
*             <Server Port> 
*             [<Iteration Delay (usecs)>]    
*             [<Message Size (bytes)>] 
*             [<# of iterations>]    (0 means forever)
*             [<traceFile > }
*
* Global vars
*     debugLevel:  0: no printfs;  1: small amount such as warnings and results
*
* Design/implementation notes:
*     The client uses one buffer to send and then to receive the echo'ed msg
*            char echoBuffer[ECHOMAX+1];  
*
*     The message is defined as:
*       octets 0-3 : seq number
*       octets 4-7 : seconds of timestamp
*       octets 8-11 : microseconds of timestamp
*
*     The client maintains this variable as the next seq number to send
*          unsigned int seqNumber = 1;
*
*     The received seq number in the echo is placed in this var
*             unsigned int RxSeqNumber = 1;
* 
*     A set of ptrs are initialized at program init time
*       echoString = (char *) echoBuffer;
*       intSendBufPtr = (int *)echoBuffer;
*       intRxBufPtr = (int *)echoBuffer;
*
*     A set of timestamps are used to monitor 
*     the observed RTT and the one way latency (OWD)
*     The client's OWD is the delay from the server to the client
*     These are the time related variables used:
*        theTime1;
*        theTime2;
*        theTime3;
*        curRTT 
*        curLATENCY
*
* Revisions:
*  Last update: 2/16/2021
*
*********************************************************/
#include "UDPEcho.h"
#include "ExpRNGObject.h"

void myUsage();
void clientCNTCCode();
void CatchAlarm(int ignored);
void exitProcessing(double curTime);



//Define this globally so our async handlers can access

char *serverIP = NULL;                   /* IP address of server */
int sock = -1;                         /* Socket descriptor */
int bStop;
FILE *newFile = NULL;
double startTime = 0.0;
double endTime = 0.0;
double  duration = 0.0;


//CreateDataFileFlag determines if an output file is produced
//          0 : do not create; 1:min output to file
//          2 : more details
unsigned int createDataFileFlag = 0;
//Set to 1 is output file set



//Counters to assess performance
unsigned int numberOfTimeOuts=0;
unsigned int numberOfTrials=0; /*counts number of attempts */
unsigned int numberRTTSamples = 0;
unsigned int numberOfSocketsUsed = 0;
unsigned int numberLost=0;
unsigned int numberOutOfOrder=0;


long avgPing = 0.0; /* final mean */
long totalPing = 0.0;
long minPing = 100000000;
long maxPing = 0;

double curRTT = 0.0;
double sumOfCurRTT = 0.0;
double meanCurRTT = 0.0; /* final mean */
double smoothedRTT = 0.0;

//ONE WAY LATENCY as a double
unsigned int numberLATENCYSamples = 0;
double curLATENCY = 0.0;
double sumOfCurLATENCY = 0.0;
double meanCurLATENCY = 0.0; /* final mean */
double smoothedLATENCY = 0.0;

unsigned int largestSeqSent = 0;
unsigned int largestSeqRecv = 0;


double RTTSample = 0.0;
double sumOfRTTSamples = 0.0;

unsigned int debugLevel = 4;
unsigned int errorCount = 0;  /* each loop iteration, any/all errors increment */
unsigned int  outOfOrderArrivals = 0;

unsigned int nIterations = 0;
int avgMessageSize;                  /* PacketSize*/

long long delay = 0;		     /* Iteration delay in microseconds */
double averageSendRate = 0.0;
double iterationDelay = 0.0;	     /* Iteration delay in seconds */

long long bytesSent = 0;

void myUsage()
{

  fprintf(stderr, "UDPEchoClient(%s): usage: <Server IP> <Server Port> [<averageSendRate>] [<Average Message Size (bytes)>] [<# of iterations>] [<traceFile>]\n", getVersion());
  fprintf(stderr, "   Example, to send 10 byte msgs 1000  iterations placing samples in a file \n");
  fprintf(stderr, "        ./client localhost 5000 1000000 10 1000 RTT.dat \n");
  fprintf(stderr, "   Example, to run forever:  ./client localhost 5000 1000000 1000 0 RTT.dat \n");
}



/*************************************************************
*
* Function: Main program for  UDPEcho client
*           
* inputs: 
*  Usage :   client  
*             <Server IP> 
*             <Server Port> 
*             [<Iteration Delay (usecs)>] 
*             [<Message Size (bytes)>] 
*             [<# of iterations>] 
*             [<trace file  >
*
*
* outputs:  
*
* notes: 
*    
*   Timeval struct defines the two components as long ints
*         The following nicely printers: 
*         printf("%ld.%06ld\n", usage.ru_stime.tv_sec, usage.ru_stime.tv_usec);
*
***************************************************************/
int main(int argc, char *argv[])
{
  struct sockaddr_in serverAddress; /* Echo server address */
  //struct sockaddr_in clientAddress; /* Source address of echo */ 
  //unsigned int clientAddressSize = 0;   /* In-out of address size for recvfrom() */
  unsigned short serverPort;        /* Echo server port */

  char *echoString;                /* String to send to echo server */
  char echoBuffer[ECHOMAX+1];      /* Buffer for receiving echoed string */
  int echoStringLen;               /* Length of string to echo */
  //int respStringLen;               /* Length of received response */
  struct hostent *thehost;	     /* Hostent from gethostbyname() */


  char traceFile[MAX_TMP_BUFFER];
  char stringA[] = "?";
  double curTime = 0.0;
  //double curTime3 = 0.0;

  struct timeval *theTime1;
  //struct timeval *theTime2;
  //struct timeval *theTime3;
  struct timeval TV1; //, TV2, TV3;
  struct sigaction myaction;
  //long usec1, usec2; //, curPing;
  int *intSendBufPtr;
  //int *intRxBufPtr;
  unsigned int seqNumber = 1;
  //unsigned int RxSeqNumber = 1;
  struct timespec reqDelay;
  //unsigned int numberResponses = 0;
  //unsigned int totalReceivedMsgs = 0;


  int rc  = SUCCESS;
  int loopFlag = 1;
  bool loopForeverFlag = true; 
  double timeStart= -1.0; //internal variables
  double timeStop= -1.0;
  double actualSleep = -1.0; //used to monitor the actual sleep time
  double sleepRemainder = -1.0;  //monitors if nanosleep exits early 


  //tmp variables used for various calculations...
  int i = 0;

  int messageSize = 0;

  // Instantiate 2 RNGs for message size and delay
  ExpRNGObject myMsgRNG;
  myMsgRNG.setRange(1,20*1024);
  ExpRNGObject myDelayRNG;
  myDelayRNG.setRange(10000, 1000000);

    theTime1 = &TV1;  //When we send

    //theTime2 = &TV2;  //When we get the ACK
    //theTime3 = &TV3;  //The time in the ACK

    //Initialize values
    numberOfTimeOuts = 0;
    numberOfTrials = 0;
    totalPing =0;
    bStop = 0;

    startTime = timestamp();
    curTime = startTime;

    setVersion(VersionLevel);
    printf("%s(Version:%s) pid:%d Entered with %d arguements\n", argv[0], getVersion(),getpid(), argc);

    if (argc <= 3)    /* need at least server name and port */
    {
      myUsage();
      exit(1);
    }

    signal (SIGINT, (__sighandler_t)clientCNTCCode);

    serverIP = argv[1];           /* First arg: server IP address (dotted quad) */

    delay = 10000000;                /*units microseconds....init to something large  */
    iterationDelay = ((double)delay)/1000000;/* Iteration delay in seconds */
    avgMessageSize = 32;
    nIterations = 0;

    /* get info from parameters , or default to defaults if they're not specified */
    //chk if asking for help
    if (argc == 2) {
      if (strcmp(stringA, argv[1])) {
        myUsage();
      }
    }
    if (argc == 3) {
       serverPort = atoi(argv[2]);
    }
    else if (argc == 4) {
       serverPort = atoi(argv[2]);
       averageSendRate = atoll(argv[3]);
    }
    else if (argc == 5) {
       serverPort = atoi(argv[2]);
       averageSendRate = atoll(argv[3]);
       avgMessageSize = atoi(argv[4]);
       if (avgMessageSize > ECHOMAX)
         avgMessageSize = ECHOMAX;
    }
    else if (argc == 6) {
      serverPort = atoi(argv[2]);
      averageSendRate = atoll(argv[3]);
      avgMessageSize = atoi(argv[4]);
      if (avgMessageSize > ECHOMAX)
        avgMessageSize = ECHOMAX;
      nIterations = atoi(argv[5])-1;
    }
    else if (argc == 7) {
      serverPort = atoi(argv[2]);
      averageSendRate = atoll(argv[3]);
      avgMessageSize = atoi(argv[4]);
      if (avgMessageSize > ECHOMAX)
        avgMessageSize = ECHOMAX;
      nIterations = atoi(argv[5])-1;

      char *tmpptr = argv[6];
      int stringSize = strnlen(tmpptr,sizeof(traceFile));
      printf("UDPEchoClient: Entered with %d args, server:%s  port:%d and traceFile: %s,  stringlength:%d \n",
          argc, serverIP, serverPort,  tmpptr,(int) strnlen(tmpptr,stringSize));
      memcpy(traceFile, tmpptr, stringSize);
      createDataFileFlag = 1;
    } 
    else {
      myUsage();
      exit(1);
    }  

 
    if (createDataFileFlag > 0  ) {
      newFile = fopen(traceFile, "w");
      if ((newFile ) == NULL) {
        printf("UDPEchoClient(%s): HARD ERROR failed fopen of file %s,  errno:%d \n", 
             argv[0],traceFile,errno);
        exit(1);
      }
    }

	// Calculate delay in microseconds froma verage send rate.
	//delay = averageSendRate / avgMessageSize*8 + 1000000;
	//delay = averageSendRate/(avgMessageSize*8*1000000);
	delay = (avgMessageSize*8*1000000)/averageSendRate;
	//delay = myDelayRNG.getVariate_d();
    iterationDelay = ((double)delay)/1000000;/* Iteration delay in seconds */
	//iterationDelay = myDelayRNG.getVariate_d() + 1000.0;
	//iterationDelay = 0.5;

    if (debugLevel > 1) {
      printf("UDPEchoClient(%s): IP:port:%s:%d #params:%d delay %lld iterationDelay %f avgMessageSize %d nIterations %d createDataFileFlag%d  \n", 
        argv[0],  serverIP, serverPort, argc,
        delay, iterationDelay, avgMessageSize, nIterations, createDataFileFlag);
    }


    myaction.sa_handler = CatchAlarm;
    if (sigfillset(&myaction.sa_mask) < 0){
      errorCount++;
      DieWithError("sigfillset() failed");
    }

    myaction.sa_flags = 0;

    if (sigaction(SIGALRM, &myaction, 0) < 0)  {
      errorCount++;
      DieWithError("sigaction failed for sigalarm");
    }

    /* Set up the echo string */

    messageSize = avgMessageSize + (rand() % (int)avgMessageSize*0.25)*(-1 * ((rand() % 2) + 1));
	echoStringLen = messageSize;

    echoString = (char *) echoBuffer;
    for (i=0; i<messageSize; i++) {
       echoString[i] = 0;
    }
    echoString[messageSize-1]='\0';

    intSendBufPtr = (int *)echoBuffer;
    //intRxBufPtr = (int *)echoBuffer;


    /* Construct the server address structure */
    memset(&serverAddress, 0, sizeof(serverAddress));    /* Zero out structure */
    serverAddress.sin_family = AF_INET;                 /* Internet addr family */
    serverAddress.sin_addr.s_addr = (in_addr_t)inet_addr(serverIP);  /* Server IP address */
    
    /* If user gave a dotted decimal address, we need to resolve it  */
    if (serverAddress.sin_addr.s_addr == (unsigned int)-1) {
        thehost = gethostbyname(serverIP);
	    serverAddress.sin_addr.s_addr = *((unsigned long *) thehost->h_addr_list[0]);
    }
    
    serverAddress.sin_port   = htons(serverPort);     /* Server port */
    /* Create a datagram/UDP socket */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
      errorCount++;
      DieWithError("socket() failed");
    } else 
      numberOfSocketsUsed++;

    if (nIterations==0)
      loopForeverFlag=true;
    else 
      loopForeverFlag=false;
	
    //Delay the correct amount of time
    timeStart=timestamp(); 
    rc = myDelay(iterationDelay);
    if (rc == FAILURE){
       printf("UDPEchoClient2:(%f): HARD ERROR:  myDelay returned an error ??? %d  \n", curTime, rc);
        exit(EXIT_FAILURE);
    }
    timeStop=timestamp(); 
    actualSleep = timeStop - timeStart;
    sleepRemainder = actualSleep - iterationDelay;  

    if (debugLevel > 3) {
      printf("UDPEchoClient2: actualSleep:%f, sleepRemainder:%f \n",actualSleep, sleepRemainder);
    }


    while ( (loopFlag>0) && (bStop != 1))
    {

      if (!loopForeverFlag ) 
      {
        if (nIterations > 0) 
        {
          nIterations--;
        }
        else
          loopFlag = 0;
      }
      if (errorCount > ERROR_LIMIT) {
        if (debugLevel > 1) {
          printf("UDPEchoClient:(%f): WARNING :  errorCount %d, hit ERROR_LIMIT \n ",
                 curTime,errorCount);
        }
        errorCount = 0;
        //try to reset things.....
        /* Create a datagram/UDP socket */
        if (sock != -1) 
          close(sock);
        sock = -1;
        if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
          errorCount++;
          printf("UDPEchoClient:(%f): HARD ERROR errorCount %d, socket reset failed, errno:%d \n ",
                 curTime,errorCount,errno);
          perror("UDPEchoClient error on recvfrom \n");
          exit(1);
        } else 
          numberOfSocketsUsed++;
      }
      numberOfTrials++;
      largestSeqSent = seqNumber;
      gettimeofday(theTime1, NULL);

      intSendBufPtr = (int *)echoBuffer;
      *intSendBufPtr++ =  (unsigned int)htonl(seqNumber++); 
      *intSendBufPtr++ =  (unsigned int)htonl(theTime1->tv_sec);
      *intSendBufPtr++ =  (unsigned int)htonl(theTime1->tv_usec);

      curTime = convertTimeval(theTime1);
      if (startTime == 0.0 ) {
        startTime = curTime;
      }

      if (debugLevel > 3) {
        printf("UDPEchoClient: iteration:%d, sending seq:%d \n", numberOfTrials, (seqNumber - 1));
      }



	  // Ensure delay is an exponentiatl random variable.
	  //delay = myDelayRNG.getVariate_d();

      iterationDelay = ((double)delay)/1000000;/* Iteration delay in seconds */
      reqDelay.tv_sec = (uint32_t)floor(iterationDelay);

      if (reqDelay.tv_sec >= 1)
        reqDelay.tv_nsec = (uint32_t)( 1000000000 * (iterationDelay - (double)reqDelay.tv_sec));
      else
        reqDelay.tv_nsec = (uint32_t) (1000000000 * iterationDelay);


      if (debugLevel > 3) {
        printf("UDPEchoClient2:iteratonDelay:%f nanosleep for %ld.%09ld \n", 
              iterationDelay, reqDelay.tv_sec, reqDelay.tv_nsec);
      }


      //Delay the correct amount of time
      timeStart=timestamp(); 
      rc = myDelay(iterationDelay);
      if (rc == FAILURE){
         printf("UDPEchoClient2:(%f): HARD ERROR:  myDelay returned an error ??? %d  \n", curTime, rc);
          exit(EXIT_FAILURE);
      }
      timeStop=timestamp(); 
      actualSleep = timeStop - timeStart;
      sleepRemainder = actualSleep - iterationDelay;  

      if (debugLevel > 3) {
        printf("UDPEchoClient2: actualSleep:%f, sleepRemainder:%f \n",actualSleep, sleepRemainder);
      }

      messageSize = avgMessageSize + (rand() % (int)avgMessageSize*0.25)*(-1 * ((rand() % 2) + 1));
	  echoStringLen = messageSize;

      echoString = (char *) echoBuffer;
      for (i=0; i<messageSize; i++) {
         echoString[i] = 0;
      }
      echoString[messageSize-1]='\0';

      //intSendBufPtr = (int *)echoBuffer;

     /* Send the message to the server */
      if (debugLevel > 3) {
        printf("UDPEchoClient: Sending seqNum:%d (%d bytes) to the server: %s, next seq#timed:%d \n", 
             seqNumber - 1, echoStringLen,serverIP, outOfOrderArrivals);
      }

	  rc =sendto(sock, echoString, echoStringLen, 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
	  if (rc < 0) 
	  {
		  errorCount++;
		  printf("UDPEchoClient:(%f) HARD error: : send error (tried %d bytes), errno:%d \n",curTime,echoStringLen,errno);
		  perror("UDPEchoClient error on sendto  \n");
		  continue; 
	  } 
	  else 
	  {
		//Else the send succeeded
		if (debugLevel > 3) {
		  printf("UDPEchoClient: Sent completed, rc = %d \n",rc);
		}

		bytesSent += echoStringLen;

		/*
		clientAddressSize = sizeof(clientAddress);
		alarm(2);            //set the timeout for 2 seconds

		//Time1: When we send the msg
		//    Time2-Time1 = RTT
		//Time2: when an ACK arrives
		//Time3:  Time contained in the ACK
		//       Time3 - curTime  - oneway time sender to client
		// Recv a response 
		respStringLen = recvfrom(sock, echoBuffer, ECHOMAX, 0, (struct sockaddr *) &clientAddress, &clientAddressSize);
		alarm(0);            //clear the timeout 
		gettimeofday(theTime2, NULL);
		curTime = convertTimeval(theTime2);
		numberResponses++;
		if (respStringLen < 0) 
		{
		  //If an error occurs,  we basically ignore it-  the next iteration
		  //we send the next sequence number -  we do not retransmit the sequence number that was dropped.
		  // EINTR means the recvfrom was prematurely unblocked because the alarm popped
		  if (errno == EINTR) 
		  { 
			//numberOfTimeOuts++; 
			printf("UDPEchoClient:(%f) WARNING :Received Timeout !! #TOs:%d errorCount:%d  \n",
				   curTime,numberOfTimeOuts,errorCount);
			continue; 
		  } else {
			//else some other error....
			errorCount++;
			printf("UDPEchoClient:(%f) WARNING : Rx error errorCount:%d, errno:%d  \n",
				   curTime,errorCount,errno);
			continue; 
		  }
		} 
		else 
		{
		  if (respStringLen != echoStringLen) 
		  {
			errorCount++;
			printf("UDPEchoClient:(%f) WARNING : Rx'ed (%d) less than expected %d)  errorCount:%d  \n",
				   curTime,respStringLen, echoStringLen,errorCount);
			continue; 
		  }
		} 


		//If we get here, either no error or we aren't worried about an error....move forward:
		//And...theTime2 and curTime is updated
		if (debugLevel > 3) {
		  printf("UDPEchoClient:(%f): Received %d bytes from  %s \n", curTime, respStringLen, serverIP);
		}

		intRxBufPtr = (int *)echoBuffer;
		RxSeqNumber = (unsigned int)ntohl( (unsigned int)(*intRxBufPtr++));
		theTime3->tv_sec = (unsigned int)ntohl( (unsigned int)(*intRxBufPtr++));
		theTime3->tv_usec = (unsigned int)ntohl( (unsigned int)(*intRxBufPtr++));
		curTime3 = convertTimeval(theTime3);
		curLATENCY = curTime - curTime3;
		smoothedLATENCY  = 0.5 * smoothedLATENCY + 0.5*curLATENCY;
		sumOfCurLATENCY += curLATENCY;
		numberLATENCYSamples++;

		//update the largest rx seq number we have ever seen
		if (RxSeqNumber > largestSeqRecv) {
		  largestSeqRecv = RxSeqNumber;
		}  
		totalReceivedMsgs++;


		if (debugLevel > 3) {
		  printf("UDPEchoClient: Received seqNumber: %d , and outOfOrderArrivals:%d, errorCount:%d \n",
			  RxSeqNumber,outOfOrderArrivals,errorCount);
		}

		//At this point we have the RxSeqNumber of the reply message that just arrived.
		//Three possibilities for what arrives based on what was sent (saved in largestSeqSent)
		//   1  Rx SeqNumber is less largestSeqSent -  means that an out of order message arrived
		//   2  Rx SeqNumber is greater than  largestSeqSent -  should never happen
		//   3  Rx SeqNumber  equals largestSeqSent - this is the case if no errors have occurred
		//   If the client ever sends a window of messages before any replies arrive then
		//     there are more possibilities.


		if (RxSeqNumber > largestSeqSent) {
			printf("UDPEchoClient2:(%f): HARD ERROR:  RxSeqNumber:%d largestSeqSent:%d  \n",
			  curTime,  RxSeqNumber, largestSeqSent);
			exit(1);
		}

		if (RxSeqNumber< largestSeqSent) {
		  outOfOrderArrivals++;
		  if (debugLevel >1 ) {
			printf("UDPEchoClient2:(%f): WARNING OutOfOrder(gap%d) RxSeqNumber:%d largestSeqSent:%d , #outOfOrder:%d  \n",
			  curTime, largestSeqSent -  RxSeqNumber, RxSeqNumber,largestSeqSent, outOfOrderArrivals);
		  }
		} 

		//Note that if a timeout occurs, we would not get here for the iteration that failed

		//Warning: the following are longs and not doubles....
		usec2 = (theTime2->tv_sec) * 1000000 + (theTime2->tv_usec);
		usec1 = (theTime1->tv_sec) * 1000000 + (theTime1->tv_usec);
		curPing = (usec2 - usec1);
		totalPing += curPing;

		numberRTTSamples++;

		curRTT = ((double)curPing) / 1000000;
		smoothedRTT  = 0.5 * smoothedRTT + 0.5*curRTT;
		sumOfCurRTT += curRTT;
		numberLost = numberOfTrials - numberRTTSamples;

		if (debugLevel > 3) {
		  printf("UDPEchoClient2(%f): curRTT:%f, smoothRTT:%f currPing:%ld actualSleep:%f RxSeqNumber:%d #TOs:%d #Lost:%d, #Errors:%d \n",
			   curTime,curRTT, smoothedRTT, curPing, actualSleep, RxSeqNumber, numberOfTimeOuts,numberLost, errorCount);
		}

		if (createDataFileFlag == 1 ) {
		  fprintf(newFile,"%f %3.6f %3.6f %d %d %d \n", 
			   curTime,curRTT, smoothedRTT, RxSeqNumber,respStringLen,  numberLost );
		}
		//show more information
		if (createDataFileFlag == 2 ) {
		  fprintf(newFile,"%f %3.6f  %3.6f %3.6f %3.6f  %d %d %d %d %d\n",
			   curTime, curRTT, smoothedRTT, curLATENCY, smoothedLATENCY, 
			   RxSeqNumber, respStringLen,numberLost, numberOfTimeOuts, errorCount);
		}
		if (debugLevel > 0) {
		  printf("%f %3.6f %3.6f %d %d %d \n", 
			   curTime,curRTT, smoothedRTT, RxSeqNumber,respStringLen,  numberLost );
		}
*/
	  }

    }

    //If we get here, we've ended because iterations finished
    exitProcessing(curTime);
    exit(0);
}

void CatchAlarm(int ignored) 
{ 

  //Since the main program loop catches this event, we don't need to do anything here.
  if (debugLevel > 3) {
    printf("UDPEchoClient2:CatchAlarm:(signal:%d): #iterations:%d,  #timeouts:%d \n", 
          ignored, numberOfTrials,numberOfTimeOuts);
  }

}

void clientCNTCCode() {

  bStop = 1;
  exitProcessing(timestamp());
  exit(0);
}


void exitProcessing(double curTime) 
{
  double avgLoss = 0.0;

  endTime = curTime;
  duration = endTime - startTime;
  if (sock != -1) 
    close(sock);
  sock = -1;
  if (numberRTTSamples != 0) {
    avgPing = (totalPing/numberRTTSamples);
    meanCurRTT = (sumOfCurRTT/numberRTTSamples);
  }
  if (numberLATENCYSamples > 0) {
     meanCurLATENCY =  sumOfCurLATENCY / numberLATENCYSamples;
  }
  if (largestSeqSent > 0) 
    avgLoss = (((double)numberLost*100)/largestSeqSent);

  // ExpTrafficGenClient:  averageSendRate averageMsgSize  numberOfIterations
  // TODO 
  printf("\nExpTrafficGenClient:  bytesSent: %lld throughput: %f avgMessageSize: %d numberIterations: %d duration: %f\n", bytesSent, (numberOfTrials*bytesSent*8)/duration, avgMessageSize, nIterations, duration);
  //printf("\nExpTrafficGenClient:  throughput: %f avgMessageSize: %d numberIterations: %d numberIterations*iterationDelay: %f\n", (numberOfTrials*avgMessageSize*8)/(numberOfTrials*iterationDelay), avgMessageSize, nIterations, numberOfTrials*iterationDelay);

  if (debugLevel > 0) {
    //printf("\nUDPEchoClient[%s:%f:%f:%s]:  \n\tmeanRTT:%3.6f smoothedRTT:%3.6f, meanLatency:%3.6f,avgLoss:%2.5f numberTrials:%d, numberLost:%d\n",

  }
  if (debugLevel > 1) {
    printf("UDPEchoClient[%s:%f:%f:%s]:  \n\tmeanRTT:%3.6f smoothedRTT:%3.6f, meanLatency:%3.6f, avgLoss:%2.5f numberTrials:%d, numberLost:%d\n",
               getVersion(), curTime,duration,serverIP, meanCurRTT, smoothedRTT, meanCurLATENCY,
               avgLoss, numberOfTrials, numberLost);

	/*
    printf("\n\t samples/#Lost/#TOs/OutofOrders/#Sockets: :%d:%d:%d:%d:%d #errors:%d\n", 
              numberRTTSamples,numberLost, numberOfTimeOuts, outOfOrderArrivals,numberOfSocketsUsed,errorCount);
			  */
  }
  if (newFile != NULL) {
    fflush(newFile);
    fclose(newFile);
  }
}



