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
 *	File		: NX_VP8Decoder.c
 *	Brief		: VP8 Video Decoder
 *	Author		: HyunChul Jun (hcjun@nexell.co.kr)
 *	History		: 2016.04.25 : Create
 */

#define	LOG_TAG				"NX_VP8DEC"

#include <assert.h>
#include <OMX_AndroidTypes.h>

#include "NX_OMXVideoDecoder.h"
#include "NX_DecoderUtil.h"

static int32_t MakeVP8DecoderSpecificInfo( NX_VIDDEC_VIDEO_COMP_TYPE *pDecComp )
{
	int32_t retSize = 0;
	OMX_U8 *pbHeader = pDecComp->tmpInputBuffer;

	PUT_LE32(pbHeader, MKTAG('D', 'K', 'I', 'F'));		/* signature 'DKIF' */
	PUT_LE16(pbHeader, 0x00);							/* version */
	PUT_LE16(pbHeader, 0x20);							/* length of header in bytes */
	PUT_LE32(pbHeader, MKTAG('V', 'P', '8', '0'));		/* codec FourCC */
	PUT_LE16(pbHeader, pDecComp->width);				/* width */
	PUT_LE16(pbHeader, pDecComp->height);				/* height */
	PUT_LE32(pbHeader, 0);								/* frame rate */
	PUT_LE32(pbHeader, 0);								/* time scale(?) */
	PUT_LE32(pbHeader, 1);								/* number of frames in file */
	PUT_LE32(pbHeader, 0);								/* unused */
	retSize += 32;
	return 	retSize;
}

static int32_t MakeVP8Stream( OMX_U8 *pIn, OMX_S32 inSize, OMX_U8 *pOut )
{
	PUT_LE32(pOut, inSize);								/* frame_chunk_len */
	PUT_LE32(pOut, 0);									/* time stamp */
	PUT_LE32(pOut, 0);
	memcpy( pOut, pIn, inSize );

	return ( inSize + 12 );
}

int32_t NX_DecodeVP8Frame(NX_VIDDEC_VIDEO_COMP_TYPE *pDecComp, NX_QUEUE *pInQueue, NX_QUEUE *pOutQueue)
{
	OMX_BUFFERHEADERTYPE* pInBuf = NULL, *pOutBuf = NULL;
	int32_t inSize = 0;
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

	/* Get Next Queue Information */
	NX_PopQueue( pInQueue, (void**)&pInBuf );
	if( pInBuf == NULL )
	{
		return 0;
	}

	inData = pInBuf->pBuffer;
	inSize = pInBuf->nFilledLen;
	pDecComp->inFrameCount++;

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

	/* Push Input Time Stamp */
	TRACE("pInBuf->nTimeStamp = %lld, pInBuf->nFilledLen = %d\n", pInBuf->nTimeStamp/1000, pInBuf->nFilledLen);
	PushVideoTimeStamp(pDecComp, pInBuf->nTimeStamp, pInBuf->nFlags );

	/* Step 2. Find First Key Frame & Do Initialize VPU */
	if( OMX_FALSE == pDecComp->bInitialized )
	{
		OMX_S32 size;
		size = MakeVP8DecoderSpecificInfo( pDecComp );
		size += MakeVP8Stream( inData, inSize, pDecComp->tmpInputBuffer + size );

		/* Initialize VPU */
		ret = InitializeCodaVpu(pDecComp, pDecComp->tmpInputBuffer, size );
		if( 0 != ret )
		{
			ErrMsg("VPU initialized Failed!!!!\n");
			goto Exit;
		}

		pDecComp->bNeedKey = OMX_FALSE;
		pDecComp->bInitialized = OMX_TRUE;

		//decIn.strmBuf = inData;
		//decIn.strmSize = 0;
		//decIn.timeStamp = pInBuf->nTimeStamp;
		//decIn.eos = 0;
		//ret = NX_V4l2DecDecodeFrame( pDecComp->hVpuCodec, &decIn, &decOut );
		decOut.outImgIdx = -1;
	}
	else
	{
		inSize = MakeVP8Stream( inData, inSize, pDecComp->tmpInputBuffer );
		decIn.strmBuf = pDecComp->tmpInputBuffer;
		decIn.strmSize = inSize;
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
#if 0
			if( pDecComp->isOutIdr == OMX_FALSE && decOut.picType[DECODED_FRAME] != PIC_TYPE_I )
			{
				NX_V4l2DecClrDspFlag( pDecComp->hVpuCodec, NULL, decOut.outImgIdx );
				goto Exit;
			}

#endif
			pDecComp->isOutIdr = OMX_TRUE;
#ifdef ANDROID
			/* Native Window Buffer Mode */
			/* Get Output Buffer Pointer From Output Buffer Pool */
			pOutBuf = pDecComp->pOutputBuffers[decOut.outImgIdx];

			if( pDecComp->outBufferUseFlag[decOut.outImgIdx] == 0 )
			{
				OMX_TICKS timestamp;
				OMX_U32 flag;
				PopVideoTimeStamp(pDecComp, &timestamp, &flag );
				NX_V4l2DecClrDspFlag( pDecComp->hVpuCodec, NULL, decOut.outImgIdx );
				ErrMsg("Unexpected Buffer Handling!!!! Goto Exit\n");
				goto Exit;
			}
			pDecComp->outBufferValidFlag[OutIdx] = 1;
			pOutBuf->nFilledLen = sizeof(struct private_handle_t);			
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
			TRACE("pOutBuf->nTimeStamp = %lld\n", pOutBuf->nTimeStamp/1000);
			pDecComp->outFrameCount++;
			pDecComp->pCallbacks->FillBufferDone(pDecComp->hComp, pDecComp->hComp->pApplicationPrivate, pOutBuf);
		}
	}

Exit:
	pInBuf->nFilledLen = 0;
	pDecComp->pCallbacks->EmptyBufferDone(pDecComp->hComp, pDecComp->hComp->pApplicationPrivate, pInBuf);

	return ret;
}
