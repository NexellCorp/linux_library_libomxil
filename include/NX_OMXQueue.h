/*
 *
 *	Copyright (C) 2016 Nexell Co. All Rights Reserved
 *	Nexell Co. Proprietary & Confidential
 *
 *	NEXELL INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
 *  AND	WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
 *  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
 *  FOR A PARTICULAR PURPOSE.
 *
 *	File		: NX_OMXQueue.h
 *	Brief		: Thread safe Queue
 *	Author		: HyunChul Jun (hcjun@nexell.co.kr)
 *	History		: 2016.04.25 : Create
 */

#ifndef __NX_OMXQueue_h__
#define __NX_OMXQueue_h__

#include <pthread.h>
#include <inttypes.h>

#define NX_MAX_QUEUE_ELEMENT	128

typedef struct NX_QUEUE{
	uint32_t head;
	uint32_t tail;
	uint32_t maxElement;
	uint32_t curElements;
	int32_t bEnabled;
	void *pElements[NX_MAX_QUEUE_ELEMENT];
	pthread_mutex_t	hMutex;
}NX_QUEUE;

int32_t NX_InitQueue( NX_QUEUE *pQueue, uint32_t maxNumElement );
int32_t NX_PushQueue( NX_QUEUE *pQueue, void *pElement );
int32_t NX_PopQueue( NX_QUEUE *pQueue, void **pElement );
int32_t NX_GetNextQueuInfo( NX_QUEUE *pQueue, void **pElement );
uint32_t NX_GetQueueCnt( NX_QUEUE *pQueue );
void NX_DeinitQueue( NX_QUEUE *pQueue );

#endif	/* __NX_OMXQueue_h__ */
