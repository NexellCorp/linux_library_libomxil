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
 *	File		: NX_OMXCommon.h
 *	Brief		: OpenMAX IL Common
 *	Author		: HyunChul Jun (hcjun@nexell.co.kr)
 *	History		: 2016.04.25 : Create
 */

#include <OMX_Core.h>
#include <OMX_Component.h>

#ifndef __NX_OMXCommon_h__
#define __NX_OMXCommon_h__

void NX_OMXSetVersion(OMX_VERSIONTYPE *pVer);
/* Buffer Tool */
void NX_InitializeOutputBuffer(OMX_BUFFERHEADERTYPE *pOutBuf);
void NX_InitializeInputBuffer(OMX_BUFFERHEADERTYPE *pInBuf);

const char *GetStateString(OMX_STATETYPE eState);

#endif	/* __NX_OMXCommon_h__ */
