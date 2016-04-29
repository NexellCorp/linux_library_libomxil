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
 *	File		: NX_DecoderUtil.h
 *	Brief		: Video Decoder Util
 *	Author		: HyunChul Jun (hcjun@nexell.co.kr)
 *	History		: 2016.04.25 : Create
 */

#ifndef __NX_DecodeUtil_h__
#define __NX_DecodeUtil_h__

#include "NX_OMXVideoDecoder.h"

#ifndef MKTAG
#define MKTAG(a,b,c,d) (a | (b << 8) | (c << 16) | (d << 24))
#endif

#define PUT_LE32(_p, _var) \
	*_p++ = (unsigned char)((_var)>>0);  \
	*_p++ = (unsigned char)((_var)>>8);  \
	*_p++ = (unsigned char)((_var)>>16); \
	*_p++ = (unsigned char)((_var)>>24);

#define PUT_BE32(_p, _var) \
	*_p++ = (unsigned char)((_var)>>24);  \
	*_p++ = (unsigned char)((_var)>>16);  \
	*_p++ = (unsigned char)((_var)>>8); \
	*_p++ = (unsigned char)((_var)>>0);

#define PUT_LE16(_p, _var) \
	*_p++ = (unsigned char)((_var)>>0);  \
	*_p++ = (unsigned char)((_var)>>8);


#define PUT_BE16(_p, _var) \
	*_p++ = (unsigned char)((_var)>>8);  \
	*_p++ = (unsigned char)((_var)>>0);


/* Profiles & Levels */
#define MAX_DEC_SUPPORTED_AVC_PROFILES		3
#define MAX_DEC_SUPPORTED_AVC_LEVELS		14

#define	MAX_DEC_SUPPORTED_MPEG4_PROFILES	2
#define	MAX_DEC_SUPPORTED_MPEG4_LEVELS		8

#define MAX_DEC_SUPPORTED_MPEG2_PROFILES	3
#define	MAX_DEC_SUPPORTED_MPEG2_LEVELS		3

/* add in H.263 by kshblue (14.07.04) */
#define	MAX_DEC_SUPPORTED_H263_PROFILES		2
#define	MAX_DEC_SUPPORTED_H263_LEVELS		8

extern const OMX_VIDEO_AVCPROFILETYPE gstDecSupportedAVCProfiles[MAX_DEC_SUPPORTED_AVC_PROFILES];
extern const OMX_VIDEO_AVCLEVELTYPE gstDecSupportedAVCLevels[MAX_DEC_SUPPORTED_AVC_LEVELS];

extern const OMX_VIDEO_MPEG4PROFILETYPE gstDecSupportedMPEG4Profiles[MAX_DEC_SUPPORTED_MPEG4_PROFILES];
extern const OMX_VIDEO_MPEG4LEVELTYPE gstDecSupportedMPEG4Levels[MAX_DEC_SUPPORTED_MPEG4_LEVELS];

extern const OMX_VIDEO_H263PROFILETYPE gstDecSupportedH263Profiles[MAX_DEC_SUPPORTED_H263_PROFILES];
extern const OMX_VIDEO_H263LEVELTYPE gstDecSupportedH263Levels[MAX_DEC_SUPPORTED_H263_LEVELS];

#ifdef __cplusplus
extern "C"{
#endif

/* each Codec's NX_XXXX_Decoder.c */
int32_t NX_DecodeAvcFrame(NX_VIDDEC_VIDEO_COMP_TYPE *pDecComp, NX_QUEUE *pInQueue, NX_QUEUE *pOutQueue);
int32_t NX_DecodeMpeg4Frame(NX_VIDDEC_VIDEO_COMP_TYPE *pDecComp, NX_QUEUE *pInQueue, NX_QUEUE *pOutQueue);
int32_t NX_DecodeMpeg2Frame(NX_VIDDEC_VIDEO_COMP_TYPE *pDecComp, NX_QUEUE *pInQueue, NX_QUEUE *pOutQueue);
int32_t NX_DecodeDiv3Frame(NX_VIDDEC_VIDEO_COMP_TYPE *pDecComp, NX_QUEUE *pInQueue, NX_QUEUE *pOutQueue);
int32_t NX_DecodeRVFrame(NX_VIDDEC_VIDEO_COMP_TYPE *pDecComp, NX_QUEUE *pInQueue, NX_QUEUE *pOutQueue);
int32_t NX_DecodeVC1Frame(NX_VIDDEC_VIDEO_COMP_TYPE *pDecComp, NX_QUEUE *pInQueue, NX_QUEUE *pOutQueue);
int32_t NX_DecodeVP8Frame(NX_VIDDEC_VIDEO_COMP_TYPE *pDecComp, NX_QUEUE *pInQueue, NX_QUEUE *pOutQueue);

/* AVC Max Frame Buffer Size Tools */
int32_t AVCFindMinimumBufferSize(OMX_VIDEO_AVCLEVELTYPE level, int width, int height);

/* NX_OMX_VideoDecoder.c */
int32_t InitializeCodaVpu(NX_VIDDEC_VIDEO_COMP_TYPE *pDecComp, unsigned char *buf, int size);

/* Copy Image YV12 to General YV12 */
int32_t CopyImageToBufferYV12( uint8_t *srcY, uint8_t *srcU, uint8_t *srcV, uint8_t *dst, uint32_t strideY, uint32_t strideUV, uint32_t width, uint32_t height );

#ifdef __cplusplus
}
#endif

#endif	/* __DecodeFrame_h__ */
