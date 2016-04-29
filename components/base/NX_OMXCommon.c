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
 *	File		: NX_OMXCommon.c
 *	Brief		: OpenMAX IL Common
 *	Author		: HyunChul Jun (hcjun@nexell.co.kr)
 *	History		: 2016.04.25 : Create
 */

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <NX_OMXBaseComponent.h>
#include <NX_OMXCommon.h>

#ifndef UNUSED_PARAM
#define	UNUSED_PARAM(X)		X=X
#endif

static const char *stPtrOMXStateTable[]={
    "OMX_StateInvalid",
    "OMX_StateLoaded",
    "OMX_StateIdle",
    "OMX_StateExecuting",
    "OMX_StatePause",
    "OMX_StateWaitForResources",
};

void NX_OMXSetVersion(OMX_VERSIONTYPE *pVer)
{
	pVer->s.nVersionMajor = NXOMX_VER_MAJOR;
	pVer->s.nVersionMinor = NXOMX_VER_MINOR;
	pVer->s.nRevision = NXOMX_VER_REVISION;
	pVer->s.nStep = NXOMX_VER_NSTEP;
}

void NX_InitializeOutputBuffer(OMX_BUFFERHEADERTYPE *pOutBuf)
{
	pOutBuf->nFilledLen = 0;
	pOutBuf->hMarkTargetComponent = 0;
	pOutBuf->pMarkData = NULL;
	pOutBuf->nTickCount = 0;
	pOutBuf->nTimeStamp = 0;
	pOutBuf->nFlags = 0;
}

void NX_InitializeInputBuffer(OMX_BUFFERHEADERTYPE *pInBuf)
{
	UNUSED_PARAM(pInBuf);
}

const char *GetStateString(OMX_STATETYPE eState)
{
	if ( eState <= OMX_StateWaitForResources )
		return stPtrOMXStateTable[eState];
	return NULL;
}
