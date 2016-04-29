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
 *	File		: NX_Div3Decoder.c
 *	Brief		: Div3 Video Decoder
 *	Author		: HyunChul Jun (hcjun@nexell.co.kr)
 *	History		: 2016.04.25 : Create
 */

#define	LOG_TAG				"NX_DIV3DEC"

#include <assert.h>
#include <OMX_AndroidTypes.h>

#include "NX_OMXVideoDecoder.h"
#include "NX_DecoderUtil.h"

static int32_t MakeDiv3DecodeSpecificInfo( NX_VIDDEC_VIDEO_COMP_TYPE *pDecComp, OMX_U8 *pIn, OMX_S32 inSize, OMX_U8 *pOut )
{
	unsigned char *pbHeader = pOut;
	int32_t nMetaData = inSize;
	unsigned char *pbMetaData = pIn;
	int32_t retSize = 0;
	if( !nMetaData )
	{
		PUT_LE32(pbHeader, MKTAG('C', 'N', 'M', 'V'));	/* signature 'CNMV' */
		PUT_LE16(pbHeader, 0x00);						/* version */
		PUT_LE16(pbHeader, 0x20);						/* length of header in bytes */
		PUT_LE32(pbHeader, MKTAG('D', 'I', 'V', '3'));	/* codec FourCC */
		PUT_LE16(pbHeader, pDecComp->width);			/* width */
		PUT_LE16(pbHeader, pDecComp->height);			/* height */
		PUT_LE32(pbHeader, 0);							/* frame rate */
		PUT_LE32(pbHeader, 0);							/* time scale(?) */
		PUT_LE32(pbHeader, 1);							/* number of frames in file */
		PUT_LE32(pbHeader, 0);							/* unused */
		retSize += 32;
	}
	else
	{
		PUT_BE32(pbHeader, nMetaData);
		retSize += 4;
		memcpy(pbHeader, pbMetaData, nMetaData);
		retSize += nMetaData;
	}
	return retSize;
}

static int32_t MakeDiv3Stream( OMX_U8 *pIn, OMX_S32 inSize, OMX_U8 *pOut )
{
	PUT_LE32(pOut,inSize);
	PUT_LE32(pOut,0);
	PUT_LE32(pOut,0);
	memcpy( pOut, pIn, inSize );

	return (inSize + 12);
}

int32_t NX_DecodeDiv3Frame(NX_VIDDEC_VIDEO_COMP_TYPE *pDecComp, NX_QUEUE *pInQueue, NX_QUEUE *pOutQueue)
{
	OMX_BUFFERHEADERTYPE* pInBuf = NULL, *pOutBuf = NULL;
	int inSize = 0;
	OMX_BYTE inData;
	NX_V4L2DEC_IN decIn;
	NX_V4L2DEC_OUT decOut;
	int32_t ret = 0;

	UNUSED_PARAM(pOutQueue);

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

	TRACE("pInBuf->nFlags = 0x%08x, size = %ld\n", (int32_t)pInBuf->nFlags, pInBuf->nFilledLen );

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
	if( !(pInBuf->nFlags & OMX_BUFFERFLAG_CODECCONFIG) )
		PushVideoTimeStamp(pDecComp, pInBuf->nTimeStamp, pInBuf->nFlags );

	/* Step 2. Find First Key Frame & Do Initialize VPU */
	if( OMX_FALSE == pDecComp->bInitialized )
	{
		OMX_S32 size;
		size = MakeDiv3DecodeSpecificInfo( pDecComp, pDecComp->pExtraData, pDecComp->nExtraDataSize, pDecComp->tmpInputBuffer );
		size += MakeDiv3Stream( inData, inSize, pDecComp->tmpInputBuffer + size );
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
		inSize = MakeDiv3Stream( inData, inSize, pDecComp->tmpInputBuffer );
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
			if( pDecComp->isOutIdr == OMX_FALSE && decOut.picType[DECODED_FRAME] != PIC_TYPE_I )
			{
				OMX_TICKS timestamp;
				OMX_U32 flag;
				PopVideoTimeStamp(pDecComp, &timestamp, &flag );
				NX_V4l2DecClrDspFlag( pDecComp->hVpuCodec, NULL, decOut.outImgIdx );
				goto Exit;
			}
			pDecComp->isOutIdr = OMX_TRUE;
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
			TRACE("nTimeStamp = %lld\n", pOutBuf->nTimeStamp/1000);
			pDecComp->outFrameCount++;
			pDecComp->pCallbacks->FillBufferDone(pDecComp->hComp, pDecComp->hComp->pApplicationPrivate, pOutBuf);
		}
	}

Exit:
	pInBuf->nFilledLen = 0;
	pDecComp->pCallbacks->EmptyBufferDone(pDecComp->hComp, pDecComp->hComp->pApplicationPrivate, pInBuf);

	return 0;
}
