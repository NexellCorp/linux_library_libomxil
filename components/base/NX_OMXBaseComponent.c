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
 *	File		: NX_OMXBaseComponent.c
 *	Brief		: OpenMAX IL Base Component
 *	Author		: HyunChul Jun (hcjun@nexell.co.kr)
 *	History		: 2016.04.25 : Create
 */

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <NX_OMXQueue.h>
#include <NX_OMXBaseComponent.h>
#include <NX_OMXMem.h>
#include <NX_OMXDebugMsg.h>

#ifndef UNUSED_PARAM
#define	UNUSED_PARAM(X)		X=X
#endif

OMX_ERRORTYPE NX_BaseComponentInit (OMX_COMPONENTTYPE *hComponent)
{
	NX_BASE_COMPNENT *pBaseComp = (NX_BASE_COMPNENT *)(hComponent->pComponentPrivate);

	pBaseComp->hComp = hComponent;
	pBaseComp->nNumPort = 0;
	pBaseComp->pCallbacks = NxMalloc( sizeof(OMX_CALLBACKTYPE) );
	if( pBaseComp->pCallbacks == NULL ){
		return OMX_ErrorInsufficientResources;
	}
	pBaseComp->pCallbacks->EventHandler = NULL;
	pBaseComp->pCallbacks->EmptyBufferDone = NULL;
	pBaseComp->pCallbacks->FillBufferDone = NULL;
	pBaseComp->eCurState = OMX_StateLoaded;
	pBaseComp->eNewState = OMX_StateLoaded;

	/* Base overrided functions */
	hComponent->GetComponentVersion    = NX_BaseGetComponentVersion	;
	hComponent->SendCommand            = NX_BaseSendCommand			;
	hComponent->SetConfig              = NX_BaseSetConfig			;
	hComponent->GetExtensionIndex      = NX_BaseGetExtensionIndex	;
	hComponent->GetState               = NX_BaseGetState			;
	hComponent->ComponentTunnelRequest = NX_BaseComponentTunnelRequest;
	hComponent->SetCallbacks           = NX_BaseSetCallbacks		;
	hComponent->UseEGLImage            = NX_BaseUseEGLImage			;
	hComponent->ComponentRoleEnum      = NX_BaseComponentRoleEnum	;
	hComponent->EmptyThisBuffer        = NX_BaseEmptyThisBuffer		;
	hComponent->GetConfig              = NX_BaseGetConfig			;
	hComponent->GetParameter           = NX_BaseGetParameter		;
	hComponent->SetParameter           = NX_BaseSetParameter		;
	hComponent->AllocateBuffer         = NX_BaseAllocateBuffer		;
	hComponent->FreeBuffer             = NX_BaseFreeBuffer			;
	hComponent->FillThisBuffer         = NX_BaseFillThisBuffer		;
	hComponent->ComponentDeInit        = NX_BaseComponentDeInit		;

	return OMX_ErrorNone;
}

OMX_ERRORTYPE NX_BaseGetComponentVersion ( OMX_HANDLETYPE hComp, OMX_STRING pComponentName, OMX_VERSIONTYPE* pComponent, OMX_VERSIONTYPE* pSpecVersion, OMX_UUIDTYPE* pComponentUUID)
{
	NX_BASE_COMPNENT *pBaseComp = (NX_BASE_COMPNENT *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);
	strcpy( pComponentName, (OMX_STRING)pBaseComp->compName );
	*pComponent = ((OMX_COMPONENTTYPE*)hComp)->nVersion;
	*pSpecVersion = ((OMX_COMPONENTTYPE*)hComp)->nVersion;
	NxMemcpy( pComponentUUID, pBaseComp->compUUID, 128 );
	return OMX_ErrorNone;
}

/*
 *
 * openmax il spec v1.1.2 : 3.2.2.2 OMX_SendCommand
 * Description : The component normally executes the command outside the context of the call,
 *  				  though a solution without threading may elect to execute it in context.
 *
 */
OMX_ERRORTYPE NX_BaseSendCommand ( OMX_HANDLETYPE hComponent, OMX_COMMANDTYPE Cmd, OMX_U32 nParam1, OMX_PTR pCmdData)
{
	NX_BASE_COMPNENT *pBaseComp = (NX_BASE_COMPNENT *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
	NX_CMD_MSG_TYPE *pCmdMsg = NxMalloc( sizeof(NX_CMD_MSG_TYPE) );
	pCmdMsg->id = pBaseComp->cmdId++;
	pCmdMsg->eCmd = Cmd;
	pCmdMsg->nParam1 = nParam1;
	pCmdMsg->pCmdData = pCmdData;

	/* Change new state */
	if( OMX_CommandStateSet == Cmd )
	{
		pBaseComp->eNewState = nParam1;
	}

	NX_PushQueue( &pBaseComp->cmdQueue, pCmdMsg );
	NX_PostSem( pBaseComp->hSemCmd );

	return OMX_ErrorNone;
}

OMX_ERRORTYPE NX_BaseGetParameter (OMX_HANDLETYPE hComponent, OMX_INDEXTYPE nParamIndex,OMX_PTR ComponentParamStruct)
{
	NX_BASE_COMPNENT *pBaseComp = (NX_BASE_COMPNENT *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
	OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;
	switch( nParamIndex )
	{
		case OMX_IndexParamPortDefinition:
		{
			OMX_PARAM_PORTDEFINITIONTYPE *pInPortDef = (OMX_PARAM_PORTDEFINITIONTYPE *)ComponentParamStruct;
			if( pInPortDef->nPortIndex >= pBaseComp->nNumPort )
				return OMX_ErrorBadPortIndex;
			pPortDef = (OMX_PARAM_PORTDEFINITIONTYPE *)(pBaseComp->pPort[pInPortDef->nPortIndex]);
			NxMemcpy( ComponentParamStruct, pPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE) );
			break;
		}
		default:
			return OMX_ErrorUnsupportedIndex;
	}
	return OMX_ErrorNone;
}

OMX_ERRORTYPE NX_BaseSetParameter (OMX_HANDLETYPE hComp, OMX_INDEXTYPE nParamIndex, OMX_PTR ComponentParamStruct)
{
	NX_BASE_COMPNENT *pBaseComp = (NX_BASE_COMPNENT *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);
	switch( nParamIndex )
	{
		case OMX_IndexParamPortDefinition:
		{
			OMX_PARAM_PORTDEFINITIONTYPE *pSrcDef = (OMX_PARAM_PORTDEFINITIONTYPE *)ComponentParamStruct;
			OMX_PARAM_PORTDEFINITIONTYPE *pDstDef = NULL;
			if( pSrcDef->nPortIndex >= pBaseComp->nNumPort )
				return OMX_ErrorBadPortIndex;
			pDstDef = (OMX_PARAM_PORTDEFINITIONTYPE *)(pBaseComp->pPort[pSrcDef->nPortIndex]);
			pDstDef->nBufferCountActual = pSrcDef->nBufferCountActual;
			switch(pDstDef->eDomain)
			{
				case OMX_PortDomainAudio:
					NxMemcpy(&pDstDef->format.audio, &pSrcDef->format.audio, sizeof(OMX_AUDIO_PORTDEFINITIONTYPE));
					break;
				case OMX_PortDomainVideo:
					pDstDef->format.video.pNativeRender          = pSrcDef->format.video.pNativeRender;
					pDstDef->format.video.nFrameWidth            = pSrcDef->format.video.nFrameWidth;
					pDstDef->format.video.nFrameHeight           = pSrcDef->format.video.nFrameHeight;
					pDstDef->format.video.nStride                = pSrcDef->format.video.nStride;
					pDstDef->format.video.nSliceHeight           = pSrcDef->format.video.nSliceHeight;
					pDstDef->format.video.xFramerate             = pSrcDef->format.video.xFramerate;
					pDstDef->format.video.bFlagErrorConcealment  = pSrcDef->format.video.bFlagErrorConcealment;
					pDstDef->format.video.eCompressionFormat     = pSrcDef->format.video.eCompressionFormat;
					pDstDef->format.video.eColorFormat           = pSrcDef->format.video.eColorFormat;
					pDstDef->format.video.pNativeWindow          = pSrcDef->format.video.pNativeWindow;
					break;
				case OMX_PortDomainImage:
					pDstDef->format.image.nFrameWidth            = pSrcDef->format.image.nFrameWidth;
					pDstDef->format.image.nFrameHeight           = pSrcDef->format.image.nFrameHeight;
					pDstDef->format.image.nStride                = pSrcDef->format.image.nStride;
					pDstDef->format.image.bFlagErrorConcealment  = pSrcDef->format.image.bFlagErrorConcealment;
					pDstDef->format.image.eCompressionFormat     = pSrcDef->format.image.eCompressionFormat;
					pDstDef->format.image.eColorFormat           = pSrcDef->format.image.eColorFormat;
					pDstDef->format.image.pNativeWindow          = pSrcDef->format.image.pNativeWindow;
					break;
				case OMX_PortDomainOther:
					NxMemcpy(&pDstDef->format.other, &pSrcDef->format.other, sizeof(OMX_OTHER_PORTDEFINITIONTYPE));
					break;
				default:
					return OMX_ErrorBadParameter;
			}
			break;
		}
		default:
			return OMX_ErrorUnsupportedIndex;
	}
	return OMX_ErrorNone;
}

OMX_ERRORTYPE NX_BaseGetConfig (OMX_HANDLETYPE hComp, OMX_INDEXTYPE nConfigIndex, OMX_PTR pComponentConfigStructure)
{
	UNUSED_PARAM(hComp);
	UNUSED_PARAM(nConfigIndex);
	UNUSED_PARAM(pComponentConfigStructure);

	return OMX_ErrorNone;
}

OMX_ERRORTYPE NX_BaseSetConfig (OMX_HANDLETYPE hComp, OMX_INDEXTYPE nConfigIndex, OMX_PTR pComponentConfigStructure)
{
	UNUSED_PARAM(hComp);
	UNUSED_PARAM(nConfigIndex);
	UNUSED_PARAM(pComponentConfigStructure);

	return OMX_ErrorNone;
}

OMX_ERRORTYPE NX_BaseGetExtensionIndex(OMX_HANDLETYPE hComponent, OMX_STRING cParameterName, OMX_INDEXTYPE* pIndexType)
{
	UNUSED_PARAM(hComponent);
	/* Adnroid Extension */
	if( strcmp(cParameterName, "OMX.google.android.index.getAndroidNativeBufferUsage") == 0 )
	{
		*pIndexType = OMX_IndexAndroidNativeBufferUsage;
		return OMX_ErrorNone;
	}
	if( strcmp(cParameterName, "OMX.google.android.index.enableAndroidNativeBuffers") == 0 )
	{
		*pIndexType = OMX_IndexEnableAndroidNativeBuffers;
		return OMX_ErrorNone;
	}
	if( strcmp(cParameterName, "OMX.google.android.index.useAndroidNativeBuffer2") == 0 )
	{
		*pIndexType = OMX_IndexUseAndroidNativeBuffer2;
		return OMX_ErrorNone;
	}
	if( strcmp(cParameterName, "OMX.google.android.index.storeMetaDataInBuffers" ) == 0 )
	{
		*pIndexType = OMX_IndexStoreMetaDataInBuffers;
		return OMX_ErrorNone;
	}
	if( strcmp(cParameterName, "OMX.NX.VIDEO_DECODER.ThumbnailMode" ) == 0 )
	{
		*pIndexType = OMX_IndexVideoDecoderThumbnailMode;
		return OMX_ErrorNone;
	}
	if( strcmp(cParameterName, "OMX.NX.VIDEO_DECODER.Extradata" ) == 0 )
	{
		*pIndexType = OMX_IndexVideoDecoderExtradata;
		return OMX_ErrorNone;
	}
	if( strcmp(cParameterName, "OMX.NX.VIDEO_DECODER.CodecTag" ) == 0 )
	{
		*pIndexType = OMX_IndexVideoDecoderCodecTag;
		return OMX_ErrorNone;
	}
	if( strcmp(cParameterName, "OMX.NX.AUDIO_DECODER.FFMPEG.Extradata" ) == 0 )
	{
		*pIndexType = OMX_IndexAudioDecoderFFMpegExtradata;
		return OMX_ErrorNone;
	}
	return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE NX_BaseGetState (OMX_HANDLETYPE hComp, OMX_STATETYPE* pState)
{
	NX_BASE_COMPNENT *pBaseComp = (NX_BASE_COMPNENT *)((OMX_COMPONENTTYPE *)hComp)->pComponentPrivate;
	*pState = pBaseComp->eCurState;

	return OMX_ErrorNone;
}

OMX_ERRORTYPE NX_BaseComponentTunnelRequest(OMX_HANDLETYPE hComp, OMX_U32 nPort, OMX_HANDLETYPE hTunneledComp, OMX_U32 nTunneledPort, OMX_TUNNELSETUPTYPE* pTunnelSetup)
{
	UNUSED_PARAM(hComp);
	UNUSED_PARAM(nPort);
	UNUSED_PARAM(hTunneledComp);
	UNUSED_PARAM(nTunneledPort);
	UNUSED_PARAM(pTunnelSetup);

	return OMX_ErrorNone;
}

OMX_ERRORTYPE NX_BaseSetCallbacks(OMX_HANDLETYPE hComponent, OMX_CALLBACKTYPE* pCallbacks, OMX_PTR pAppData)
{
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *)hComponent;
	NX_BASE_COMPNENT *pBaseComp = (NX_BASE_COMPNENT *)(hComp->pComponentPrivate);
	pBaseComp->pCallbacks = pCallbacks;
	pBaseComp->pCallbackData = pAppData;

	return OMX_ErrorNone;
}

OMX_ERRORTYPE NX_BaseComponentDeInit(OMX_HANDLETYPE hComponent)
{
	UNUSED_PARAM(hComponent);

	return OMX_ErrorNone;
}

OMX_ERRORTYPE NX_BaseUseEGLImage(OMX_HANDLETYPE hComponent, OMX_BUFFERHEADERTYPE** ppBufferHdr, OMX_U32 nPortIndex, OMX_PTR pAppPrivate, void* eglImage)
{
	UNUSED_PARAM(hComponent);
	UNUSED_PARAM(ppBufferHdr);
	UNUSED_PARAM(nPortIndex);
	UNUSED_PARAM(pAppPrivate);
	UNUSED_PARAM(eglImage);

	return OMX_ErrorNone;
}

OMX_ERRORTYPE NX_BaseComponentRoleEnum(OMX_HANDLETYPE hComponent, OMX_U8 *cRole, OMX_U32 nIndex)
{
	UNUSED_PARAM(hComponent);
	UNUSED_PARAM(cRole);
	UNUSED_PARAM(nIndex);

	return OMX_ErrorNone;
}

/*
 *
 * Base Allocate Buffer for each Port
 *
 */
OMX_ERRORTYPE NX_BaseAllocateBuffer (OMX_HANDLETYPE hComponent, OMX_BUFFERHEADERTYPE** pBuffer, OMX_U32 nPortIndex, OMX_PTR pAppPrivate, OMX_U32 nSizeBytes)
{
	OMX_COMPONENTTYPE *pStdComp = (OMX_COMPONENTTYPE *)hComponent;
	NX_BASE_COMPNENT *pComp = (NX_BASE_COMPNENT *)pStdComp->pComponentPrivate;
	NX_BASEPORTTYPE *pPort = NULL;
	OMX_BUFFERHEADERTYPE **pPortBuf = NULL;
	OMX_U32 i = 0;

	if( nPortIndex >= pComp->nNumPort )
		return OMX_ErrorBadPortIndex;

	if( OMX_StateLoaded != pComp->eCurState || OMX_StateIdle != pComp->eNewState )
		return OMX_ErrorIncorrectStateTransition;

	if( 0 ==nPortIndex )
	{
		pPort = pComp->pInputPort;
		pPortBuf = pComp->pInputBuffers;
	}
	else
	{
		pPort = pComp->pOutputPort;
		pPortBuf = pComp->pOutputBuffers;
	}

	if( pPort->stdPortDef.nBufferSize > nSizeBytes )
		return OMX_ErrorBadParameter;

	for( i=0 ; i<pPort->stdPortDef.nBufferCountActual ; i++ )
	{
		if( NULL == pPortBuf[i] )
		{
			/* Allocate Actual Data */
			pPortBuf[i] = NxMalloc( sizeof(OMX_BUFFERHEADERTYPE) );
			if( NULL == pPortBuf[i] )
				return OMX_ErrorInsufficientResources;
			NxMemset( pPortBuf[i], 0, sizeof(OMX_BUFFERHEADERTYPE) );
			pPortBuf[i]->nSize = sizeof(OMX_BUFFERHEADERTYPE);
			NX_OMXSetVersion( &pPortBuf[i]->nVersion );
			pPortBuf[i]->pBuffer = NxMalloc( nSizeBytes );
			if( NULL == pPortBuf[i]->pBuffer )
				return OMX_ErrorInsufficientResources;
			pPortBuf[i]->nAllocLen = nSizeBytes;
			pPortBuf[i]->pAppPrivate = pAppPrivate;
			pPortBuf[i]->pPlatformPrivate = pStdComp;
			if( OMX_DirInput == pPort->stdPortDef.eDir )
			{
				pPortBuf[i]->nInputPortIndex = pPort->stdPortDef.nPortIndex;
			}
			else
			{
				pPortBuf[i]->nOutputPortIndex = pPort->stdPortDef.nPortIndex;
			}
			pPort->nAllocatedBuf ++;
			if( pPort->nAllocatedBuf == pPort->stdPortDef.nBufferCountActual )
			{
				pPort->stdPortDef.bPopulated = OMX_TRUE;
				pPort->eBufferType = NX_BUFHEADER_TYPE_ALLOCATED;
				NX_PostSem(pComp->hBufAllocSem);
			}
			*pBuffer = pPortBuf[i];
			return OMX_ErrorNone;
		}
	}
	pPort->stdPortDef.bPopulated = OMX_TRUE;
	*pBuffer = pPortBuf[0];

	return OMX_ErrorInsufficientResources;
}

/*
 *
 * Base Free Buffer for each Port
 *
 */
OMX_ERRORTYPE NX_BaseFreeBuffer (OMX_HANDLETYPE hComponent, OMX_U32 nPortIndex, OMX_BUFFERHEADERTYPE* pBuffer)
{
	OMX_COMPONENTTYPE *pStdComp = (OMX_COMPONENTTYPE *)hComponent;
	NX_BASE_COMPNENT *pComp = (NX_BASE_COMPNENT *)pStdComp->pComponentPrivate;
	NX_BASEPORTTYPE *pPort = NULL;
	OMX_BUFFERHEADERTYPE **pPortBuf = NULL;
	OMX_U32 i=0;

	if( nPortIndex >= pComp->nNumPort )
		return OMX_ErrorBadPortIndex;

	if( OMX_StateLoaded != pComp->eNewState )
		return OMX_ErrorIncorrectStateTransition;

	if( 0 == nPortIndex )
	{
		pPort = pComp->pInputPort;
		pPortBuf = pComp->pInputBuffers;
	}
	else
	{
		pPort = pComp->pOutputPort;
		pPortBuf = pComp->pOutputBuffers;
	}

	if( NULL == pBuffer )
	{
		return OMX_ErrorBadParameter;
	}

	for( i=0 ; i<pPort->stdPortDef.nBufferCountActual ; i++ )
	{
		if( NULL != pBuffer && pBuffer == pPortBuf[i] )
		{
			if( pPort->eBufferType == NX_BUFHEADER_TYPE_ALLOCATED )
			{
				NxFree(pPortBuf[i]->pBuffer);
				pPortBuf[i]->pBuffer = NULL;
			}
			NxFree(pPortBuf[i]);
			pPortBuf[i] = NULL;
			pPort->nAllocatedBuf --;
			if( 0 == pPort->nAllocatedBuf )
			{
				pPort->stdPortDef.bPopulated = OMX_FALSE;
				NX_PostSem(pComp->hBufAllocSem);
			}
			return OMX_ErrorNone;
		}
	}
	return OMX_ErrorInsufficientResources;
}

OMX_ERRORTYPE NX_BaseEmptyThisBuffer (OMX_HANDLETYPE hComp, OMX_BUFFERHEADERTYPE* pBuffer)
{
	NX_BASE_COMPNENT *pComp = (NX_BASE_COMPNENT *)((OMX_COMPONENTTYPE *)hComp)->pComponentPrivate;
	/* Push data to command buffer */
	// assert( NULL != pComp->pInputPort );
	// assert( NULL != pComp->pInputPortQueue );

	if( pComp->eCurState == pComp->eNewState )
	{
		if( !(OMX_StateIdle == pComp->eCurState || OMX_StatePause == pComp->eCurState || OMX_StateExecuting == pComp->eCurState) )
			return OMX_ErrorIncorrectStateOperation;
	}
	else
	{
		if( (OMX_StateIdle==pComp->eNewState) && (OMX_StateExecuting==pComp->eCurState || OMX_StatePause==pComp->eCurState) )
			return OMX_ErrorIncorrectStateOperation;
	}

	if( 0 != NX_PushQueue( pComp->pInputPortQueue, pBuffer ) )
	{
		return OMX_ErrorUndefined;
	}
	NX_PostSem( pComp->hBufChangeSem );

	return OMX_ErrorNone;
}

OMX_ERRORTYPE NX_BaseFillThisBuffer(OMX_HANDLETYPE hComp, OMX_BUFFERHEADERTYPE* pBuffer)
{
	NX_BASE_COMPNENT *pComp = (NX_BASE_COMPNENT *)((OMX_COMPONENTTYPE *)hComp)->pComponentPrivate;
	/* Push data to command buffer */
	// assert( NULL != pComp->pInputPort );
	// assert( NULL != pComp->pInputPortQueue );
	if( pComp->eCurState == pComp->eNewState )
	{
		if( !(OMX_StateIdle == pComp->eCurState || OMX_StatePause == pComp->eCurState || OMX_StateExecuting == pComp->eCurState) )
			return OMX_ErrorIncorrectStateOperation;
	}
	else
	{
		if( (OMX_StateIdle==pComp->eNewState) && (OMX_StateExecuting==pComp->eCurState || OMX_StatePause==pComp->eCurState) )
			return OMX_ErrorIncorrectStateOperation;
	}

	if( 0 != NX_PushQueue( pComp->pOutputPortQueue, pBuffer ) )
	{
		return OMX_ErrorUndefined;
	}
	NX_PostSem( pComp->hBufChangeSem );

	return OMX_ErrorNone;
}

/*
 *
 * Base Command Thread Procdure
 *
 */
void NX_BaseCommandThread( void *arg )
{
	NX_BASE_COMPNENT *pComp = (NX_BASE_COMPNENT *)arg;
	NX_CMD_MSG_TYPE *pCmd = NULL;
	NX_DbgTrace( "enter %s() loop\n", __FUNCTION__ );

	NX_PostSem( pComp->hSemCmdWait );
	while(1)
	{
		if( 0 != NX_PendSem( pComp->hSemCmd ) )
		{
			DbgMsg( "%s() : Exit command loop : error NX_PendSem()\n", __FUNCTION__ );
			break;
		}

		if( NX_THREAD_CMD_RUN!=pComp->eCmdThreadCmd && 0==NX_GetQueueCnt(&pComp->cmdQueue) )
		{
			break;
		}

		if( 0 != NX_PopQueue( &pComp->cmdQueue, (void**)&pCmd ) )
		{
			DbgMsg( "%s() : Exit command loop : NX_PopQueue()\n", __FUNCTION__ );
			break;
		}

		NX_DbgTrace("%s() : pCmd = %p\n", __FUNCTION__, pCmd);

		if( pCmd != NULL )
		{
			if( pComp->cbCmdProcedure )
				pComp->cbCmdProcedure( pComp, pCmd->eCmd, pCmd->nParam1, pCmd->pCmdData );
			NxFree( pCmd );
		}
	}
	NX_DbgTrace( ">> exit %s() loop!!\n", __FUNCTION__ );
}

/*
 *
 * Base Component Tools
 *
 */
static const char *EventCodeToString( OMX_EVENTTYPE eEvent )
{
	switch(eEvent)
	{
		case OMX_EventCmdComplete:              return "OMX_EventCmdComplete";
		case OMX_EventError:                    return "OMX_EventError";
		case OMX_EventMark:						return "OMX_EventMark";
		case OMX_EventPortSettingsChanged:		return "OMX_EventPortSettingsChanged";
		case OMX_EventBufferFlag:				return "OMX_EventBufferFlag";
		case OMX_EventResourcesAcquired:		return "OMX_EventResourcesAcquired";
		case OMX_EventComponentResumed:			return "OMX_EventComponentResumed";
		case OMX_EventDynamicResourcesAvailable:return "OMX_EventDynamicResourcesAvailable";
		case OMX_EventPortFormatDetected:		return "OMX_EventPortFormatDetected";
		default:								return "Unknown";
	}
}

int SendEvent( NX_BASE_COMPNENT *pBaseComp, OMX_EVENTTYPE eEvent, OMX_U32 param1, OMX_U32 param2, OMX_PTR pEventData )
{
	NX_DbgTrace("Event : %s, param1=%ld, param2=%ld\n", EventCodeToString(eEvent), param1, param2);
	(*(pBaseComp->pCallbacks->EventHandler)) (pBaseComp->hComp, pBaseComp->pCallbackData, eEvent, param1, param2, pEventData);

	return 0;
}
