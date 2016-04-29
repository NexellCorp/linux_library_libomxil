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
 *	File		: NX_OMXVideoDecoder.h
 *	Brief		: OpenMAX IL Video Decoder
 *	Author		: HyunChul Jun (hcjun@nexell.co.kr)
 *	History		: 2016.04.25 : Create
 */

#ifndef __NX_OMXVideoDecoder_h__
#define __NX_OMXVideoDecoder_h__

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_Video.h>
#include <NX_OMXBasePort.h>
#include <NX_OMXBaseComponent.h>
#include <NX_OMXSemaphore.h>
#include <NX_OMXQueue.h>

#ifdef ANDROID
#include <utils/Log.h>
#include <hardware/gralloc.h>
#include <cutils/native_handle.h>
#include <gralloc_priv.h>
#include <media/hardware/MetadataBufferType.h>
#include <sys/mman.h>
#include <system/graphics.h>
#include <cutils/properties.h>			/* Android get/set property */
#endif	/* ANDROID */

#include <nx_fourcc.h>

#include <nx_video_api.h>

#ifdef ANDROID
#include <nx_deinterlace.h>
#include <nx_graphictools.h>
#endif

#define FFDEC_VID_VER_MAJOR			0
#define FFDEC_VID_VER_MINOR			1
#define FFDEC_VID_VER_REVISION		0
#define FFDEC_VID_VER_NSTEP			0

#define	VPUDEC_VID_NUM_PORT			2
#define	VPUDEC_VID_INPORT_INDEX		0
#define	VPUDEC_VID_OUTPORT_INDEX	1

#define	VID_INPORT_MIN_BUF_CNT		6					/* Max 6 Avaliable */
#define	VID_INPORT_MIN_BUF_SIZE		(1024*1024*4)		/* 32 Mbps( 32Mbps, 1 fps ) */

/* Default Native Buffer Mode's buffers & buffer size */
#define	VID_OUTPORT_MIN_BUF_CNT_THUMB	4
#define	VID_OUTPORT_MIN_BUF_CNT			12				/* Max Avaiable Frames */

#define	VID_OUTPORT_MIN_BUF_CNT_H264_UNDER720P	22		/* ~720p */
#define	VID_OUTPORT_MIN_BUF_CNT_H264_1080P		12		/* 1080p */

#define	VID_OUTPORT_MIN_BUF_CNT_INTERLACE		4

#define	VID_OUTPORT_MIN_BUF_SIZE	(4*1024)			/* Video Memory Structure Size */

#define	VID_TEMP_IN_BUF_SIZE		(4*1024*1024)

#ifndef UNUSED_PARAM
#define	UNUSED_PARAM(X)		X=X
#endif

/*
 *
 * DEBUG FLAGS
 *
 */
#define	DEBUG_ANDROID	0
#define	DEBUG_BUFFER	0
#define	DEBUG_FUNC		0
#define	TRACE_ON		0
#define	DEBUG_FLUSH		0
#define	DEBUG_STATE		0
#define	DEBUG_PARAM		0

#if DEBUG_BUFFER
#define	DbgBuffer(fmt,...)	DbgMsg(fmt, ##__VA_ARGS__)
#else
#define DbgBuffer(fmt,...)	do{}while(0)
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

#if DEBUG_STATE
#define	DBG_STATE(fmt,...)		DbgMsg(fmt, ##__VA_ARGS__)
#else
#define	DBG_STATE(fmt,...)		do{}while(0)
#endif

#if DEBUG_FLUSH
#define	DBG_FLUSH(fmt,...)		DbgMsg(fmt, ##__VA_ARGS__)
#else
#define	DBG_FLUSH(fmt,...)		do{}while(0)
#endif

#if DEBUG_PARAM
#define	DBG_PARAM(fmt,...)		DbgMsg(fmt, ##__VA_ARGS__)
#else
#define	DBG_PARAM(fmt,...)		do{}while(0)
#endif

struct OutBufferTimeInfo{
	OMX_TICKS			timestamp;
	OMX_U32				flag;
};

typedef struct tNX_VIDDEC_VIDEO_COMP_TYPE NX_VIDDEC_VIDEO_COMP_TYPE;

/* Define Transform Template Component Type */
struct tNX_VIDDEC_VIDEO_COMP_TYPE{
	NX_BASECOMPONENTTYPE		/* Nexell Base Component Type */
	/* Buffer Thread */
	pthread_t					hBufThread;
	pthread_mutex_t				hBufMutex;
	NX_THREAD_CMD				eCmdBufThread;

	/* Video Format */
	OMX_VIDEO_PARAM_PORTFORMATTYPE	inputFormat;
	OMX_VIDEO_PARAM_PORTFORMATTYPE	outputFormat;

	/* Management Output Buffer */
	OMX_S32						outBufferUseFlag[NX_OMX_MAX_BUF];	/* Output Buffer Use Flag( Flag for Decoding ) */
	OMX_S32						outBufferValidFlag[NX_OMX_MAX_BUF];	/* Valid Buffer Flag */
	OMX_S32						outUsableBuffers;					/* Max Allocated Buffers or Max Usable Buffers */
	OMX_S32						outUsableBufferIdx;
	OMX_S32						curOutBuffers;						/* Currently Queued Buffer Counter */
	OMX_S32						minRequiredFrameBuffer;				/* Minimum H/W Required FrameBuffer( Sequence Output ) */
	OMX_S32						outBufferable;						/* Display Buffers */
	struct OutBufferTimeInfo	outTimeStamp[NX_OMX_MAX_BUF];		/* Output Timestamp */

	OMX_U32						outBufferAllocSize;					/* Native Buffer Mode vs ThumbnailMode */
	OMX_U32						numOutBuffers;

	OMX_BOOL					isOutIdr;

	union {
		OMX_VIDEO_PARAM_AVCTYPE avcType;
		OMX_VIDEO_PARAM_MPEG2TYPE mpeg2Type;
		OMX_VIDEO_PARAM_MPEG4TYPE mpeg4Type;
		OMX_VIDEO_PARAM_H263TYPE h263Type;
		OMX_VIDEO_PARAM_WMVTYPE wmvType;
	} codecType;

	/* for decoding */
	OMX_BOOL					bFlush;
	NX_V4L2DEC_HANDLE			hVpuCodec;
	OMX_S32						videoCodecId;
	OMX_BOOL					bInitialized;
	OMX_BOOL					bNeedKey;
	OMX_BOOL					bStartEoS;
	OMX_BOOL					frameDelay;
	OMX_U16						rvFrameCnt;

	int32_t						(*DecodeFrame)(NX_VIDDEC_VIDEO_COMP_TYPE *, NX_QUEUE *, NX_QUEUE *);

	/* Decoder Temporary Buffer */
	OMX_U8						tmpInputBuffer[VID_TEMP_IN_BUF_SIZE];

	OMX_S32						width, height;

	OMX_BOOL					bUseNativeBuffer;
	OMX_BOOL					bEnableThumbNailMode;
	OMX_BOOL					bMetaDataInBuffers;

	/* Extra Informations & */
	OMX_U8						*codecSpecificData;
	OMX_S32						codecSpecificDataSize;

	/* FFMPEG Parser Data */
	OMX_U8						*pExtraData;
	OMX_S32						nExtraDataSize;
	OMX_S32						codecTag;

	/* Native WindowBuffer */
	NX_VID_MEMORY_INFO			vidFrameBuf[MAX_DEC_FRAME_BUFFERS];	/* Video Buffer Info */
	NX_VID_MEMORY_HANDLE		hVidFrameBuf[MAX_DEC_FRAME_BUFFERS];

	/* x-ms-wmv */
	OMX_BOOL					bXMSWMVType;

	/* for Debugging */
	OMX_S32						inFrameCount;
	OMX_S32						outFrameCount;
	OMX_S32						instanceId;

	OMX_BOOL					bNeedSequenceData;

	OMX_BOOL					bInterlaced;		/* 0 : Progressive, 1 : SW_Interlaced, 2 : 3D_Interlaced */
	void						*hDeinterlace;
};

void InitVideoTimeStamp(NX_VIDDEC_VIDEO_COMP_TYPE *pDecComp);
void PushVideoTimeStamp(NX_VIDDEC_VIDEO_COMP_TYPE *pDecComp, OMX_TICKS timestamp, OMX_U32 flag );
int32_t PopVideoTimeStamp(NX_VIDDEC_VIDEO_COMP_TYPE *pDecComp, OMX_TICKS *timestamp, OMX_U32 *flag );
int32_t flushVideoCodec(NX_VIDDEC_VIDEO_COMP_TYPE *pDecComp);
int32_t openVideoCodec(NX_VIDDEC_VIDEO_COMP_TYPE *pDecComp);
void closeVideoCodec(NX_VIDDEC_VIDEO_COMP_TYPE *pDecComp);
void DeInterlaceFrame( NX_VIDDEC_VIDEO_COMP_TYPE *pDecComp, NX_V4L2DEC_OUT *pDecOut );
int32_t GetUsableBufferIdx( NX_VIDDEC_VIDEO_COMP_TYPE *pDecComp );
#ifndef ANDROID
int32_t FindBufferIdx( NX_VIDDEC_VIDEO_COMP_TYPE *pDecComp, OMX_BUFFERHEADERTYPE* pBuffer );
#endif

OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComponent);

#endif	/* __NX_OMXVideoDecoder_h__ */
