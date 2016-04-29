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
 *	File		: NX_OMXSemaphore.c
 *	Brief		: OpenMAX IL Semaphore
 *	Author		: HyunChul Jun (hcjun@nexell.co.kr)
 *	History		: 2016.04.25 : Create
 */

#include <pthread.h>
#include <assert.h>

#include <NX_OMXSemaphore.h>
#include <NX_OMXMem.h>

/* Create & Initialization */
NX_SEMAPHORE *NX_CreateSem( uint32_t initValue, uint32_t maxValue )
{
	/* Create Semaphore */
	NX_SEMAPHORE *hSem = NxMalloc( sizeof(NX_SEMAPHORE) );
	if( NULL == hSem )
		return NULL;
	hSem->nValue = initValue;
	hSem->nMaxValue = maxValue;
	if( 0 != pthread_cond_init( &hSem->hCond, NULL ) )
	{
		NxFree( hSem );
		return NULL;
	}
	if( 0 != pthread_mutex_init( &hSem->hMutex, NULL ) )
	{
		pthread_cond_destroy( &hSem->hCond );
		NxFree( hSem );
		return NULL;
	}
	return hSem;
}

void NX_DestroySem( NX_SEMAPHORE *hSem )
{
	/* Destroy Semaphore */
	if( hSem )
	{
		pthread_cond_destroy( &hSem->hCond );
		pthread_mutex_destroy( &hSem->hMutex );
		NxFree( hSem );
	}
}

int32_t NX_PendSem( NX_SEMAPHORE *hSem )
{
	int32_t error = 0;
	assert( NULL != hSem );
	pthread_mutex_lock( &hSem->hMutex );

	/* If post occurs faster than the thread that requires Pending, */
	/* this signal is ignored, the dead lock may occur if the wait */
	/* If using pthread_cond_wait implement a semaphore, */
	/* because there must be value only if the value is 0 pending */
	if( hSem->nValue == 0 )
		error = pthread_cond_wait( &hSem->hCond, &hSem->hMutex );
	if( 0 != error )
	{
		error = NX_ESEM;
	}
	else
	{
		hSem->nValue --;
	}
	pthread_mutex_unlock( &hSem->hMutex );
	return error;
}

//int32_t NX_PendTimedSem( NX_SEMAPHORE *hSem, uint32_t milliSeconds )
//{
//	return -1;
//}

int32_t NX_PostSem( NX_SEMAPHORE *hSem )
{
	int32_t error = 0;
	assert( NULL != hSem );
	pthread_mutex_lock( &hSem->hMutex );
	if( hSem->nValue >= hSem->nMaxValue )
	{
		error = NX_ESEM_OVERFLOW;
	}
	else
	{
		hSem->nValue ++;
	}
	pthread_cond_signal( &hSem->hCond );
	pthread_mutex_unlock( &hSem->hMutex );
	return error;
}
