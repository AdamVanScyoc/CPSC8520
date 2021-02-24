/*********************************************************
*
* Module Name: UDP Echo client/server header file
*
* File Name:    UDPEcho.h	
*
* Summary:
*  This file contains common stuff for the client and server
*
* Revisions:
*
*********************************************************/
#ifndef	__UDPEcho_h
#define	__UDPEcho_h
#include "common.h" 
#include "utils.h"
#include "version.h"

#define ECHOMAX 10000     /* Longest string to echo */
#define ERROR_LIMIT 5

typedef struct {
  uint16_t size;
  uint16_t mode;
  uint32_t sequenceNum;
  uint32_t timeSentSeconds;
  uint32_t timeSentUSeconds;
  uint32_t timeRxSeconds;
  uint32_t timeRxUSeconds;
} netTime;

typedef struct {
  uint16_t size;
  uint16_t mode;
  time_t timestamp;
  uint32_t sequenceNum;
  struct timeval timeSent;
  uint64_t timeSeconds;
  uint64_t microSeconds;
} messageHeaderLarge;

typedef struct {
  uint16_t size;
  uint16_t mode;
  uint32_t sequenceNum;
  uint32_t timeSentSeconds;
  uint32_t timeSentUSeconds;
  uint32_t timeRxSeconds;
  uint32_t timeRxUSeconds;
  uint32_t OWDelay;         //the sender fills this in if it can estimate the one way latency 
} messageHeader;


#endif

