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
 *	File		: NX_OMXDebugMsg.h
 *	Brief		: OpenMAX IL Debug Message
 *	Author		: HyunChul Jun (hcjun@nexell.co.kr)
 *	History		: 2016.04.25 : Create
 */ 

#ifndef __NX_OMXDebugMsg_h__
#define __NX_OMXDebugMsg_h__

#define NX_DEBUG

#define	NX_DBG_DISABLE		0
#define NX_DBG_ERROR		1
#define NX_DBG_WARNING		2
#define NX_DBG_INFO			3
#define NX_DBG_DEBUG		4
#define NX_DBG_TRACE		5

#include <inttypes.h>

void NX_DbgSetPrefix( const char *pPrefix );
void NX_DbgSetLevel( uint32_t TargetLevel );
void NX_RelMsg( uint32_t level, const char *format, ... );
void NX_DbgTrace( const char *format, ... );
void NX_ErrMsg( const char *format, ... );

#ifdef ANDROID	/* For Android */
#include <cutils/log.h>
#ifdef NX_DEBUG
#define	NX_LOGE(...)			ALOGE(__VA_ARGS__)
#define	NX_LOGV(...)			ALOGV(__VA_ARGS__)
#define	NX_LOGD(...)			ALOGD(__VA_ARGS__)
#define	NX_LOGI(...)			ALOGI(__VA_ARGS__)
#define	NX_LOGW(...)			ALOGW(__VA_ARGS__)

#ifndef DbgMsg
#define	DbgMsg(...)				ALOGD(__VA_ARGS__)
#endif	/* DbgMsg */
#ifndef	ErrMsg
#define	ErrMsg(...)				ALOGE(__VA_ARGS__)
#endif	/* ErrMsg */

#else
#include <stdio.h>
#define	NX_LOGE(...)			printf(__VA_ARGS__)
#define	NX_LOGV(...)			printf(__VA_ARGS__)
#define	NX_LOGD(...)			printf(__VA_ARGS__)
#define	NX_LOGI(...)			printf(__VA_ARGS__)
#define	NX_LOGW(...)			printf(__VA_ARGS__)

#ifndef DbgMsg
#define	DbgMsg(...)				printf(__VA_ARGS__)
#endif	/* DbgMsg */
#ifndef	ErrMsg
#define	ErrMsg(...)				printf(__VA_ARGS__)
#endif	/* ErrMsg */
#endif

#else			/* For Linux */
#include <stdio.h>

#define	NX_LOGE(...)			NX_RelMsg(NX_DBG_ERROR  , __VA_ARGS__)
#define	NX_LOGV(...)			NX_RelMsg(NX_DBG_TRACE  , __VA_ARGS__)
#define	NX_LOGD(...)			NX_RelMsg(NX_DBG_DEBUG  , __VA_ARGS__)
#define	NX_LOGI(...)			NX_RelMsg(NX_DBG_INFO   , __VA_ARGS__)
#define	NX_LOGW(...)			NX_RelMsg(NX_DBG_WARNING, __VA_ARGS__)

#ifndef DbgMsg
#define	DbgMsg(...)				NX_RelMsg(NX_DBG_DEBUG  , __VA_ARGS__)
#endif	/* DbgMsg */
#ifndef	ErrMsg
#define	ErrMsg(...)				NX_ErrMsg(__VA_ARGS__)
#endif	/* ErrMsg */

#endif

#endif	/* __NX_OMXDebugMsg_h__ */
