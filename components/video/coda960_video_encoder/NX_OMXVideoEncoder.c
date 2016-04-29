#define	LOG_TAG				"NX_OMXENC"

#include <assert.h>

#include <sys/mman.h>		/* mmap, munmap */

/* #include <OMX_AndroidTypes.h> */

#include <nx_fourcc.h>
#include <nx_video_alloc.h>

#include <stdbool.h>

#include <NX_OMXVideoEncoder.h>

#ifdef ANDROID
#include <ion/ion.h>
#include <hardware/gralloc.h>
#include <cutils/native_handle.h>
#include <gralloc_priv.h>
/* psw0523 add for new gralloc */
#include <ion-private.h>
#include <nexell_format.h>
#include <csc.h>		/* NEON Color Space Converter */
#endif

#define	DEBUG_BUFFER	0
#define	TRACE_ON		0
#define	DEBUG_FUNC		0

#ifdef ANDROID
#define DbgMsg(fmt,...)		ALOGD(fmt,##__VA_ARGS__)
#define ErrMsg(fmt,...)		ALOGE(fmt,##__VA_ARGS__)
#else

#endif

#if	TRACE_ON
#define	TRACE(fmt,...)		DbgMsg(fmt, ##__VA_ARGS__)
#else
#define	TRACE(fmt,...)		do{}while(0)
#endif

#if	DEBUG_FUNC
#define	FUNC_IN				DbgMsg("%s() In\n", __FUNCTION__)
#define	FUNC_OUT			DbgMsg("%s() OUT\n", __FUNCTION__)
#else
#define	FUNC_IN				do{}while(0)
#define	FUNC_OUT			do{}while(0)
#endif

#if DEBUG_BUFFER
#define	DbgBuffer(fmt,...)	LOGD(fmt,##__VA_ARGS__)
#else
#define DbgBuffer(fmt,...)	 do{}while(0)
#endif

#ifndef UNUSED_PARAM
#define	UNUSED_PARAM(X)		X=X
#endif

#ifndef ALIGN
#define	ALIGN(X,N)	( ((X)+N-1) & (~(N-1)) )
#endif

/* Default Recomanded Functions for Implementation Components */
static OMX_ERRORTYPE NX_VidEncGetParameter			( OMX_HANDLETYPE hComp, OMX_INDEXTYPE nParamIndex,OMX_PTR ComponentParamStruct);
static OMX_ERRORTYPE NX_VidEncSetParameter			( OMX_HANDLETYPE hComp, OMX_INDEXTYPE nParamIndex, OMX_PTR ComponentParamStruct);
static OMX_ERRORTYPE NX_VidEncUseBuffer				( OMX_HANDLETYPE hComp, OMX_BUFFERHEADERTYPE** ppBufferHdr, OMX_U32 nPortIndex, OMX_PTR pAppPrivate, OMX_U32 nSizeBytes, OMX_U8* pBuffer);
static OMX_ERRORTYPE NX_VidEncComponentDeInit		( OMX_HANDLETYPE hComp);
static void NX_VidEncBufferMgmtThread( void *arg );
static void NX_VidEncCommandProc( NX_BASE_COMPNENT *pBaseComp, OMX_COMMANDTYPE Cmd, OMX_U32 nParam1, OMX_PTR pCmdData );

static OMX_S32		gstNumInstance = 0;
static OMX_S32		gstMaxInstance = 2;

static OMX_S32 EncoderOpen(NX_VIDENC_COMP_TYPE *pEncComp);
static OMX_S32 EncoderClose(NX_VIDENC_COMP_TYPE *pEncComp);
static OMX_S32 EncodeFrame(NX_VIDENC_COMP_TYPE *pEncComp, NX_QUEUE *pInQueue, NX_QUEUE *pOutQueue);

static void SetDefaultMp4EncParam( OMX_VIDEO_PARAM_MPEG4TYPE *mp4Type, int portIndex )
{
	mp4Type->nSize = sizeof(OMX_VIDEO_PARAM_MPEG4TYPE);
	NX_OMXSetVersion( &mp4Type->nVersion );
	mp4Type->nPortIndex = portIndex;
	mp4Type->nSliceHeaderSpacing = 0;						/* nSliceHeaderSpacing is the number of macroblocks in a slice (H263+ Annex K). This value shall be zero if not used. */
	mp4Type->bSVH = OMX_FALSE;								/* bSVH is a Boolean value that enables or disables short header mode. */
	mp4Type->bGov = OMX_TRUE;								/* bGov is a Boolean value that enables or disables group of VOP (GOV), where VOP is the abbreviation for video object planes. */
	mp4Type->nPFrames = 29;									/* nPFrames is the number of P frames between I frames. */
	mp4Type->nBFrames = 0;									/* nPFrames is the number of B frames between I frames. */
	mp4Type->nIDCVLCThreshold = 0;							/* nIDCVLCThreshold is the value of the intra-DC variable-length coding (VLC) threshold. */
	mp4Type->bACPred = OMX_FALSE;							/* bACPred is the Boolean value that enables or disables AC prediction. */
	mp4Type->nMaxPacketSize = 256 * 1024;					/* nMaxPacketSize is the maximum size of the packet in bytes. */
	mp4Type->nTimeIncRes = 1000;							/* nTimeIncRes is the VOP time increment resolution for MPEG-4. This value is interpreted as described in the MPEG-4 standard. */
	mp4Type->eProfile = OMX_VIDEO_MPEG4ProfileSimple;		/* eProfile is the profile used for MPEG-4 encoding or decoding. */
	mp4Type->eLevel = OMX_VIDEO_MPEG4Level2;				/* eLevel is the maximum processing level that an encoder or decoder supports for a particular MPEG-4 profile. */
	mp4Type->nAllowedPictureTypes = OMX_VIDEO_PictureTypeP;	/* nAllowedPictureTypes identifies the picture types allowed in the bit stream.
															   For more information on picture types, see Table 4-44: Supported Video Picture Types. */
	mp4Type->nHeaderExtension = 0;							/* nHeaderExtension specifies the number of consecutive video packets between header extension codes. */
	mp4Type->bReversibleVLC = OMX_FALSE;					/* bReversibleVLC is a Boolean value that enables or disables the use of reversible variable-length coding. */
}

static void SetDefaultH263EncParam( OMX_VIDEO_PARAM_H263TYPE *h263Type, int portIndex )
{
	h263Type->nSize = sizeof(OMX_VIDEO_PARAM_H263TYPE);
	NX_OMXSetVersion( &h263Type->nVersion );
	h263Type->nPortIndex = portIndex;
	h263Type->nPFrames = 29;								/* nPFrames is the number of P frames between each I frame. */
	h263Type->nBFrames = 0;									/* nBFrames is the number of B frames between each I frame */
	h263Type->eProfile = OMX_VIDEO_H263ProfileH320Coding;	/* eProfile is the profile used for H.263 encoding or decoding. */
	h263Type->eLevel = OMX_VIDEO_H263Level10;				/* eLevel is the maximum processing level that an encoder or decoder supports for a H.263 */
	h263Type->bPLUSPTYPEAllowed = OMX_FALSE;				/* bPLUSPTYPEAllowed is Indicating that it is allowed to use PLUSPTYPE (specified in the 1998 version of H.263) to indicate custom picture sizes or clock frequencies */
	h263Type->nAllowedPictureTypes = OMX_VIDEO_PictureTypeP;/* nAllowedPictureTypes sepcifies the picture types allowed in the bitstream. */
	h263Type->bForceRoundingTypeToZero = OMX_FALSE;			/* bForceRoundingTypeToZero is the value of the RTYPE bit (bit 6 of MPPTYPE) is not constrained.
															   It is recommended to change the value of the RTYPE bit for each reference picture in error-free communication. */
	h263Type->nPictureHeaderRepetition = OMX_TRUE;			/* nPictureHeaderRepetition specifies the frequency of picture header repetition. */
	h263Type->nGOBHeaderInterval = OMX_TRUE;				/* nGOBHeaderInterval sepcifies the interval of non-empty GOB headers in units of GOBs */
}

static void SetDefaultAvcEncParam( OMX_VIDEO_PARAM_AVCTYPE *avcType, int portIndex )
{
	avcType->nSize = sizeof(OMX_VIDEO_PARAM_AVCTYPE);
	NX_OMXSetVersion(&avcType->nVersion);
	avcType->nPortIndex = portIndex;
	avcType->nSliceHeaderSpacing = 0;							/* Number of macroblocks between slice header, put zero if not used */
	avcType->nPFrames = 29;										/* Number of P frames between each I frame */
	avcType->nBFrames = 0;										/* Number of B frames between each I frame */
	avcType->bUseHadamard = OMX_FALSE;							/* Enable/disable Hadamard transform */
	avcType->nRefFrames = 1;									/* Max number of reference frames to use for inter motion search (1-16) */
	avcType->nRefIdx10ActiveMinus1 = 0;							/* Pic param set ref frame index (index into ref frame buffer of trailing frames list), B frame support */
	avcType->nRefIdx11ActiveMinus1 = 0;							/* Pic param set ref frame index (index into ref frame buffer of forward frames list), B frame support */
	avcType->bEnableUEP = OMX_FALSE;							/* Enable/disable unequal error protection. This is only valid of data partitioning is enabled. */
	avcType->bEnableFMO = OMX_FALSE;							/* Enable/disable flexible macroblock ordering */
	avcType->bEnableASO = OMX_FALSE;							/* Enable/disable arbitrary slice ordering */
	avcType->bEnableRS = OMX_FALSE;								/* Enable/disable sending of redundant slices */
	avcType->eProfile = OMX_VIDEO_AVCProfileBaseline;			/* AVC profile(s) to use */
	avcType->eLevel = OMX_VIDEO_AVCLevel41;						/* AVC level(s) to use */
	avcType->nAllowedPictureTypes = OMX_VIDEO_PictureTypeP;		/* Specifies the picture types allowed in the bitstream */
	avcType->bFrameMBsOnly = OMX_FALSE;							/* specifies that every coded picture of the coded video sequence is a coded frame containing only frame macroblocks */
	avcType->bMBAFF = OMX_FALSE;								/* Enable/disable switching between frame and field macroblocks within a picture */
	avcType->bEntropyCodingCABAC = OMX_FALSE;					/* Entropy decoding method to be applied for the syntax elements for which two descriptors appear in the syntax tables */
	avcType->bWeightedPPrediction = OMX_FALSE;					/* Enable/disable weighted prediction shall not be applied to P and SP slices */
	avcType->nWeightedBipredicitonMode = 0;						/* Default weighted prediction is applied to B slices */
	avcType->bconstIpred = OMX_TRUE;							/* Enable/disable intra prediction */
	avcType->bDirect8x8Inference = OMX_FALSE;					/* Specifies the method used in the derivation process for luma motion vectors for B_Skip, B_Direct_16x16 and B_Direct_8x8 as specified in subclause 8.4.1.2 of the AVC spec */
	avcType->bDirectSpatialTemporal = OMX_FALSE;				/* Flag indicating spatial or temporal direct mode used in B slice coding (related to bDirect8x8Inference) . Spatial direct mode is more common and should be the default. */
	avcType->nCabacInitIdc = 0;									/* Index used to init CABAC contexts */
	avcType->eLoopFilterMode = OMX_VIDEO_AVCLoopFilterEnable;	/* Enable/disable loop filter */
}

OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComponent)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_COMPONENTTYPE *pComp = (OMX_COMPONENTTYPE *)hComponent;
	NX_BASEPORTTYPE *pPort = NULL;
	OMX_U32 i=0;
	NX_VIDENC_COMP_TYPE *pEncComp;

	if( gstNumInstance >= gstMaxInstance )
	{
		return OMX_ErrorInsufficientResources;
	}

	pEncComp = (NX_VIDENC_COMP_TYPE *)NxMalloc( sizeof(NX_VIDENC_COMP_TYPE) );
	if( pEncComp == NULL )
	{
		return OMX_ErrorInsufficientResources;
	}

	/* Component configuration */
	NxMemset( pEncComp, 0, sizeof(NX_VIDENC_COMP_TYPE) );
	pComp->pComponentPrivate = pEncComp;

	/* Initialize Base Component Informations */
	if( OMX_ErrorNone != (eError=NX_BaseComponentInit( pComp )) )
	{
		NxFree( pEncComp );
		return eError;
	}
	NX_OMXSetVersion( &pComp->nVersion );


	/* Port Configuration */
	/*	Create ports */
	for( i = 0; i < VIDDEC_NUM_PORT; i++ )
	{
		pEncComp->pPort[i] = (OMX_PTR)NxMalloc( sizeof(NX_BASEPORTTYPE) );
		NxMemset( pEncComp->pPort[i], 0, sizeof(NX_BASEPORTTYPE) );
		pEncComp->pBufQueue[i] = (OMX_PTR)NxMalloc( sizeof(NX_QUEUE) );
	}
	pEncComp->nNumPort = VIDDEC_NUM_PORT;
	/* Input port configuration */
	pEncComp->pInputPort = (NX_BASEPORTTYPE *)pEncComp->pPort[VIDDEC_INPORT_INDEX];
	pEncComp->pInputPortQueue = (NX_QUEUE *)pEncComp->pBufQueue[VIDDEC_INPORT_INDEX];
	NX_InitQueue( pEncComp->pInputPortQueue, NX_OMX_MAX_BUF );
	pPort = pEncComp->pInputPort;
	NX_OMXSetVersion( &pPort->stdPortDef.nVersion );
	NX_InitOMXPort( &pPort->stdPortDef, VIDDEC_INPORT_INDEX, OMX_DirInput, OMX_TRUE, OMX_PortDomainVideo );
	pPort->stdPortDef.nBufferCountActual = VIDENC_INPORT_MIN_BUF_CNT;
	pPort->stdPortDef.nBufferCountMin    = VIDENC_INPORT_MIN_BUF_CNT;
	pPort->stdPortDef.nBufferSize        = VIDENC_INPORT_MIN_BUF_SIZE;

	/* Output port configuration */
	pEncComp->pOutputPort = (NX_BASEPORTTYPE *)pEncComp->pPort[VIDDEC_OUTPORT_INDEX];
	pEncComp->pOutputPortQueue = (NX_QUEUE *)pEncComp->pBufQueue[VIDDEC_OUTPORT_INDEX];
	NX_InitQueue( pEncComp->pOutputPortQueue, NX_OMX_MAX_BUF );
	pPort = pEncComp->pOutputPort;
	NX_OMXSetVersion( &pPort->stdPortDef.nVersion );
	NX_InitOMXPort( &pPort->stdPortDef, VIDDEC_OUTPORT_INDEX, OMX_DirOutput, OMX_TRUE, OMX_PortDomainVideo );
	pPort->stdPortDef.nBufferCountActual = VIDENC_OUTPORT_MIN_BUF_CNT;
	pPort->stdPortDef.nBufferCountMin    = VIDENC_OUTPORT_MIN_BUF_CNT;
	pPort->stdPortDef.nBufferSize        = VIDENC_OUTPORT_MIN_BUF_SIZE;


	/* Registration OMX Standard Functions */
	/* Specific implemented functions */
	pComp->GetParameter           = NX_VidEncGetParameter;
	pComp->SetParameter           = NX_VidEncSetParameter;
	pComp->UseBuffer              = NX_VidEncUseBuffer;
	pComp->ComponentDeInit        = NX_VidEncComponentDeInit;

	/* Registration Command Procedure */
	pEncComp->cbCmdProcedure = NX_VidEncCommandProc;	/* Command procedure */

	/* Create command thread */
	NX_InitQueue( &pEncComp->cmdQueue, NX_MAX_QUEUE_ELEMENT );
	pEncComp->hSemCmd = NX_CreateSem( 0, NX_MAX_QUEUE_ELEMENT );
	pEncComp->hSemCmdWait = NX_CreateSem( 0, 1 );
	pEncComp->eCmdThreadCmd = NX_THREAD_CMD_RUN;
	pthread_create( &pEncComp->hCmdThread, NULL, (void*)&NX_BaseCommandThread, pEncComp );
	NX_PendSem( pEncComp->hSemCmdWait );

	/* Set Component Name & Role */
	pEncComp->compName = strdup("OMX.NX.VIDEO_ENCODER");
	pEncComp->compRole = strdup("video_encoder");

	/* Buffer */
	pthread_mutex_init( &pEncComp->hBufMutex, NULL );
	pEncComp->hBufAllocSem = NX_CreateSem( 0, 16 );
	gstNumInstance++;

	pEncComp->bUseNativeBuffer = OMX_FALSE;
	pEncComp->bMetaDataInBuffers = OMX_FALSE;

	/* Default Encoder Paramter */
	pEncComp->bCodecSpecificInfo= OMX_FALSE;
	pEncComp->hVpuCodec			= NULL;
	pEncComp->encWidth			= 1024;
	pEncComp->encHeight			= 768;
	pEncComp->encKeyInterval	= VIDENC_DEF_FRAMERATE;
	pEncComp->encFrameRate		= VIDENC_DEF_FRAMERATE;
	pEncComp->encBitRate		= VIDENC_DEF_BITRATE;

	pEncComp->bSendCodecSpecificInfo = OMX_FALSE;

	pEncComp->hCSCMem			= NULL;		/* CSC Temporal Memory */

	pEncComp->encIntraRefreshMbs= 0;

	SetDefaultMp4EncParam( &pEncComp->omxMp4EncParam, 1 );
	SetDefaultAvcEncParam( &pEncComp->omxAVCEncParam, 1 );
	SetDefaultH263EncParam( &pEncComp->omxH263EncParam, 1 );

	pEncComp->pPrevInputBuffer = NULL;

	return OMX_ErrorNone;
}

static OMX_ERRORTYPE NX_VidEncComponentDeInit(OMX_HANDLETYPE hComponent)
{
	OMX_COMPONENTTYPE *pComp = (OMX_COMPONENTTYPE *)hComponent;
	NX_VIDENC_COMP_TYPE *pEncComp = (NX_VIDENC_COMP_TYPE *)pComp->pComponentPrivate;
	OMX_U32 i=0;

	/* prevent duplacation */
	if( NULL == pComp->pComponentPrivate )
	{
		return OMX_ErrorNone;
	}

	/* Destroy command thread */
	pEncComp->eCmdThreadCmd = NX_THREAD_CMD_EXIT;
	NX_PostSem( pEncComp->hSemCmdWait );
	NX_PostSem( pEncComp->hSemCmd );
	pthread_join( pEncComp->hCmdThread, NULL );
	NX_DeinitQueue( &pEncComp->cmdQueue );
	/* Destroy Semaphore */
	NX_DestroySem( pEncComp->hSemCmdWait );
	NX_DestroySem( pEncComp->hSemCmd );

	/* Destroy port resource */
	for( i=0; i<VIDDEC_NUM_PORT ; i++ )
	{
		if( pEncComp->pPort[i] )
		{
			NxFree(pEncComp->pPort[i]);
		}

		if( pEncComp->pBufQueue[i] )
		{
			/* Deinit Queue */
			NX_DeinitQueue( pEncComp->pBufQueue[i] );
			NxFree( pEncComp->pBufQueue[i] );
		}
	}

	NX_BaseComponentDeInit( hComponent );

	/* Buffer */
	pthread_mutex_destroy( &pEncComp->hBufMutex );
	NX_DestroySem(pEncComp->hBufAllocSem);

	if( pEncComp->compName )
	{
		free(pEncComp->compName);
	}

	if( pEncComp->compRole )
	{
		free(pEncComp->compRole);
	}

	if( pEncComp->hCSCMem )
	{
		NX_FreeVideoMemory( pEncComp->hCSCMem );
	}

	if( pEncComp )
	{
		NxFree( pEncComp );
		pComp->pComponentPrivate = NULL;
	}

	gstNumInstance --;

	return OMX_ErrorNone;
}

#define	MAX_SUPPORTED_AVC_LEVELS	14
const OMX_VIDEO_AVCLEVELTYPE gstSuportedAVCLevels[MAX_SUPPORTED_AVC_LEVELS] =
{
	OMX_VIDEO_AVCLevel1 ,	/**< Level 1 */
	OMX_VIDEO_AVCLevel1b,	/**< Level 1b */
	OMX_VIDEO_AVCLevel11,	/**< Level 1.1 */
	OMX_VIDEO_AVCLevel12,	/**< Level 1.2 */
	OMX_VIDEO_AVCLevel13,	/**< Level 1.3 */
	OMX_VIDEO_AVCLevel2 ,	/**< Level 2 */
	OMX_VIDEO_AVCLevel21,	/**< Level 2.1 */
	OMX_VIDEO_AVCLevel22,	/**< Level 2.2 */
	OMX_VIDEO_AVCLevel3 ,	/**< Level 3 */
	OMX_VIDEO_AVCLevel31,	/**< Level 3.1 */
	OMX_VIDEO_AVCLevel32,	/**< Level 3.2 */
	OMX_VIDEO_AVCLevel4 ,	/**< Level 4 */
	OMX_VIDEO_AVCLevel41,	/**< Level 4.1 */
	OMX_VIDEO_AVCLevel42,	/**< Level 4.2 */
};

#define MAX_SUPPORTED_MP4_LEVELS	8
const OMX_VIDEO_MPEG4LEVELTYPE gstSuportedMP4Levels[MAX_SUPPORTED_MP4_LEVELS] =
{
	OMX_VIDEO_MPEG4Level0 ,   /**< Level 0 */
	OMX_VIDEO_MPEG4Level0b,   /**< Level 0b */
	OMX_VIDEO_MPEG4Level1 ,   /**< Level 1 */
	OMX_VIDEO_MPEG4Level2 ,   /**< Level 2 */
	OMX_VIDEO_MPEG4Level3 ,   /**< Level 3 */
	OMX_VIDEO_MPEG4Level4 ,   /**< Level 4 */
	OMX_VIDEO_MPEG4Level4a,   /**< Level 4a */
	OMX_VIDEO_MPEG4Level5 ,   /**< Level 5 */
};

#define MAX_SUPPORTED_H263_LEVELS	8
const OMX_VIDEO_H263LEVELTYPE gstSuportedH263Levels[MAX_SUPPORTED_H263_LEVELS] =
{
	OMX_VIDEO_H263Level10,
	OMX_VIDEO_H263Level20,
	OMX_VIDEO_H263Level30,
	OMX_VIDEO_H263Level40,
	OMX_VIDEO_H263Level45,
	OMX_VIDEO_H263Level50,
	OMX_VIDEO_H263Level60,
	OMX_VIDEO_H263Level70,
};

static OMX_ERRORTYPE NX_VidEncGetParameter (OMX_HANDLETYPE hComp, OMX_INDEXTYPE nParamIndex,OMX_PTR ComponentParamStruct)
{
	OMX_COMPONENTTYPE *pStdComp = (OMX_COMPONENTTYPE *)hComp;
	NX_VIDENC_COMP_TYPE *pEncComp = (NX_VIDENC_COMP_TYPE *)pStdComp->pComponentPrivate;
	TRACE("%s(): In, (nParamIndex=0x%08x)\n", __FUNCTION__, nParamIndex );
	switch( nParamIndex )
	{
		case OMX_IndexParamVideoPortFormat:
		{
			OMX_VIDEO_PARAM_PORTFORMATTYPE *pVideoFormat = (OMX_VIDEO_PARAM_PORTFORMATTYPE *)ComponentParamStruct;
			OMX_U32 nIndex = pVideoFormat->nIndex;
			TRACE("%s() : OMX_IndexParamVideoPortFormat : port Index = %ld, format index = %ld\n", __FUNCTION__, pVideoFormat->nPortIndex, pVideoFormat->nIndex );
			/* Input Information */
			if( pVideoFormat->nPortIndex == 0 )
			{
				switch( nIndex )
				{
					case 0:
						memcpy( pVideoFormat, &pEncComp->inputFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE) );
						pVideoFormat->eColorFormat = OMX_COLOR_FormatYUV420Planar;
						break;
					case 1:
						memcpy( pVideoFormat, &pEncComp->inputFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE) );
						pVideoFormat->eColorFormat = OMX_COLOR_FormatAndroidOpaque;
						break;
					default:
						return OMX_ErrorNoMore;
				}
				pVideoFormat->nIndex = nIndex;
			}
			/* Output Format */
			else
			{
				if( nIndex > 0 )
					return OMX_ErrorNoMore;
				memcpy( pVideoFormat, &pEncComp->outputFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE) );
				pVideoFormat->nIndex = nIndex;
			}
			break;
		}
		case OMX_IndexParamVideoMpeg4:
		{
			OMX_VIDEO_PARAM_MPEG4TYPE *pMp4Type = (OMX_VIDEO_PARAM_MPEG4TYPE *)ComponentParamStruct;
			TRACE("%s() : OMX_IndexParamVideoMpeg4 : port Index = %ld\n", __FUNCTION__, pMp4Type->nPortIndex );
			if( pMp4Type->nPortIndex != 1 )
			{
				return OMX_ErrorBadPortIndex;
			}
			memcpy( pMp4Type, &pEncComp->omxMp4EncParam, sizeof(OMX_VIDEO_PARAM_MPEG4TYPE) );
			break;
		}
		case OMX_IndexParamVideoAvc:
		{
			OMX_VIDEO_PARAM_AVCTYPE *pAvcType = (OMX_VIDEO_PARAM_AVCTYPE *)ComponentParamStruct;
			TRACE("%s() : OMX_IndexParamVideoAvc : port Index = %ld\n", __FUNCTION__, pAvcType->nPortIndex );
			if( pAvcType->nPortIndex != 1 )
			{
				return OMX_ErrorBadPortIndex;
			}
			memcpy( pAvcType, &pEncComp->omxAVCEncParam, sizeof(OMX_VIDEO_PARAM_AVCTYPE) );
			pAvcType->nPortIndex = 1;
			break;
		}
		case OMX_IndexParamVideoH263:
		{
			OMX_VIDEO_PARAM_H263TYPE *pH263Type = (OMX_VIDEO_PARAM_H263TYPE *)ComponentParamStruct;
			TRACE("%s() : OMX_VIDEO_PARAM_H263TYPE : port Index = %ld\n", __FUNCTION__, pH263Type->nPortIndex );
			if( pH263Type->nPortIndex != 1 )
			{
				return OMX_ErrorBadPortIndex;
			}
			memcpy( pH263Type, &pEncComp->omxH263EncParam, sizeof(OMX_VIDEO_PARAM_H263TYPE) );
			break;
		}
		case OMX_IndexParamVideoProfileLevelQuerySupported:
		{
			OMX_VIDEO_PARAM_PROFILELEVELTYPE *profileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)ComponentParamStruct;
			if( profileLevel->nPortIndex > 1 )
			{
				return OMX_ErrorBadPortIndex;
			}

			if ( pEncComp->vpuCodecId == NX_AVC_ENC )
			{
				if( profileLevel->nProfileIndex >= MAX_SUPPORTED_AVC_LEVELS )
				{
					return OMX_ErrorNoMore;
				}
				profileLevel->eProfile = OMX_VIDEO_AVCProfileBaseline;
				profileLevel->eLevel = gstSuportedAVCLevels[profileLevel->nProfileIndex];
			}
			else if ( pEncComp->vpuCodecId == NX_MP4_ENC )
			{
				if ( profileLevel->nProfileIndex >= MAX_SUPPORTED_MP4_LEVELS )
				{
					return OMX_ErrorNoMore;
				}
				profileLevel->eProfile = OMX_VIDEO_MPEG4ProfileSimple;
				profileLevel->eLevel = gstSuportedMP4Levels[profileLevel->nProfileIndex];
			}
			else if ( pEncComp->vpuCodecId == NX_H263_ENC )
			{
				if ( profileLevel->nProfileIndex >= MAX_SUPPORTED_H263_LEVELS )
				{
					return OMX_ErrorNoMore;
				}
				profileLevel->eProfile = OMX_VIDEO_H263ProfileH320Coding;
				profileLevel->eLevel = gstSuportedH263Levels[profileLevel->nProfileIndex];
			}
			else
			{
				return OMX_ErrorNoMore;
			}
			break;
		}
		case OMX_IndexParamVideoBitrate:
		{
			OMX_VIDEO_PARAM_BITRATETYPE *bitRate = (OMX_VIDEO_PARAM_BITRATETYPE *)ComponentParamStruct;
			bitRate->eControlRate = OMX_Video_ControlRateVariable;		//	VBR
			bitRate->nTargetBitrate = pEncComp->encBitRate;
			bitRate->nPortIndex = 1;
			break;
		}
		case OMX_IndexParamVideoErrorCorrection:
		{
			OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *errCorrection = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)ComponentParamStruct;
			errCorrection->nPortIndex = 1;
			errCorrection->bEnableHEC = OMX_FALSE;
			errCorrection->bEnableResync = OMX_FALSE;
			errCorrection->nResynchMarkerSpacing = 0;
			errCorrection->bEnableDataPartitioning = OMX_FALSE;
			errCorrection->bEnableRVLC = OMX_FALSE;
			break;
		}
		default :
			return NX_BaseGetParameter( hComp, nParamIndex, ComponentParamStruct );
	}
	return OMX_ErrorNone;
}

static OMX_ERRORTYPE NX_VidEncSetParameter (OMX_HANDLETYPE hComp, OMX_INDEXTYPE nParamIndex, OMX_PTR ComponentParamStruct)
{
	NX_VIDENC_COMP_TYPE *pEncComp = (NX_VIDENC_COMP_TYPE *)((OMX_COMPONENTTYPE *)hComp)->pComponentPrivate;
	TRACE("%s(): In, (nParamIndex=0x%08x)\n", __FUNCTION__, nParamIndex );
	switch( nParamIndex )
	{
		/* Just check currnet role. */
		case OMX_IndexParamStandardComponentRole:
		{
			OMX_PARAM_COMPONENTROLETYPE *pInRole = (OMX_PARAM_COMPONENTROLETYPE *)ComponentParamStruct;
			if( !strcmp( (OMX_STRING)pInRole->cRole, "video_encoder.avc"  )  )
			{
				/* Set Input Format */
				pEncComp->inputFormat.eCompressionFormat = OMX_VIDEO_CodingUnused;
				pEncComp->inputFormat.eColorFormat = OMX_COLOR_FormatYUV420Planar;
				pEncComp->inputFormat.nPortIndex= 0;
				/* Set Output Fomrmat */
				pEncComp->outputFormat.eCompressionFormat = OMX_VIDEO_CodingAVC;
				pEncComp->outputFormat.eColorFormat = OMX_COLOR_FormatUnused;
				pEncComp->outputFormat.nPortIndex= 1;
				pEncComp->vpuCodecId = NX_AVC_ENC;
			}
			else if( !strcmp( (OMX_STRING)pInRole->cRole, "video_encoder.mpeg4"  )  ) {
				/* Set Input Format */
				pEncComp->inputFormat.eCompressionFormat = OMX_VIDEO_CodingUnused;
				pEncComp->inputFormat.eColorFormat = OMX_COLOR_FormatYUV420Planar;
				pEncComp->inputFormat.nPortIndex= 0;
				/* Set Output Fomrmat */
				pEncComp->outputFormat.eCompressionFormat = OMX_VIDEO_CodingMPEG4;
				pEncComp->outputFormat.eColorFormat = OMX_COLOR_FormatUnused;
				pEncComp->outputFormat.nPortIndex= 1;
				pEncComp->vpuCodecId = NX_MP4_ENC;
			}
			else if ( (!strcmp( (OMX_STRING)pInRole->cRole, "video_encoder.h263") ) || (!strcmp( (OMX_STRING)pInRole->cRole, "video_encoder.3gpp")) )
			{
				/* Set Input Format */
				pEncComp->inputFormat.eCompressionFormat = OMX_VIDEO_CodingUnused;
				pEncComp->inputFormat.eColorFormat = OMX_COLOR_FormatYUV420Planar;
				pEncComp->inputFormat.nPortIndex= 0;
				/* Set Output Fomrmat */
				pEncComp->outputFormat.eCompressionFormat = OMX_VIDEO_CodingH263;
				pEncComp->outputFormat.eColorFormat = OMX_COLOR_FormatUnused;
				pEncComp->outputFormat.nPortIndex= 1;
				pEncComp->vpuCodecId = NX_H263_ENC;
			}
			else{
				/* Error */
				ErrMsg("Error: %s(): in role = %s\n", __FUNCTION__, (OMX_STRING)pInRole->cRole );
				return OMX_ErrorBadParameter;
			}
			if( pEncComp->compRole ){
				free ( pEncComp->compRole );
				pEncComp->compRole = strdup((OMX_STRING)pInRole->cRole);
			}
			TRACE("%s(): Set new role : in role = %s, comp role = %s\n", __FUNCTION__, (OMX_STRING)pInRole->cRole, pEncComp->compRole );
			break;
		}
#ifdef ANDROID
		case OMX_IndexEnableAndroidNativeBuffers:
		{
			struct EnableAndroidNativeBuffersParams *pEnNativeBuf = (struct EnableAndroidNativeBuffersParams *)ComponentParamStruct;
			if( pEnNativeBuf->nPortIndex != 1 )
				return OMX_ErrorBadPortIndex;
			pEncComp->bUseNativeBuffer = pEnNativeBuf->enable;
			/* FIXME : TODO : Should be delete fourced TRUE. */
#ifdef	LOLLIPOP
			pEncComp->bUseNativeBuffer = OMX_TRUE;
#endif
			DbgMsg("Native buffer flag is %s!!", (pEncComp->bUseNativeBuffer==OMX_TRUE)?"Enabled":"Disabled");
			break;
		}
#endif
		case OMX_IndexParamVideoPortFormat:
		{
			OMX_VIDEO_PARAM_PORTFORMATTYPE *pVideoFormat = (OMX_VIDEO_PARAM_PORTFORMATTYPE *)ComponentParamStruct;
			int32_t h_stride = pEncComp->encWidth;
			int32_t v_stride = pEncComp->encHeight;
			if( pVideoFormat->nPortIndex == 0 )
			{
				memcpy( &pEncComp->inputFormat, pVideoFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE) );
			}
			else
			{
				memcpy( &pEncComp->outputFormat, pVideoFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE) );
			}
			pEncComp->pOutputPort->stdPortDef.nBufferSize = h_stride*v_stride + ALIGN(v_stride>>1,16) * ALIGN(h_stride>>1,16) * 2;
			break;
		}

		case OMX_IndexParamVideoMpeg4:
		{
			OMX_VIDEO_PARAM_MPEG4TYPE *pMp4Type = (OMX_VIDEO_PARAM_MPEG4TYPE *)ComponentParamStruct;
			TRACE("%s() : OMX_IndexParamVideoMpeg4 : port Index = %ld\n", __FUNCTION__, pMp4Type->nPortIndex );
			if( pMp4Type->nPortIndex != 1 )
			{
				return OMX_ErrorBadPortIndex;
			}
			memcpy( &pEncComp->omxMp4EncParam, pMp4Type, sizeof(OMX_VIDEO_PARAM_MPEG4TYPE) );
			break;
		}
		case OMX_IndexParamVideoAvc:
		{
			OMX_VIDEO_PARAM_AVCTYPE *pAvcType = (OMX_VIDEO_PARAM_AVCTYPE *)ComponentParamStruct;
			TRACE("%s() : OMX_IndexParamVideoAvc : port Index = %ld\n", __FUNCTION__, pAvcType->nPortIndex );
			if( pAvcType->nPortIndex != 1 )
			{
				return OMX_ErrorBadPortIndex;
			}
			memcpy( &pEncComp->omxMp4EncParam, pAvcType, sizeof(OMX_VIDEO_PARAM_AVCTYPE) );
			break;
		}
		case OMX_IndexParamVideoH263:
		{
			OMX_VIDEO_PARAM_H263TYPE *pH263Type = (OMX_VIDEO_PARAM_H263TYPE *)ComponentParamStruct;
			TRACE("%s() : OMX_IndexParamVideoH263 : port Index = %ld\n", __FUNCTION__, pH263Type->nPortIndex );
			if( pH263Type->nPortIndex != 1 )
			{
				return OMX_ErrorBadPortIndex;
			}
			memcpy( &pEncComp->omxH263EncParam, pH263Type, sizeof(OMX_VIDEO_PARAM_H263TYPE) );
			break;
		}

		case OMX_IndexParamVideoBitrate:
		{
			OMX_VIDEO_PARAM_BITRATETYPE *pBitRate = (OMX_VIDEO_PARAM_BITRATETYPE *)ComponentParamStruct;
			TRACE("%s() : OMX_IndexParamVideoBitrate : port Index = %ld\n", __FUNCTION__, pBitRate->nPortIndex );
			if( pBitRate->nPortIndex != 1 )
				return OMX_ErrorBadPortIndex;

			if( pBitRate->nTargetBitrate == 0 )
			{
				/* Default Bitrate */
				pEncComp->encBitRate = VIDENC_DEF_BITRATE;
			}
			else{
				pEncComp->encBitRate = pBitRate->nTargetBitrate;
			}
			break;
		}
		case OMX_IndexParamVideoIntraRefresh:
		{
			OMX_VIDEO_PARAM_INTRAREFRESHTYPE *pIRF = (OMX_VIDEO_PARAM_INTRAREFRESHTYPE *)ComponentParamStruct;
			if( pIRF->eRefreshMode == OMX_VIDEO_IntraRefreshCyclic )
			{
				pEncComp->encIntraRefreshMbs = pIRF->nCirMBs;
			}
			else if( pIRF->eRefreshMode == OMX_VIDEO_IntraRefreshAdaptive )
			{
				ErrMsg("Unsupported Encoder Setting( IntraRefreshMode(OMX_VIDEO_IntraRefreshAdaptive)!!!");
				return OMX_ErrorUnsupportedSetting;
			}
			else if( pIRF->eRefreshMode == OMX_VIDEO_IntraRefreshBoth )
			{
				ErrMsg("Unsupported Encoder Setting( IntraRefreshMode(OMX_VIDEO_IntraRefreshBoth))!!!");
				return OMX_ErrorUnsupportedSetting;
			}
			else
			{
				return OMX_ErrorBadParameter;
			}
		}
		case OMX_IndexConfigVideoFramerate:
		{
			OMX_CONFIG_FRAMERATETYPE *pFrameRate = (OMX_CONFIG_FRAMERATETYPE *)ComponentParamStruct;
			TRACE("%s() : OMX_IndexConfigVideoFramerate : port Index = %ld\n", __FUNCTION__, pFrameRate->nPortIndex );
			if( pFrameRate->nPortIndex != 1 )
				return OMX_ErrorBadPortIndex;

			if( pFrameRate->xEncodeFramerate == 0 )
			{
				/* Default Bitrate */
				pEncComp->encFrameRate = VIDENC_DEF_FRAMERATE;
			}
			else{
				pEncComp->encFrameRate = pFrameRate->xEncodeFramerate;
			}
			break;
		}
		case OMX_IndexParamVideoErrorCorrection:
		{
			/* OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *errCorrection = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)ComponentParamStruct; */
			break;
		}
#ifdef ANDROID
		/* For Android Extension */
		case OMX_IndexStoreMetaDataInBuffers:
		{
			struct StoreMetaDataInBuffersParams *pParam = (struct StoreMetaDataInBuffersParams *)ComponentParamStruct;
			TRACE("%s() : OMX_IndexStoreMetaDataInBuffers : port Index = %ld\n", __FUNCTION__, pParam->nPortIndex );
			pEncComp->bMetaDataInBuffers = pParam->bStoreMetaData;
			break;
		}
#endif
		default :
			return NX_BaseSetParameter( hComp, nParamIndex, ComponentParamStruct );
	}
	return OMX_ErrorNone;
}

static OMX_ERRORTYPE NX_VidEncUseBuffer (OMX_HANDLETYPE hComponent, OMX_BUFFERHEADERTYPE** ppBufferHdr, OMX_U32 nPortIndex, OMX_PTR pAppPrivate, OMX_U32 nSizeBytes, OMX_U8* pBuffer)
{
	OMX_COMPONENTTYPE *pStdComp = (OMX_COMPONENTTYPE *)hComponent;
	NX_VIDENC_COMP_TYPE *pEncComp = (NX_VIDENC_COMP_TYPE *)pStdComp->pComponentPrivate;
	NX_BASEPORTTYPE *pPort = NULL;
	OMX_BUFFERHEADERTYPE         **pPortBuf = NULL;
	OMX_U32 i=0;

	if( nPortIndex >= pEncComp->nNumPort )
	{
		return OMX_ErrorBadPortIndex;
	}

	if( OMX_StateLoaded != pEncComp->eCurState || OMX_StateIdle != pEncComp->eNewState )
	{
		return OMX_ErrorIncorrectStateTransition;
	}

	if( 0 ==nPortIndex )
	{
		pPort = pEncComp->pInputPort;
		pPortBuf = pEncComp->pInputBuffers;
	}else
	{
		pPort = pEncComp->pOutputPort;
		pPortBuf = pEncComp->pOutputBuffers;
	}

	if( pPort->stdPortDef.nBufferSize > nSizeBytes )
	{
		return OMX_ErrorBadParameter;
	}

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
			pPortBuf[i]->pBuffer = pBuffer;
			pPortBuf[i]->nAllocLen = nSizeBytes;
			pPortBuf[i]->pAppPrivate = pAppPrivate;
			pPortBuf[i]->pPlatformPrivate = pStdComp;
			if( OMX_DirInput == pPort->stdPortDef.eDir )
			{
				pPortBuf[i]->nInputPortIndex = pPort->stdPortDef.nPortIndex;
			}else
			{
				pPortBuf[i]->nOutputPortIndex = pPort->stdPortDef.nPortIndex;
			}
			pPort->nAllocatedBuf ++;
			if( pPort->nAllocatedBuf == pPort->stdPortDef.nBufferCountActual )
			{
				pPort->stdPortDef.bPopulated = OMX_TRUE;
				pPort->eBufferType = NX_BUFHEADER_TYPE_USEBUFFER;
				NX_PostSem(pEncComp->hBufAllocSem);
			}
			*ppBufferHdr = pPortBuf[i];
			return OMX_ErrorNone;
		}
	}
	return OMX_ErrorInsufficientResources;
}



/*
 *		Command Execution Thread
 */

static OMX_ERRORTYPE NX_VidEncStateTransition( NX_VIDENC_COMP_TYPE *pEncComp, OMX_STATETYPE eCurState, OMX_STATETYPE eNewState )
{
	OMX_U32 i=0;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_PARAM_PORTDEFINITIONTYPE *pPort = NULL;
	NX_QUEUE *pQueue = NULL;
	OMX_BUFFERHEADERTYPE *pBufHdr = NULL;

	/* Check Basic Erros */
	if( eCurState == eNewState )
	{
		return OMX_ErrorSameState;
	}
	if( OMX_StateInvalid==eCurState || OMX_StateInvalid==eNewState )
	{
		return OMX_ErrorInvalidState;
	}

	if( OMX_StateLoaded == eCurState )
	{
		switch( eNewState )
		{
			/* CHECKME */
			case OMX_StateIdle:
				/* Wait buffer allocation event */
				for( i=0 ; i<pEncComp->nNumPort ; i++ ){
					pPort = (OMX_PARAM_PORTDEFINITIONTYPE *)pEncComp->pPort[i];
					if( OMX_TRUE == pPort->bEnabled ){
						NX_PendSem(pEncComp->hBufAllocSem);
					}
					//
					//	TODO : Need exit check.
					//
				}

				/* Open Decoder */
				if( EncoderOpen( pEncComp ) != 0 )
				{
					DbgMsg("EncoderOpen() -- Error\n");
					return OMX_ErrorUndefined;
				}

				/* Start buffer management thread */
				pEncComp->eCmdBufThread = NX_THREAD_CMD_PAUSE;
				/* Create buffer control semaphore */
				pEncComp->hBufCtrlSem = NX_CreateSem(0, 1);
				/* Create buffer mangement semaphore */
				pEncComp->hBufChangeSem = NX_CreateSem(0, VIDDEC_NUM_PORT*1024);
				/* Create buffer management thread */
				pEncComp->eCmdBufThread = NX_THREAD_CMD_PAUSE;
				/* Create buffer management thread */
				pthread_create( &pEncComp->hBufThread, NULL, (void*)&NX_VidEncBufferMgmtThread , pEncComp );
				/* Wait thread creation */
				NX_PendSem(pEncComp->hBufCtrlSem);
				pEncComp->eCurState = eNewState;
				break;
			case OMX_StateWaitForResources:
			case OMX_StateExecuting:
			case OMX_StatePause:
			default:
				return OMX_ErrorIncorrectStateTransition;
		}
	}
	else if( OMX_StateIdle == eCurState )
	{
		switch( eNewState )
		{
			case OMX_StateLoaded:
			{
				/* Open Decoder */
				EncoderClose( pEncComp );

				/* Exit buffer management thread */
				pEncComp->eCmdBufThread = NX_THREAD_CMD_EXIT;
				NX_PostSem( pEncComp->hBufChangeSem );
				NX_PostSem( pEncComp->hBufCtrlSem );
				pthread_join( pEncComp->hBufThread, NULL );
				NX_DestroySem( pEncComp->hBufChangeSem );
				NX_DestroySem( pEncComp->hBufCtrlSem );
				pEncComp->hBufChangeSem = NULL;
				pEncComp->hBufCtrlSem = NULL;

				/* Wait buffer free */
				for( i=0 ; i<pEncComp->nNumPort ; i++ )
				{
					pPort = (OMX_PARAM_PORTDEFINITIONTYPE *)pEncComp->pPort[i];
					if( OMX_TRUE == pPort->bEnabled )
					{
						NX_PendSem(pEncComp->hBufAllocSem);
					}
					//
					//	TODO : Need exit check.
					//
				}
				pEncComp->eCurState = eNewState;
				break;
			}
			case OMX_StateExecuting:
				/* Step 1. Check in/out buffer queue.							*/
				/* Step 2. If buffer is not ready in the queue, return error.	*/
				/* Step 3. Craete buffer management thread.						*/
				/* Step 4. Start buffer processing.								*/
				pEncComp->eCmdBufThread = NX_THREAD_CMD_RUN;
				NX_PostSem( pEncComp->hBufCtrlSem );
				pEncComp->eCurState = eNewState;
				break;
			case OMX_StatePause:
				/* Step 1. Check in/out buffer queue.							*/
				/* Step 2. If buffer is not ready in the queue, return error.	*/
				/* Step 3. Craete buffer management thread.						*/
				pEncComp->eCurState = eNewState;
				break;
			case OMX_StateWaitForResources:
			default:
				return OMX_ErrorIncorrectStateTransition;
		}
	}else if( OMX_StateExecuting == eCurState ){
		switch( eNewState )
		{
			case OMX_StateIdle:
				/* Step 1. Stop buffer processing */
				pEncComp->eCmdBufThread = NX_THREAD_CMD_PAUSE;
				/* Write dummy */
				NX_PostSem( pEncComp->hBufChangeSem );
				/* Step 2. Flushing buffers. */
				/* Return buffer to supplier. */
				pthread_mutex_lock( &pEncComp->hBufMutex );

				if( pEncComp->pPrevInputBuffer )
				{
					pEncComp->pCallbacks->EmptyBufferDone(pEncComp->hComp, pEncComp->hComp->pApplicationPrivate, pEncComp->pPrevInputBuffer);
				}

				for( i=0 ; i<pEncComp->nNumPort ; i++ )
				{
					pPort = (OMX_PARAM_PORTDEFINITIONTYPE *)pEncComp->pPort[i];
					pQueue = (NX_QUEUE*)pEncComp->pBufQueue[i];
					if( OMX_DirInput == pPort->eDir )
					{
						do {
							pBufHdr = NULL;
							if( 0 == NX_PopQueue( pQueue, (void**)&pBufHdr ) )
							{
								pEncComp->pCallbacks->EmptyBufferDone( pEncComp->hComp, pEncComp->hComp->pApplicationPrivate, pBufHdr );
							}
							else
							{
								break;
							}
						} while(1);
					}
					else if( OMX_DirOutput == pPort->eDir )
					{
						do {
							pBufHdr = NULL;
							if( 0 == NX_PopQueue( pQueue, (void**)&pBufHdr ) )
							{
								pEncComp->pCallbacks->FillBufferDone( pEncComp->hComp, pEncComp->hComp->pApplicationPrivate, pBufHdr );
							}
							else
							{
								break;
							}
						}while(1);
					}
				}

				pthread_mutex_unlock( &pEncComp->hBufMutex );
				/* Step 3. Exit buffer management thread. */
				pEncComp->eCurState = eNewState;
				break;
			case OMX_StatePause:
				/* Step 1. Stop buffer processing using buffer management semaphore */
				pEncComp->eCurState = eNewState;
				break;
			case OMX_StateLoaded:
			case OMX_StateWaitForResources:
			default:
				return OMX_ErrorIncorrectStateTransition;
		}
	}
	else if( OMX_StatePause==eCurState )
	{
		switch( eNewState )
		{
			case OMX_StateIdle:
				/* Step 1. Flushing buffers. */
				/* Step 2. Exit buffer management thread. */
				pEncComp->eCurState = eNewState;
				break;
			case OMX_StateExecuting:
				/* Step 1. Start buffer processing. */
				pEncComp->eCurState = eNewState;
				break;
			case OMX_StateLoaded:
			case OMX_StateWaitForResources:
			default:
				return OMX_ErrorIncorrectStateTransition;
		}
	}
	else if( OMX_StateWaitForResources==eCurState )
	{
		switch( eNewState )
		{
			case OMX_StateLoaded:
				pEncComp->eCurState = eNewState;
				break;
			case OMX_StateIdle:
				pEncComp->eCurState = eNewState;
				break;
			case OMX_StateExecuting:
			case OMX_StatePause:
			default:
				return OMX_ErrorIncorrectStateTransition;
		}
	}
	else
	{
		/* Error */
		return OMX_ErrorUndefined;
	}
	return eError;
}

static void NX_VidEncCommandProc( NX_BASE_COMPNENT *pBaseComp, OMX_COMMANDTYPE Cmd, OMX_U32 nParam1, OMX_PTR pCmdData )
{
	NX_VIDENC_COMP_TYPE *pEncComp = (NX_VIDENC_COMP_TYPE *)pBaseComp;
	OMX_ERRORTYPE eError=OMX_ErrorNone;
	OMX_EVENTTYPE eEvent = OMX_EventCmdComplete;
	OMX_COMPONENTTYPE *pStdComp = pEncComp->hComp;
	OMX_U32 nData1 = 0, nData2 = 0;

	(void)eError;

	switch( Cmd )
	{
		/* If the component successfully transitions to the new state,						*/
		/* it notifies the IL client of the new state via the OMX_EventCmdComplete event,	*/
		/* indicating OMX_CommandStateSet for nData1 and the new state for nData2.			*/
		case OMX_CommandStateSet:    // Change the component state
		{
			if( pEncComp->eCurState == nParam1 )
			{
				eEvent = OMX_EventError;
				nData1 = OMX_ErrorSameState;
				break;
			}

			eError = NX_VidEncStateTransition( pEncComp, pEncComp->eCurState, nParam1 );
			nData1 = OMX_CommandStateSet;
			nData2 = nParam1;		/*	Must be set new state (OMX_STATETYPE) */
			TRACE("%s : OMX_CommandStateSet\n", __FUNCTION__);
			break;
		}
		case OMX_CommandFlush:		/* Flush the data queue(s) of a component */
		{
			OMX_BUFFERHEADERTYPE* pBuf = NULL;
			TRACE("%s : OMX_CommandFlush \n", __FUNCTION__);

			pthread_mutex_lock( &pEncComp->hBufMutex );

			if( nParam1 == VIDDEC_INPORT_INDEX || nParam1 == OMX_ALL )
			{
				do {
					if( NX_GetQueueCnt(pEncComp->pInputPortQueue) > 0 )
					{
						/* Flush buffer */
						NX_PopQueue( pEncComp->pInputPortQueue, (void**)&pBuf );
						pEncComp->pCallbacks->EmptyBufferDone(pStdComp, pStdComp->pApplicationPrivate, pBuf);
					}
					else
					{
						break;
					}
				} while(1);
				SendEvent( (NX_BASE_COMPNENT*)pEncComp, eEvent, OMX_CommandFlush, VIDDEC_INPORT_INDEX, pCmdData );
			}

			if( nParam1 == VIDDEC_OUTPORT_INDEX || nParam1 == OMX_ALL )
			{
				do {
					if( NX_GetQueueCnt(pEncComp->pOutputPortQueue) > 0 )
					{
						/* Flush buffer */
						NX_PopQueue( pEncComp->pOutputPortQueue, (void**)&pBuf );
						pEncComp->pCallbacks->FillBufferDone(pStdComp, pStdComp->pApplicationPrivate, pBuf);
					}
					else
					{
						break;
					}
				} while(1);
				SendEvent( (NX_BASE_COMPNENT*)pEncComp, eEvent, OMX_CommandFlush, VIDDEC_OUTPORT_INDEX, pCmdData );
			}

			if( nParam1 == OMX_ALL )		/* Output Port Flushing */
			{
				SendEvent( (NX_BASE_COMPNENT*)pEncComp, eEvent, OMX_CommandFlush, OMX_ALL, pCmdData );
			}

			pthread_mutex_unlock( &pEncComp->hBufMutex );
			//pEncComp->bFlush = OMX_TRUE;
			return;
		}
		/* Openmax spec v1.1.2 : 3.4.4.3 Non-tunneled Port Disablement and Enablement. */
		case OMX_CommandPortDisable:	/* Disable a port on a component. */
		{
			/* Check Parameter */
			if( OMX_ALL != nParam1 || VIDDEC_NUM_PORT>=nParam1 )
			{
				/* Bad parameter */
				eEvent = OMX_EventError;
				nData1 = OMX_ErrorBadPortIndex;
				ErrMsg(" Errror : %s:Line(%d) : OMX_ErrorBadPortIndex(%ld)\n", __FILE__, __LINE__, nParam1);
				break;
			}

			/* Step 1. The component shall return the buffers with a call to EmptyBufferDone/FillBufferDone, */
			NX_PendSem( pEncComp->hBufCtrlSem );
			if( OMX_ALL == nParam1 )
			{
				/* Disable all ports */
			}else
			{
				pEncComp->pInputPort->stdPortDef.bEnabled = OMX_FALSE;
				/* Specific port */
			}
			TRACE("NX_VidEncCommandProc : OMX_CommandPortDisable \n");
			NX_PostSem( pEncComp->hBufCtrlSem );
			break;
		}
		case OMX_CommandPortEnable:		/* Enable a port on a component. */
		{
			NX_PendSem( pEncComp->hBufCtrlSem );
			TRACE("NX_VidEncCommandProc : OMX_CommandPortEnable \n");
			NX_PostSem( pEncComp->hBufCtrlSem );
			break;
		}
		case OMX_CommandMarkBuffer:		/* Mark a component/buffer for observation */
		{
			NX_PendSem( pEncComp->hBufCtrlSem );
			TRACE("NX_VidEncCommandProc : OMX_CommandMarkBuffer \n");
			NX_PostSem( pEncComp->hBufCtrlSem );
			break;
		}
		default:
		{
			break;
		}
	}
	SendEvent( (NX_BASE_COMPNENT*)pEncComp, eEvent, nData1, nData2, pCmdData );
}



/*
 *		Buffer Management Thread
 */

static int NX_VidEncTransform(NX_VIDENC_COMP_TYPE *pEncComp, NX_QUEUE *pInQueue, NX_QUEUE *pOutQueue)
{
	return EncodeFrame( pEncComp, pInQueue, pOutQueue );
}

/*
 *
 *	Buffer control semaphore is synchronize state of component and state of buffer thread.
 *
 *	Transition Action
 *	1. OMX_StateLoaded   --> OMX_StateIdle      ;
 *		==> Create Buffer management thread, initialize buf change sem & buf control sem,
 *			thread state set to NX_THREAD_CMD_PAUSE
 *	2. OMX_StateIdle     --> OMX_StateExecuting ;
 *		==> Set state to NX_THREAD_CMD_RUN and post control semaphore
 *	3. OMX_StateIdle     --> OMX_StatePause     ;
 *		==> Noting.
 *	4. OMX_StatePause    --> OMX_StateExecuting ;
 *		==> Set state to NX_THREAD_CMD_RUN and post control semaphore
 *	5. OMX_StateExcuting --> OMX_StatePause     ;
 *		==> Set state to NX_THREAD_CMD_PAUSE and post dummy buf change semaphore
 *	6. OMX_StateExcuting --> OMX_StateIdle      ;
 *		==> Set state to NX_THREAD_CMD_PAUSE and post dummy buf change semaphore and
 *			return all buffers on the each port.
 *	7. OMX_StateIdle     --> OMX_Loaded         ;
 *
 */
static void NX_VidEncBufferMgmtThread( void *arg )
{
	NX_VIDENC_COMP_TYPE *pEncComp = (NX_VIDENC_COMP_TYPE *)arg;
	NX_PostSem( pEncComp->hBufCtrlSem );					/* Thread Creation Semaphore */
	while( NX_THREAD_CMD_EXIT != pEncComp->eCmdBufThread )
	{
		NX_PendSem( pEncComp->hBufCtrlSem );				/* Thread Control Semaphore */
		while( NX_THREAD_CMD_RUN == pEncComp->eCmdBufThread )
		{
			NX_PendSem( pEncComp->hBufChangeSem );			/* wait buffer management command */
			if( NX_THREAD_CMD_RUN != pEncComp->eCmdBufThread )
			{
				break;
			}
			/* check input buffer first */
			pthread_mutex_lock( &pEncComp->hBufMutex );
			if( NX_GetQueueCnt(pEncComp->pInputPortQueue)>0 && NX_GetQueueCnt(pEncComp->pOutputPortQueue)>0 )
			{
				if( NX_VidEncTransform(pEncComp, pEncComp->pInputPortQueue, pEncComp->pOutputPortQueue) < 0 )
				{
					pEncComp->pCallbacks->EventHandler(
						(OMX_HANDLETYPE)pEncComp->hComp, pEncComp->pCallbackData, OMX_EventError, 0, 0, NULL );
				}
			}
			pthread_mutex_unlock( &pEncComp->hBufMutex );
		}
	}
	/* Release or Return buffer's */
	TRACE(" exit buffer management thread.\n");
}




/*
 *	Codec Management Functions
 */
#include <linux/videodev2.h>
#include <linux/videodev2_nxp_media.h>

static OMX_S32 CodecIdToV4l2Format( OMX_S32 codecId )
{
	OMX_S32 v4l2Format = -1;

	if( codecId == NX_AVC_ENC )
	{
		v4l2Format = V4L2_PIX_FMT_H264;
	}
	else if( codecId == NX_MP4_ENC )
	{
		v4l2Format = V4L2_PIX_FMT_MPEG4;
	}
	else if( codecId == NX_H263_ENC )
	{
		v4l2Format = V4L2_PIX_FMT_H263;
	}
	else if( codecId == NX_JPEG_ENC )
	{
	}
	/* Due To : Add v4l2Format according to Codec Format */
	else
	{
		ErrMsg("CodecIdToV4l2Format : Invalid Format. ( %d )\n", codecId );
		return -1;
	}
	return v4l2Format;
}

static OMX_S32 EncoderOpen(NX_VIDENC_COMP_TYPE *pEncComp)
{
	/* Find Input Port Definition */
	OMX_PARAM_PORTDEFINITIONTYPE *pInPortDef = (OMX_PARAM_PORTDEFINITIONTYPE *)pEncComp->pPort[0];
	pEncComp->encWidth = pInPortDef->format.video.nFrameWidth;
	pEncComp->encHeight = pInPortDef->format.video.nFrameHeight;
	NX_V4L2ENC_PARAM encInitParam;
	uint8_t *pSeqBuf;

	/* add by kshblue (14.07.04) */
	if ( pEncComp->encWidth == 0 || pEncComp->encHeight == 0)
	{
		pEncComp->encWidth = 1280, pEncComp->encHeight = 720;
	}

	TRACE( "==============================================\n" );
	TRACE( "  encWidth       = %d\n", pEncComp->encWidth       );
	TRACE( "  encHeight      = %d\n", pEncComp->encHeight      );
	TRACE( "  encKeyInterval = %d\n", pEncComp->encKeyInterval );
	TRACE( "  encFrameRate   = %d\n", pEncComp->encFrameRate   );
	TRACE( "  encBitRate     = %d\n", pEncComp->encBitRate     );
	TRACE( "==============================================\n" );

	pEncComp->hVpuCodec = NX_V4l2EncOpen( CodecIdToV4l2Format(pEncComp->vpuCodecId) );
	if( NULL == pEncComp->hVpuCodec  )
	{
		ErrMsg("NX_V4l2EncOpen() failed\n");
		return -1;
	}

	memset(&encInitParam, 0, sizeof(encInitParam));

	/* Check NV12 Format */
#ifdef LOLLIPOP
	if( pEncComp->bUseNativeBuffer == OMX_FALSE && pEncComp->inputFormat.eColorFormat == OMX_COLOR_FormatAndroidOpaque )
#else
	if( pEncComp->inputFormat.eColorFormat == OMX_COLOR_FormatAndroidOpaque )
#endif
	{
		DbgMsg("Encoder Input Format NV12\n");
		encInitParam.chromaInterleave = 1;
	}

	encInitParam.profile = V4L2_MPEG_VIDEO_H263_PROFILE_P3;

	encInitParam.width   = pEncComp->encWidth;
	encInitParam.height  = pEncComp->encHeight;
	encInitParam.gopSize = pEncComp->encKeyInterval;
	encInitParam.bitrate = ( pEncComp->encBitRate ) ? ( pEncComp->encBitRate ) : ( 10*1024*1024 );
	encInitParam.fpsNum  = pEncComp->encFrameRate;
	encInitParam.fpsDen  = 1;
	encInitParam.numIntraRefreshMbs = pEncComp->encIntraRefreshMbs;
	encInitParam.searchRange = 1;
	encInitParam.rotAngle = 0;
	encInitParam.mirDirection = 0;

	/* Rate Control */
	encInitParam.enableRC = ( encInitParam.bitrate ) ? (1) : (0);	/* Enable Rate Control */
	encInitParam.RCAlgorithm = 1;
	encInitParam.disableSkip = 0;		/* Enable Skip */
	encInitParam.maximumQp = 0;			/* Max Qunatization Scale */
	encInitParam.initialQp = ( encInitParam.bitrate ) ? (0) : (23);	/* Default Encoder API ( enableRC == 0 ) */
	encInitParam.RCDelay = 0;
	encInitParam.rcVbvSize = 0;
	encInitParam.gammaFactor = 0;

	if( VID_ERR_NONE != NX_V4l2EncInit( pEncComp->hVpuCodec, &encInitParam ) )
	{
		ErrMsg("NX_V4l2EncInit() failed\n");
		return -1;
	}

	if( VID_ERR_NONE != NX_V4l2EncGetSeqInfo( pEncComp->hVpuCodec, &pSeqBuf, &pEncComp->seqBufSize ) )
	{
		ErrMsg("NX_V4l2EncGetSeqInfo() failed\n");
		return -1;
	}
	memcpy( pEncComp->pSeqBuf, pSeqBuf, pEncComp->seqBufSize );

	if( pEncComp->seqBufSize > 0 )
	{
		DbgMsg("Sequrnce buffer Size = %d\n", pEncComp->seqBufSize );
		pEncComp->bCodecSpecificInfo = OMX_TRUE;
	}

	return 0;
}

static OMX_S32 EncoderClose(NX_VIDENC_COMP_TYPE *pEncComp)
{
	if( pEncComp->hVpuCodec )
	{
		NX_V4l2EncClose( pEncComp->hVpuCodec );
		pEncComp->hVpuCodec = NULL;
	}

	if( pEncComp->hCSCMem )
	{
		NX_FreeVideoMemory( pEncComp->hCSCMem );
		pEncComp->hCSCMem = NULL;
	}

	return 0;
}


/*
 *	pSrc : Y + U(Cb) + V(Cr) (IYUV format)
 */
static int32_t CopyImage( uint8_t *pSrc, int32_t w, int32_t h, NX_VID_MEMORY_INFO *pImg )
{
	int32_t i, j;
	uint8_t *pDst, *pCb, *pCr;

	/* Copy Lu */
	pDst = (uint8_t*)pImg->pBuffer[0];
	for( i=0 ; i<h ; i++ )
	{
		memcpy(pDst, pSrc, w);
		pDst += pImg->stride[0];
		pSrc += w;
	}

	pCb = pSrc;
	pCr = pSrc + w*h/4;

	/* switch( pImg->fourCC ) */
	switch( pImg->format )
	{
		case FOURCC_NV12:
		{
			uint8_t *pCbCr;
			pDst = (uint8_t*)pImg->pBuffer[1];
			for( i=0 ; i<h/2 ; i++ )
			{
				pCbCr = pDst + pImg->stride[1]*i;
				for( j=0 ; j<w/2 ; j++ )
				{
					*pCbCr++ = *pCb++;
					*pCbCr++ = *pCr++;
				}
			}
			break;
		}
		case FOURCC_NV21:
		{
			uint8_t *pCrCb;
			pDst = (uint8_t*)pImg->pBuffer[1];
			for( i=0 ; i<h/2 ; i++ )
			{
				pCrCb = pDst + pImg->stride[1]*i;
				for( j=0 ; j<w/2 ; j++ )
				{
					*pCrCb++ = *pCr++;
					*pCrCb++ = *pCb++;
				}
			}
			break;
		}
		case FOURCC_MVS0:
		case FOURCC_YV12:
		case FOURCC_IYUV:
		{
			/* Cb */
			pDst = (uint8_t*)pImg->pBuffer[1];
			for( i=0 ; i<h/2 ; i++ )
			{
				memcpy(pDst, pCb, w/2);
				pDst += pImg->stride[1];
				pCb += w/2;
			}

			/* Cr */
			pDst = (uint8_t*)pImg->pBuffer[2];
			for( i=0 ; i<h/2 ; i++ )
			{
				memcpy(pDst, pCr, w/2);
				pDst += pImg->stride[2];
				pCr += w/2;
			}
			break;
		}
	}
	return 0;
}

static OMX_S32 EncodeFrame(NX_VIDENC_COMP_TYPE *pEncComp, NX_QUEUE *pInQueue, NX_QUEUE *pOutQueue)
{
	OMX_BUFFERHEADERTYPE* pInBuf = NULL, *pOutBuf = NULL;
	NX_V4L2ENC_IN encIn;
	NX_V4L2ENC_OUT encOut;
	VPU_ERROR_E ret;
	/* NX_VID_MEMORY_INFO inputMem; */

	FUNC_IN;

	if( NX_PopQueue( pInQueue, (void**)&pInBuf ) || pInBuf == NULL )
	{
		return 0;
	}

	if( pInBuf->nFlags & OMX_BUFFERFLAG_EOS )
	{
		pInBuf->nFilledLen = 0;		/* wast all input buffer */
		pEncComp->pCallbacks->EmptyBufferDone( pEncComp, pEncComp->hComp->pApplicationPrivate, pInBuf );

		NX_PopQueue( pOutQueue, (void**)&pOutBuf );
		pOutBuf->nOffset = 0;
		pOutBuf->nTimeStamp = pInBuf->nTimeStamp;
		pOutBuf->nFilledLen = 0;
		pOutBuf->nFlags = pInBuf->nFlags;
		pEncComp->pCallbacks->FillBufferDone( pEncComp, pEncComp->hComp->pApplicationPrivate, pOutBuf );
		return 0;
	}

#if 0
	/*
	 *	ION memory( Android Gralloc private_handle_t type )
	 */
	if( pEncComp->bUseNativeBuffer == OMX_TRUE || pEncComp->bMetaDataInBuffers == OMX_TRUE )
	{
		struct private_handle_t const *hPrivate;
		hPrivate = (struct private_handle_t const *)recodingBuffer[1];
		int ion_fd = ion_open();
		if( ion_fd<0 )
		{
			ALOGE("%s: failed to ion_open", __func__);
			return ion_fd;
		}
		int ret = ion_get_phys(ion_fd, hPrivate->share_fd, (long unsigned int *)&inputMem.luPhyAddr);
		if (ret != 0) {
			ALOGE("%s: failed to ion_get_phys", __func__);
			close( ion_fd );
			return ret;
		}

		int vStride = ALIGN(hPrivate->height, 16);
		inputMem.memoryMap = 0;		//	Linear
		inputMem.fourCC    = FOURCC_MVS0;
		inputMem.imgWidth  = pEncComp->encWidth;
		inputMem.imgHeight = pEncComp->encHeight;
		inputMem.cbPhyAddr = inputMem.luPhyAddr + hPrivate->stride * vStride;
		inputMem.crPhyAddr = inputMem.cbPhyAddr + ALIGN(hPrivate->stride >> 1, 16) * ALIGN(vStride >> 1, 16);
		inputMem.luStride  = hPrivate->stride;
		inputMem.cbStride  = inputMem.crStride = hPrivate->stride >> 1;
        close(ion_fd);
	}
#endif
	/*
	 *	YV12 Data Format
	 */
#if 1
	/* Copy YV12 Format Memory to H/W Accessable memory */
	{
		/* Step 1. Allocate H/W Accessable Memory */
		if( pEncComp->hCSCMem == NULL )
		{
			pEncComp->hCSCMem = NX_AllocateVideoMemory( pEncComp->encWidth, pEncComp->encHeight, 3, FOURCC_MVS0, 4096 );
			if( pEncComp->hCSCMem )
			{
				if( 0 != NX_MapVideoMemory( pEncComp->hCSCMem ) )
				{
					printf("Video Memory Mapping Failed\n");
				}
				else
				{
					printf("Frame Memory allocation Failed\n");
				}
			}
		}
		if( pEncComp->hCSCMem )
		{
			char *srcY = (char*)pInBuf->pBuffer;
			char *srcU = srcY + pEncComp->encWidth * pEncComp->encHeight;
			char *srcV = srcU + pEncComp->encWidth * pEncComp->encHeight / 4;

			(void)srcV;

#if 0
			/* Copy System YV12 Memory to H/W YV12 Memory */
			cscYV12ToYV12(  srcY, srcU, srcV,
							(char*)pEncComp->hCSCMem->luVirAddr, (char*)pEncComp->hCSCMem->cbVirAddr, (char*)pEncComp->hCSCMem->crVirAddr,
							pEncComp->encWidth, pEncComp->hCSCMem->luStride, pEncComp->hCSCMem->cbStride,
							pEncComp->encWidth, pEncComp->encHeight );
#else
			/* ToDo : Need Implemention for CSC Routine */
#endif
			/* memcpy(&inputMem, pEncComp->hCSCMem, sizeof(inputMem) ); */
			encIn.pImage = pEncComp->hCSCMem;
			CopyImage( pInBuf->pBuffer, pEncComp->encWidth, pEncComp->encHeight, encIn.pImage );
		}
	}
#endif

	NX_PopQueue( pOutQueue, (void**)&pOutBuf );

	if( pEncComp->bSendCodecSpecificInfo==OMX_FALSE && pEncComp->seqBufSize>0 )
	{
		OMX_BUFFERHEADERTYPE* pDsi = pOutBuf;
		pDsi->nFlags = OMX_BUFFERFLAG_CODECCONFIG;
		pDsi->nOffset = 0;
		pDsi->nTimeStamp = 0;

		NxMemcpy( pDsi->pBuffer, pEncComp->pSeqBuf, pEncComp->seqBufSize );
		pDsi->nFilledLen = pEncComp->seqBufSize;

		DbgMsg(">> Send DSI Frame(%d) <<\n", pEncComp->seqBufSize);
		NX_PopQueue( pOutQueue, (void**)&pOutBuf );
		pEncComp->bSendCodecSpecificInfo = OMX_TRUE;
		pEncComp->pCallbacks->FillBufferDone( pEncComp->hComp, pEncComp->hComp->pApplicationPrivate, pDsi );
	}

	/* encIn.pImage = &inputMem; */
	encIn.timeStamp = -1;
	encIn.forcedIFrame = 0;
	encIn.forcedSkipFrame = 0;
	encIn.quantParam = 0;

	/* Encode Image */
	ret = NX_V4l2EncEncodeFrame( pEncComp->hVpuCodec, &encIn, &encOut );
	if( ret != VID_ERR_NONE )
	{
		ErrMsg("NX_V4l2EncEncodeFrame() failed !!!\n");
		return -1;
	}

	pInBuf->nFilledLen = 0;	/* wast all input buffer */
	pEncComp->pCallbacks->EmptyBufferDone( pEncComp, pEncComp->hComp->pApplicationPrivate, pInBuf );

	pOutBuf->nFlags = OMX_BUFFERFLAG_ENDOFFRAME;

	if( encOut.frameType )
		pOutBuf->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;

	memcpy(pOutBuf->pBuffer, encOut.outBuf, encOut.bufSize);
	pOutBuf->nOffset = 0;
	pOutBuf->nTimeStamp = pInBuf->nTimeStamp;
	pOutBuf->nFilledLen = encOut.bufSize;

	TRACE("Encoder : nTimeStamp(%lld), outLength(%ld) \n", pOutBuf->nTimeStamp, pOutBuf->nFilledLen);

	pEncComp->pCallbacks->FillBufferDone( pEncComp, pEncComp->hComp->pApplicationPrivate, pOutBuf );

	FUNC_OUT;
	return 0;
}
