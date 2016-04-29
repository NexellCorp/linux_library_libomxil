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
 *	File		: NX_OMXBasePort.c
 *	Brief		: OpenMAX IL Base Port
 *	Author		: HyunChul Jun (hcjun@nexell.co.kr)
 *	History		: 2016.04.25 : Create
 */

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <NX_OMXBasePort.h>

OMX_ERRORTYPE NX_InitOMXPort(OMX_PARAM_PORTDEFINITIONTYPE *pPort, OMX_U32 nPortIndex, OMX_DIRTYPE eDir, OMX_BOOL bEnabled, OMX_PORTDOMAINTYPE eDomain)
{
	pPort->nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
	pPort->nVersion.nVersion = 0;
	pPort->nPortIndex = nPortIndex;
	pPort->eDir = eDir;
	pPort->bEnabled = bEnabled;
	pPort->eDomain = eDomain;
	pPort->bPopulated = OMX_FALSE;
	return OMX_ErrorNone;
}

