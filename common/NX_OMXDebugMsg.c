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
 *	File		: NX_OMXDebugMsg.c
 *	Brief		: OpenMAX IL Debug Message
 *	Author		: HyunChul Jun (hcjun@nexell.co.kr)
 *	History		: 2016.04.25 : Create
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <NX_OMXDebugMsg.h>

// static uint32_t gst_TargetDebugLevel = NX_DBG_DEBUG;
static uint32_t gst_TargetDebugLevel = NX_DBG_TRACE;
static char gst_Prefix[128] = "[NX_OXM]";

void NX_DbgSetPrefix( const char *pPrefix )
{
	strcpy( gst_Prefix, pPrefix );
}

void NX_DbgSetLevel( uint32_t TargetLevel )
{
	printf("%s >>> NX_DbgSetLevel : from %d to %d\n", gst_Prefix, gst_TargetDebugLevel, TargetLevel);
	gst_TargetDebugLevel = TargetLevel;
}

void NX_RelMsg( uint32_t level, const char *format, ... )
{
	char buffer[1024];
	if( level > gst_TargetDebugLevel )	return;

	{
		va_list marker;
		va_start(marker, format);
		vsprintf(buffer, format, marker);
#ifdef ANDROID
		ALOGD( "%s", buffer );
#else
		printf( "%s %s", gst_Prefix, buffer );
#endif
		va_end(marker);
	}
}

void NX_ErrMsg( const char *format, ... )
{
	char buffer[1024];
	va_list marker;
	va_start(marker, format);
	vsprintf(buffer, format, marker);
#ifdef ANDROID
	ALOGE( "%s", buffer );
#else
	printf( "%s %s", gst_Prefix, buffer );
#endif
	va_end(marker);
}

void NX_DbgTrace( const char *format, ... )
{
#ifdef NX_DEBUG
	char buffer[1024];
	if( NX_DBG_TRACE > gst_TargetDebugLevel )	return;
	{
		va_list marker;
		va_start(marker, format);
		vsprintf(buffer, format, marker);
#ifdef ANDROID
		ALOGD( "%s", buffer );
#else
		printf( "%s %s", gst_Prefix, buffer );
#endif
		va_end(marker);
	}
#endif
}
