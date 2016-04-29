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
 *	File		: NX_OMXSemaphore.h
 *	Brief		: Semaphore
 *	Author		: HyunChul Jun (hcjun@nexell.co.kr)
 *	History		: 2016.04.25 : Create
 */

#ifndef __NX_Semaphore_h__
#define __NX_Semaphore_h__

#define		NX_ESEM_TIMEOUT			ETIMEDOUT			/* Timeout */
#define		NX_ESEM					-1
#define		NX_ESEM_OVERFLOW		-2					/* Exceed Max Value */

#include <inttypes.h>

typedef struct _NX_SEMAPHORE NX_SEMAPHORE;

struct _NX_SEMAPHORE{
	uint32_t nValue;
	uint32_t nMaxValue;
	pthread_cond_t hCond;
	pthread_mutex_t hMutex;
};

NX_SEMAPHORE *NX_CreateSem( uint32_t initValue, uint32_t maxValue );
void NX_DestroySem( NX_SEMAPHORE *hSem );
int NX_PendSem( NX_SEMAPHORE *hSem );
int NX_PostSem( NX_SEMAPHORE *hSem );
//int32 NX_PendTimedSem( NX_SEMAPHORE *hSem, uint32 milliSeconds );

#endif	/* __NX_Semaphore_h__ */
