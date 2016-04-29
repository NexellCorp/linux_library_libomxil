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
 *	File		: NX_OMXBasePort.h
 *	Brief		: OpenMAX IL Base Port
 *	Author		: HyunChul Jun (hcjun@nexell.co.kr)
 *	History		: 2016.04.25 : Create
 */

#include <OMX_Core.h>
#include <OMX_Component.h>

#ifndef __NX_OMXBasePort_h__
#define __NX_OMXBasePort_h__

#define	NX_OMX_MAX_PORTS		4		/* Max number of ports within components */
typedef enum NX_BUFFER_HEADER_TYPE{
	NX_BUFHEADER_TYPE_ALLOCATED,
	NX_BUFHEADER_TYPE_USEBUFFER
}NX_BUFFER_HEADER_TYPE;

OMX_ERRORTYPE NX_BasePortEnable();
OMX_ERRORTYPE NX_BasePortDisable();

OMX_ERRORTYPE NX_InitOMXPort(OMX_PARAM_PORTDEFINITIONTYPE *pPort, OMX_U32 nPortIndex, OMX_DIRTYPE eDir, OMX_BOOL bEnabled, OMX_PORTDOMAINTYPE eDomain);

/*
 * ///////////////////////////////////////////////////////////////////////
 * //					Nexell base port type
 */
typedef struct NX_BASEPORTTYPE{
	OMX_PARAM_PORTDEFINITIONTYPE	stdPortDef;
	OMX_U32							nAllocatedBuf;
	NX_BUFFER_HEADER_TYPE			eBufferType;
}NX_BASEPORTTYPE;
/*
 * //	End of nexell base port type
 * ///////////////////////////////////////////////////////////////////////
 */

#endif	/* __NX_OMXBasePort_h__ */
