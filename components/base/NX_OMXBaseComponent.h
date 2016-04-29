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
 *	File		: NX_OMXBaseComponent.h
 *	Brief		: OpenMAX IL Base Component
 *	Author		: HyunChul Jun (hcjun@nexell.co.kr)
 *	History		: 2016.04.25 : Create
 */

#ifndef __NX_OMXBaseComponent_h__
#define __NX_OMXBaseComponent_h__

/* Core & STD Header */
#include <OMX_Core.h>
#include <OMX_Component.h>

/* Components Header */
#include <NX_OMXCommon.h>
#include <NX_OMXBasePort.h>

/* Utils */
#include <NX_OMXQueue.h>
#include <NX_OMXSemaphore.h>
#include <NX_OMXDebugMsg.h>
#include <NX_OMXMem.h>

#include <pthread.h>

/* OpenMax IL version 1.0.0.0 */
#define	NXOMX_VER_MAJOR		1
#define	NXOMX_VER_MINOR		0
#define	NXOMX_VER_REVISION	0
#define	NXOMX_VER_NSTEP		0

/* Default Recomanded Functions for Implementation Components */
OMX_ERRORTYPE NX_BaseComponentInit (OMX_COMPONENTTYPE *hComponent);
OMX_ERRORTYPE NX_BaseGetComponentVersion ( OMX_HANDLETYPE hComp, OMX_STRING pComponentName, OMX_VERSIONTYPE* pComponent, OMX_VERSIONTYPE* pSpecVersion, OMX_UUIDTYPE* pComponentUUID);
OMX_ERRORTYPE NX_BaseSendCommand ( OMX_HANDLETYPE hComponent, OMX_COMMANDTYPE Cmd, OMX_U32 nParam1, OMX_PTR pCmdData);
OMX_ERRORTYPE NX_BaseGetParameter (OMX_HANDLETYPE hComponent, OMX_INDEXTYPE nParamIndex,OMX_PTR ComponentParamStruct);
OMX_ERRORTYPE NX_BaseSetParameter (OMX_HANDLETYPE hComp, OMX_INDEXTYPE nParamIndex, OMX_PTR ComponentParamStruct);
OMX_ERRORTYPE NX_BaseGetConfig (OMX_HANDLETYPE hComp, OMX_INDEXTYPE nConfigIndex, OMX_PTR pComponentConfigStructure);
OMX_ERRORTYPE NX_BaseSetConfig (OMX_HANDLETYPE hComp, OMX_INDEXTYPE nConfigIndex, OMX_PTR pComponentConfigStructure);
OMX_ERRORTYPE NX_BaseGetExtensionIndex(OMX_HANDLETYPE hComponent, OMX_STRING cParameterName, OMX_INDEXTYPE* pIndexType);
OMX_ERRORTYPE NX_BaseGetState (OMX_HANDLETYPE hComp, OMX_STATETYPE* pState);
OMX_ERRORTYPE NX_BaseComponentTunnelRequest(OMX_HANDLETYPE hComp, OMX_U32 nPort, OMX_HANDLETYPE hTunneledComp, OMX_U32 nTunneledPort, OMX_TUNNELSETUPTYPE* pTunnelSetup);
OMX_ERRORTYPE NX_BaseSetCallbacks(OMX_HANDLETYPE hComponent, OMX_CALLBACKTYPE* pCallbacks, OMX_PTR pAppData);
OMX_ERRORTYPE NX_BaseComponentDeInit(OMX_HANDLETYPE hComponent);
OMX_ERRORTYPE NX_BaseUseEGLImage(OMX_HANDLETYPE hComponent, OMX_BUFFERHEADERTYPE** ppBufferHdr, OMX_U32 nPortIndex, OMX_PTR pAppPrivate, void* eglImage);
OMX_ERRORTYPE NX_BaseComponentRoleEnum(OMX_HANDLETYPE hComponent, OMX_U8 *cRole, OMX_U32 nIndex);

OMX_ERRORTYPE NX_BaseAllocateBuffer(OMX_HANDLETYPE hComponent, OMX_BUFFERHEADERTYPE** pBuffer, OMX_U32 nPortIndex, OMX_PTR pAppPrivate, OMX_U32 nSizeBytes);
OMX_ERRORTYPE NX_BaseFreeBuffer (OMX_HANDLETYPE hComponent, OMX_U32 nPortIndex, OMX_BUFFERHEADERTYPE* pBuffer);
OMX_ERRORTYPE NX_BaseEmptyThisBuffer (OMX_HANDLETYPE hComp, OMX_BUFFERHEADERTYPE* pBuffer);
OMX_ERRORTYPE NX_BaseFillThisBuffer(OMX_HANDLETYPE hComp, OMX_BUFFERHEADERTYPE* pBuffer);

#define	NX_OMX_MAX_PORTS		4		/* Max number of ports within components */

typedef enum NX_THREAD_CMD{
	NX_THREAD_CMD_INVALID,
	NX_THREAD_CMD_RUN,
	NX_THREAD_CMD_PAUSE,
	NX_THREAD_CMD_EXIT
}NX_THREAD_CMD;

/*
 * ///////////////////////////////////////////////////////////////////////
 * //					Nexell Base Component Type
 */
typedef struct _NX_CMD_MSG_TYPE NX_CMD_MSG_TYPE;
struct _NX_CMD_MSG_TYPE{
	OMX_U32					id;
	OMX_COMMANDTYPE			eCmd;
	OMX_U32					nParam1;
	OMX_PTR					pCmdData;
};
/*
 * //
 * ///////////////////////////////////////////////////////////////////////
 */

#define	NX_OMX_MAX_BUF		128
/*
 * ///////////////////////////////////////////////////////////////////////
 * //					Nexell Base Component Type
 */
typedef struct _NX_BASE_COMPNENT NX_BASE_COMPNENT;
#define NX_BASECOMPONENTTYPE								\
	OMX_COMPONENTTYPE		*hComp;							\
	OMX_U32					nNumPort;						\
	OMX_STRING				compName;						\
	OMX_STRING				compRole;						\
	OMX_UUIDTYPE			compUUID[128];					\
	OMX_PTR					pPort[NX_OMX_MAX_PORTS];		\
	OMX_PTR					pBufQueue[NX_OMX_MAX_PORTS];	\
	OMX_STATETYPE			eCurState;						\
	OMX_STATETYPE			eNewState;						\
	OMX_CALLBACKTYPE		*pCallbacks;					\
	OMX_PTR					pCallbackData;					\
	/*					Command Thread				*/		\
	pthread_t				hCmdThread;						\
	NX_THREAD_CMD			eCmdThreadCmd;					\
	NX_SEMAPHORE			*hSemCmd;						\
	NX_SEMAPHORE			*hSemCmdWait;					\
	NX_QUEUE				cmdQueue;						\
	OMX_U32					cmdId;							\
	/*					Command Procedure			*/		\
	void					(*cbCmdProcedure)(NX_BASE_COMPNENT*, OMX_COMMANDTYPE , OMX_U32 , OMX_PTR );\
	/*					In/Out Port & Buffer		*/		\
	NX_BASEPORTTYPE			*pInputPort;					\
	NX_BASEPORTTYPE			*pOutputPort;					\
	OMX_BUFFERHEADERTYPE	*pInputBuffers[NX_OMX_MAX_BUF];	\
	OMX_BUFFERHEADERTYPE	*pOutputBuffers[NX_OMX_MAX_BUF];\
	/*	Buffer allocation semaphore ( Semaphore )	*/		\
	NX_SEMAPHORE			*hBufAllocSem;					\
	/*	Buffer thread control semaphore ( Mutex )	*/		\
	NX_SEMAPHORE			*hBufCtrlSem;					\
	/*	Buffer status change semaphore ( Event )	*/		\
	NX_SEMAPHORE			*hBufChangeSem;					\
	NX_QUEUE				*pInputPortQueue;				\
	NX_QUEUE				*pOutputPortQueue;				\
/*
 * //	End of nexell base component type
 * ///////////////////////////////////////////////////////////////////////
 */

struct _NX_BASE_COMPNENT{
	NX_BASECOMPONENTTYPE
};

/*
 *
 * Base Component Tools
 *
 */
int SendEvent( NX_BASE_COMPNENT *pBaseComp, OMX_EVENTTYPE eEvent, OMX_U32 param1, OMX_U32 param2, OMX_PTR pEventData );
void NX_BaseCommandThread( void *arg );

#endif	/* __NX_OMXBaseComponent_h__ */
