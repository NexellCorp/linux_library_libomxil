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
 *	File		: NX_AVCDecoder.c
 *	Brief		: AVC Video Decoder
 *	Author		: HyunChul Jun (hcjun@nexell.co.kr)
 *	History		: 2016.04.25 : Create
 */

#define	LOG_TAG				"NX_AVCDEC"

#include <assert.h>
#include <OMX_AndroidTypes.h>
#include "NX_OMXVideoDecoder.h"
#include "NX_DecoderUtil.h"

#ifdef TRACE_ON
#undef TRACE_ON
#define TRACE_ON 1
#endif

/* From NX_AVCUtil */
int32_t avc_get_video_size(unsigned char *buf, int32_t buf_size, int32_t *width, int32_t *height);

static int32_t AVCCheckPortReconfiguration( NX_VIDDEC_VIDEO_COMP_TYPE *pDecComp, OMX_BYTE inBuf, OMX_S32 inSize )
{
	if ( (inBuf != NULL) && (inSize > 0) )
	{
		int32_t w,h; /*	width, height, left, top, right, bottom */
		OMX_BYTE pbyStrm = inBuf;
		uint32_t uPreFourByte = (uint32_t)-1;

		do
		{
			if ( pbyStrm >= (inBuf + inSize) )		break;
			uPreFourByte = (uPreFourByte << 8) + *pbyStrm++;

			if ( uPreFourByte == 0x00000001 || uPreFourByte<<8 == 0x00000100 )
			{
				/* SPS start code */
				if ( (pbyStrm[0] & 0x1F) == 7 )
				{
					pbyStrm = ( uPreFourByte == 0x00000001 ) ? ( pbyStrm - 4 ) : ( pbyStrm - 3 );
					if( avc_get_video_size( pbyStrm, inSize - (pbyStrm - inBuf), &w, &h ) )
					{
						if( pDecComp->width != w || pDecComp->height != h )
						{
							DbgMsg("New Video Resolution = %ld x %ld --> %d x %d\n", pDecComp->width, pDecComp->height, w, h);

							/* Change Port Format & Resolution Information */
							pDecComp->pOutputPort->stdPortDef.format.video.nFrameWidth  = pDecComp->width  = w;
							pDecComp->pOutputPort->stdPortDef.format.video.nFrameHeight = pDecComp->height = h;

							/* Native Mode */
							if( pDecComp->bUseNativeBuffer )
							{
								pDecComp->pOutputPort->stdPortDef.nBufferSize = 4096;
							}
							else
							{
								pDecComp->pOutputPort->stdPortDef.nBufferSize = ((((w+15)>>4)<<4) * (((h+15)>>4)<<4))*3/2;
							}

							/* Need Port Reconfiguration */
							SendEvent( (NX_BASE_COMPNENT*)pDecComp, OMX_EventPortSettingsChanged, OMX_DirOutput, 0, NULL );
							if( OMX_TRUE == pDecComp->bInitialized )
							{
								pDecComp->bInitialized = OMX_FALSE;
								InitVideoTimeStamp(pDecComp);
								closeVideoCodec(pDecComp);
								openVideoCodec(pDecComp);
							}
							pDecComp->pOutputPort->stdPortDef.bEnabled = OMX_FALSE;
						}
						else
						{
							DbgMsg("Video Resolution = %ld x %ld --> %d x %d\n", pDecComp->width, pDecComp->height, w, h);
						}
						return 1;
					}
					break;
				}
			}
		} while(1);
	}

	return 0;
}

int32_t NX_DecodeAvcFrame(NX_VIDDEC_VIDEO_COMP_TYPE *pDecComp, NX_QUEUE *pInQueue, NX_QUEUE *pOutQueue)
{
	OMX_BUFFERHEADERTYPE* pInBuf = NULL, *pOutBuf = NULL;
	int32_t inSize = 0;
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

	/* Get Next Queue Information */
	NX_PopQueue( pInQueue, (void**)&pInBuf );
	if( pInBuf == NULL )
	{
		return 0;
	}

	inData = pInBuf->pBuffer;
	inSize = pInBuf->nFilledLen;
	pDecComp->inFrameCount++;

	TRACE("pInBuf->nFlags = 0x%08x, size = %ld\n", (int32_t)pInBuf->nFlags, pInBuf->nFilledLen );

	/* Check End Of Stream */
	if( pInBuf->nFlags & OMX_BUFFERFLAG_EOS )
	{
		DbgMsg("=========================> Receive Endof Stream Message (%ld)\n", pInBuf->nFilledLen);
		pDecComp->bStartEoS = OMX_TRUE;
		if( inSize <= 0)
		{
			goto Exit;
		}
	}

	/* Step 1. Found Sequence Information */
	if( OMX_TRUE == pDecComp->bNeedSequenceData && pInBuf->nFlags & OMX_BUFFERFLAG_CODECCONFIG )
	{
		if( pInBuf->nFlags & OMX_BUFFERFLAG_CODECCONFIG )
		{
			pDecComp->bNeedSequenceData = OMX_FALSE;
			DbgMsg("Copy Extra Data (%d)\n", inSize );
			AVCCheckPortReconfiguration( pDecComp, inData, inSize );
			if( pDecComp->codecSpecificData )
				free( pDecComp->codecSpecificData );
			pDecComp->codecSpecificData = malloc(inSize);
			memcpy( pDecComp->codecSpecificData + pDecComp->codecSpecificDataSize, inData, inSize );
			pDecComp->codecSpecificDataSize += inSize;

			if( ( inSize>4 && inData[0]==0 && inData[1]==0 && inData[2]==0 && inData[3]==1 && ((inData[4]&0x0F)==0x07) ) ||
				( inSize>4 && inData[0]==0 && inData[1]==0 && inData[2]==1 && ((inData[3]&0x0F)==0x07) ) )
			{
				int32_t w,h; /* width, height, left, top, right, bottom */
				if( avc_get_video_size( pDecComp->codecSpecificData, pDecComp->codecSpecificDataSize, &w, &h ) )
				{
					if( pDecComp->width != w || pDecComp->height != h )
					{
						/* Need Port Reconfiguration */
						SendEvent( (NX_BASE_COMPNENT*)pDecComp, OMX_EventPortSettingsChanged, OMX_DirOutput, 0, NULL );

						/* Change Port Format */
						pDecComp->pOutputPort->stdPortDef.format.video.nFrameWidth = w;
						pDecComp->pOutputPort->stdPortDef.format.video.nFrameHeight = h;

						/* Native Mode */
						if( pDecComp->bUseNativeBuffer )
						{
							pDecComp->pOutputPort->stdPortDef.nBufferSize = 4096;
						}
						else
						{
							pDecComp->pOutputPort->stdPortDef.nBufferSize = ((((w+15)>>4)<<4) * (((h+15)>>4)<<4))*3/2;
						}
						goto Exit;
					}
				}
			}
			goto Exit;
		}
	}

	// {
	// 	OMX_U8 *buf = pInBuf->pBuffer;
	// 	DbgMsg("nFilledLen(%7d),TimeStamp(%lld),Flags(0x%08x), Data: 0x%02x%02x%02x%02x, 0x%02x%02x%02x%02x, 0x%02x%02x%02x%02x, 0x%02x%02x%02x%02x, 0x%02x%02x%02x%02x, 0x%02x%02x%02x%02x\n",
	// 		pInBuf->nFilledLen, pInBuf->nTimeStamp, pInBuf->nFlags,
	// 		buf[ 0],buf[ 1],buf[ 2],buf[ 3],buf[ 4],buf[ 5],buf[ 6],buf[ 7],
	// 		buf[ 8],buf[ 9],buf[10],buf[11],buf[12],buf[13],buf[14],buf[15],
	// 		buf[16],buf[17],buf[18],buf[19],buf[20],buf[21],buf[22],buf[23] );
	// }

	/* Push Input Time Stamp */
	if( !(pInBuf->nFlags & OMX_BUFFERFLAG_CODECCONFIG) )
		PushVideoTimeStamp(pDecComp, pInBuf->nTimeStamp, pInBuf->nFlags );

	/* Step 2. Find First Key Frame & Do Initialize VPU */
	if( OMX_FALSE == pDecComp->bInitialized )
	{
		int32_t initBufSize;
		unsigned char *initBuf;

		if( pDecComp->codecSpecificDataSize == 0 && pDecComp->nExtraDataSize>0 )
		{
			initBufSize = inSize + pDecComp->nExtraDataSize;
			initBuf = (unsigned char *)malloc( initBufSize );
			memcpy( initBuf, pDecComp->pExtraData, pDecComp->nExtraDataSize );
			memcpy( initBuf + pDecComp->nExtraDataSize, inData, inSize );
		}
		else
		{
			initBufSize = inSize + pDecComp->codecSpecificDataSize;
			initBuf = (unsigned char*)malloc(initBufSize);
			memcpy( initBuf, pDecComp->codecSpecificData, pDecComp->codecSpecificDataSize );
			memcpy( initBuf + pDecComp->codecSpecificDataSize, inData, inSize );
		}

		if( OMX_TRUE == pDecComp->bNeedSequenceData )
		{
			if( AVCCheckPortReconfiguration( pDecComp, initBuf, initBufSize ) )
			{
				pDecComp->bNeedSequenceData = OMX_FALSE;
				if( pDecComp->codecSpecificData )
					free( pDecComp->codecSpecificData );
				pDecComp->codecSpecificData = malloc(initBufSize);
				memcpy( pDecComp->codecSpecificData, initBuf, initBufSize );
				pDecComp->codecSpecificDataSize = initBufSize;
				goto Exit;
			}
			else
			{
				goto Exit;
			}
		}

		/* Initialize VPU */
		ret = InitializeCodaVpu(pDecComp, initBuf, initBufSize );
		free( initBuf );

		if( 0 > ret )
		{
			ErrMsg("VPU initialized Failed!!!!\n");
			goto Exit;
		}
		else if( ret > 0  )
		{
			ret = 0;
			goto Exit;
		}

		pDecComp->bNeedKey = OMX_FALSE;
		pDecComp->bInitialized = OMX_TRUE;

		decIn.strmBuf = inData;
		//decIn.strmSize = 0;
		decIn.strmSize = initBufSize;
		decIn.timeStamp = pInBuf->nTimeStamp;
		decIn.eos = 0;
		ret = NX_V4l2DecDecodeFrame( pDecComp->hVpuCodec, &decIn, &decOut );
	}
	else
	{
		decIn.strmBuf = inData;
		decIn.strmSize = inSize;
		decIn.timeStamp = pInBuf->nTimeStamp;
		decIn.eos = 0;
		ret = NX_V4l2DecDecodeFrame( pDecComp->hVpuCodec, &decIn, &decOut );
	}

	TRACE("decOut : outImgIdx(%d) decIdx(%d) \n", decOut.outImgIdx, decOut.outDecIdx );
	TRACE("Output Buffer : ColorFormat(0x%08x), NatvieBuffer(%d), Thumbnail(%d), MetaDataInBuffer(%d), ret(%d)\n",
			pDecComp->outputFormat.eColorFormat, pDecComp->bUseNativeBuffer, pDecComp->bEnableThumbNailMode, pDecComp->bMetaDataInBuffers, ret );

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
			goto Exit;
		}
		else
		{
			int32_t OutIdx = ( pDecComp->bInterlaced == 0 ) ? ( decOut.outImgIdx ) : ( GetUsableBufferIdx(pDecComp) );
			// if( pDecComp->isOutIdr == OMX_FALSE && decOut.picType[DECODED_FRAME] != PIC_TYPE_I )
			// {
			// 	OMX_TICKS timestamp;
			// 	OMX_U32 flag;
			// 	PopVideoTimeStamp(pDecComp, &timestamp, &flag );
			// 	NX_V4l2DecClrDspFlag( pDecComp->hVpuCodec, NULL, decOut.outImgIdx );
			// 	goto Exit;
			// }
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
				ErrMsg("Unexpected Buffer Handling!!!! Goto Exit(%ld,%d)\n", pDecComp->curOutBuffers, decOut.outImgIdx);
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

			DeInterlaceFrame( pDecComp, &decOut );
			pDecComp->outFrameCount++;
			pDecComp->pCallbacks->FillBufferDone(pDecComp->hComp, pDecComp->hComp->pApplicationPrivate, pOutBuf);
		}
	}

Exit:
	pInBuf->nFilledLen = 0;
	pDecComp->pCallbacks->EmptyBufferDone(pDecComp->hComp, pDecComp->hComp->pApplicationPrivate, pInBuf);

	return ret;
}
