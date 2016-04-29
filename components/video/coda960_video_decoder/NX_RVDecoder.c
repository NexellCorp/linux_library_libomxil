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
 *	File		: NX_RVDecoder.c
 *	Brief		: RV Video Decoder
 *	Author		: HyunChul Jun (hcjun@nexell.co.kr)
 *	History		: 2016.04.25 : Create
 */

#define	LOG_TAG				"NX_RVDEC"

#include <assert.h>
#include <OMX_AndroidTypes.h>

#include "NX_OMXVideoDecoder.h"
#include "NX_DecoderUtil.h"

static int32_t MakeRVDecodeSpecificInfo( NX_VIDDEC_VIDEO_COMP_TYPE *pDecComp )
{
	OMX_S32 size;
	OMX_S32 fourcc;
	OMX_S32 width  = pDecComp->width;
	OMX_S32 height = pDecComp->height;
	OMX_U8 *pData = pDecComp->codecSpecificData;
	OMX_S32 frameRate = 30;

	pDecComp->codecSpecificDataSize = 0;
	fourcc = MKTAG('R','V','4','0');
	size = 26 + pDecComp->nExtraDataSize;
	PUT_BE32(pData, size); /* Length */
	PUT_LE32(pData, MKTAG('V', 'I', 'D', 'O')); /* MOFTag */
	PUT_LE32(pData, fourcc); /* SubMOFTagl */
	PUT_BE16(pData, width);
	PUT_BE16(pData, height);
	PUT_BE16(pData, 0x0c); /* BitCount */
	PUT_BE16(pData, 0x00); /* PadWidth */
	PUT_BE16(pData, 0x00); /* PadHeight */
	PUT_LE32(pData, frameRate);

	memcpy(pData, pDecComp->pExtraData, pDecComp->nExtraDataSize); /* OpaqueDatata */
	pDecComp->codecSpecificDataSize = size;

	return size;
}

static int32_t MakeRVPacketData( OMX_U8 *pIn, OMX_S32 inSize, OMX_U8 *pOut, OMX_U16 frameCnt )
{
	OMX_U8 *p = pIn;
	OMX_S32 cSlice, nSlice;
	OMX_S32 i, val, offset;
	OMX_S32 size;

	frameCnt = frameCnt;

	cSlice = p[0] + 1;
	nSlice =  inSize - 1 - (cSlice * 8);
	size = 20 + (cSlice*8);

	PUT_BE32(pOut, nSlice);
	PUT_LE32(pOut, 0);
	PUT_BE16(pOut, 0);
	PUT_BE16(pOut, 0x02);	/* Flags */
	PUT_BE32(pOut, 0x00);	/* LastPacket */
	PUT_BE32(pOut, cSlice);	/* NumSegments */
	offset = 1;
	for (i = 0; i < (int32_t) cSlice; i++)
	{
		val = (p[offset+3] << 24) | (p[offset+2] << 16) | (p[offset+1] << 8) | p[offset];
		PUT_BE32(pOut, val); /* isValid */
		offset += 4;
		val = (p[offset+3] << 24) | (p[offset+2] << 16) | (p[offset+1] << 8) | p[offset];
		PUT_BE32(pOut, val); /* Offset */
		offset += 4;
	}

	memcpy(pOut, pIn+(1+(cSlice*8)), nSlice);
	size += nSlice;
	return size;
}

int32_t NX_DecodeRVFrame(NX_VIDDEC_VIDEO_COMP_TYPE *pDecComp, NX_QUEUE *pInQueue, NX_QUEUE *pOutQueue)
{
	OMX_BUFFERHEADERTYPE* pInBuf = NULL, *pOutBuf = NULL;
	int32_t inSize = 0, rcSize=0;
	OMX_BYTE inData;
	NX_V4L2DEC_IN decIn;
	NX_V4L2DEC_OUT decOut;
	int32_t ret = 0;

	UNUSED_PARAM(pOutQueue);

	memset(&decIn,  0, sizeof(decIn)  );

	if( pDecComp->bFlush )
	{
		flushVideoCodec( pDecComp );
		pDecComp->bFlush = OMX_FALSE;
	}

	NX_PopQueue( pInQueue, (void**)&pInBuf );
	if( pInBuf == NULL )
	{
		return 0;
	}

	inData = pInBuf->pBuffer;
	inSize = pInBuf->nFilledLen;
	pDecComp->inFrameCount++;

	TRACE("pInBuf->nFlags = 0x%08x, size = %ld, timeStamp = %lld\n", (int32_t)pInBuf->nFlags, pInBuf->nFilledLen, pInBuf->nTimeStamp );

	if( pInBuf->nFlags & OMX_BUFFERFLAG_EOS )
	{
		DbgMsg("=========================> Receive Endof Stream Message (%ld)\n", pInBuf->nFilledLen);

		pDecComp->bStartEoS = OMX_TRUE;
		if( inSize <= 0)
		{
			pInBuf->nFilledLen = 0;
			pDecComp->pCallbacks->EmptyBufferDone(pDecComp->hComp, pDecComp->hComp->pApplicationPrivate, pInBuf);
			return 0;
		}
	}

	/* Step 1. Found Sequence Information */
	if( OMX_FALSE == pDecComp->bInitialized )
	{
		if( pDecComp->codecSpecificData )
			free(pDecComp->codecSpecificData);
		/* RV Sequence Need Addtional 26 bytes but we allocate 128 bytes additionally. */
		pDecComp->codecSpecificData = malloc(pDecComp->nExtraDataSize + 128);
		MakeRVDecodeSpecificInfo( pDecComp );
	}

	//{
	//	OMX_U8 *buf = pInBuf->pBuffer;
	//	DbgMsg("pInBuf->nFlags(%7d) = 0x%08x, 0x%02x%02x%02x%02x, 0x%02x%02x%02x%02x, 0x%02x%02x%02x%02x, 0x%02x%02x%02x%02x, 0x%02x%02x%02x%02x, 0x%02x%02x%02x%02x\n", pInBuf->nFilledLen, pInBuf->nFlags,
	//		buf[ 0],buf[ 1],buf[ 2],buf[ 3],buf[ 4],buf[ 5],buf[ 6],buf[ 7],
	//		buf[ 8],buf[ 9],buf[10],buf[11],buf[12],buf[13],buf[14],buf[15],
	//		buf[16],buf[17],buf[18],buf[19],buf[20],buf[21],buf[22],buf[23] );
	//}

	/* Push Input Time Stamp */
	PushVideoTimeStamp(pDecComp, pInBuf->nTimeStamp, pInBuf->nFlags );

	/* Step 2. Find First Key Frame & Do Initialize VPU */
	if( OMX_FALSE == pDecComp->bInitialized )
	{
		int32_t size = pDecComp->codecSpecificDataSize;
		memcpy( pDecComp->tmpInputBuffer, pDecComp->codecSpecificData, pDecComp->codecSpecificDataSize );
		size += MakeRVPacketData( inData, inSize, pDecComp->tmpInputBuffer+size, pDecComp->rvFrameCnt );

		/* Initialize VPU */
		ret = InitializeCodaVpu(pDecComp, pDecComp->tmpInputBuffer, size );
		if( 0 != ret )
		{
			ErrMsg("VPU initialized Failed!!!!\n");
			goto Exit;
		}

		pDecComp->bNeedKey = OMX_FALSE;
		pDecComp->bInitialized = OMX_TRUE;

		//decIn.strmBuf = pDecComp->tmpInputBuffer;
		//decIn.strmSize = 0;
		//decIn.timeStamp = pInBuf->nTimeStamp;
		//decIn.eos = 0;
		//ret = NX_V4l2DecDecodeFrame( pDecComp->hVpuCodec, &decIn, &decOut );
		decOut.outImgIdx = -1;
	}
	else
	{
		rcSize = MakeRVPacketData( inData, inSize, pDecComp->tmpInputBuffer, pDecComp->rvFrameCnt++ );
		decIn.strmBuf = pDecComp->tmpInputBuffer;
		decIn.strmSize = rcSize;
		decIn.timeStamp = pInBuf->nTimeStamp;
		decIn.eos = 0;
		ret = NX_V4l2DecDecodeFrame( pDecComp->hVpuCodec, &decIn, &decOut );
	}
	TRACE("decOut : decIdx = %d, dispIdx = %d \n", decOut.outDecIdx, decOut.outImgIdx );

	if( ret==VID_ERR_NONE && decOut.outImgIdx >= 0 && ( decOut.outImgIdx < NX_OMX_MAX_BUF ) )
	{
		NX_VID_MEMORY_INFO *pImg = NULL;
		if( OMX_TRUE == pDecComp->bEnableThumbNailMode )
		{
			/* Thumbnail Mode */
			pImg = &decOut.outImg;
			NX_PopQueue( pOutQueue, (void**)&pOutBuf );
			CopyImageToBufferYV12( (OMX_U8*)pImg->pBuffer[0], (OMX_U8*)pImg->pBuffer[1], (OMX_U8*)pImg->pBuffer[2],
				pOutBuf->pBuffer, pImg->stride[0], pImg->stride[1], pDecComp->width, pDecComp->height );

			NX_V4l2DecClrDspFlag( pDecComp->hVpuCodec, NULL, decOut.outImgIdx );
			pOutBuf->nFilledLen = pDecComp->width * pDecComp->height * 3 / 2;
			if( 0 != PopVideoTimeStamp(pDecComp, &pOutBuf->nTimeStamp, &pOutBuf->nFlags )  )
			{
				pOutBuf->nTimeStamp = pInBuf->nTimeStamp;
				pOutBuf->nFlags     = pInBuf->nFlags;
			}
			DbgMsg("ThumbNail Mode : pOutBuf->nAllocLen = %ld, pOutBuf->nFilledLen = %ld\n", pOutBuf->nAllocLen, pOutBuf->nFilledLen );
			pDecComp->outFrameCount++;
			pDecComp->pCallbacks->FillBufferDone(pDecComp->hComp, pDecComp->hComp->pApplicationPrivate, pOutBuf);
		}
		else
		{
			int32_t OutIdx = decOut.outImgIdx;
#ifdef ANDROID			
			/* Native Window Buffer Mode */
			/* Get Output Buffer Pointer From Output Buffer Pool */
			pOutBuf = pDecComp->pOutputBuffers[OutIdx];

			if( pDecComp->outBufferUseFlag[OutIdx] == 0 )
			{
				OMX_TICKS timestamp;
				OMX_U32 flag;
				PopVideoTimeStamp(pDecComp, &timestamp, &flag );
				NX_V4l2DecClrDspFlag( pDecComp->hVpuCodec, NULL, decOut.outImgIdx );
				ErrMsg("Unexpected Buffer Handling!!!! Goto Exit\n");
				goto Exit;
			}
#else
			if( 0 != NX_PopQueue( pOutQueue, (void**)&pOutBuf ) )
			{
				OMX_TICKS timestamp;
				OMX_U32 flag;
				PopVideoTimeStamp(pDecComp, &timestamp, &flag );
				NX_V4l2DecClrDspFlag( pDecComp->hVpuCodec, NULL, decOut.outImgIdx );
				ErrMsg("Unexpected Buffer Handling!!!! Goto Exit(%ld,%d)\n", pDecComp->curOutBuffers, decOut.outImgIdx);
				goto Exit;
			}
			OutIdx = FindBufferIdx(pDecComp, pOutBuf);
			pDecComp->outBufferValidFlag[OutIdx] = decOut.outImgIdx + 1;
			pOutBuf->nFilledLen = pDecComp->width * pDecComp->height * 3 / 2;
			pImg = &decOut.outImg;
			CopyImageToBufferYV12( (OMX_U8*)pImg->pBuffer[0], (OMX_U8*)pImg->pBuffer[1], (OMX_U8*)pImg->pBuffer[2],
				pOutBuf->pBuffer, pImg->stride[0], pImg->stride[1], pDecComp->width, pDecComp->height );
#endif
			DbgBuffer("curOutBuffers(%ld),idx(%d)\n", pDecComp->curOutBuffers, decOut.outImgIdx);			

			pDecComp->outBufferUseFlag[OutIdx] = 0;
			pDecComp->curOutBuffers --;

			if( 0 != PopVideoTimeStamp(pDecComp, &pOutBuf->nTimeStamp, &pOutBuf->nFlags )  )
			{
				pOutBuf->nTimeStamp = pInBuf->nTimeStamp;
				pOutBuf->nFlags     = pInBuf->nFlags;
			}
			TRACE("nTimeStamp = %lld\n", pOutBuf->nTimeStamp/1000);
			pDecComp->outFrameCount++;
			pDecComp->pCallbacks->FillBufferDone(pDecComp->hComp, pDecComp->hComp->pApplicationPrivate, pOutBuf);
		}
	}

Exit:
	pInBuf->nFilledLen = 0;
	pDecComp->pCallbacks->EmptyBufferDone(pDecComp->hComp, pDecComp->hComp->pApplicationPrivate, pInBuf);

	return ret;
}
