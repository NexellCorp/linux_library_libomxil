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
 *	File		: NX_DecoderUtil.c
 *	Brief		: Video Decoder Util
 *	Author		: HyunChul Jun (hcjun@nexell.co.kr)
 *	History		: 2016.04.25 : Create
 */

#include <OMX_Video.h>

#include "NX_DecoderUtil.h"

/*
 *  H.264(AVC) Supported Profile & Level
 */

const OMX_VIDEO_AVCPROFILETYPE gstDecSupportedAVCProfiles[MAX_DEC_SUPPORTED_AVC_PROFILES] =
{
	OMX_VIDEO_AVCProfileBaseline,   /**< Baseline profile */
	OMX_VIDEO_AVCProfileMain    ,   /**< Main profile */
	OMX_VIDEO_AVCProfileHigh    ,   /**< High profile */
};

const OMX_VIDEO_AVCLEVELTYPE gstDecSupportedAVCLevels[MAX_DEC_SUPPORTED_AVC_LEVELS] =
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

/* Max DPB(Deocded Picture Buffer Size Table) */
const double gstAVCMaxDeocdedPictureBuffer[MAX_DEC_SUPPORTED_AVC_LEVELS] =
{
	148.5  , 148.5  , 337.5  , 891.0  , 891.0  , 891.0  , 1782.0 ,
	3037.5 , 3037.5 , 6750.0 , 7680.0 , 12288.0, 12288.0, 13056.0
};

#ifndef MIN
#define MIN(A,B)	((A>B)?B:A)
#endif
int32_t AVCFindMinimumBufferSize(OMX_VIDEO_AVCLEVELTYPE level, int32_t width, int32_t height)
{
	int32_t MBs;
	double MaxDPB;
	double buffers;
	if( level < OMX_VIDEO_AVCLevel1 || level > OMX_VIDEO_AVCLevel42 )
		return -1;

	MBs = ((width+15)>>4) * ((height+15)>>4);
	MaxDPB = gstDecSupportedAVCLevels[level];
	buffers = 1024.*MaxDPB/(MBs*384.) + 0.9999;
	return MIN( (int32_t)buffers, 16 );
}

/*
 * Mpeg4 Supported Profile & Level
 */
const OMX_VIDEO_MPEG4PROFILETYPE gstDecSupportedMPEG4Profiles[MAX_DEC_SUPPORTED_MPEG4_PROFILES] =
{
	OMX_VIDEO_MPEG4ProfileSimple,
	OMX_VIDEO_MPEG4ProfileAdvancedSimple,
};

const OMX_VIDEO_MPEG4LEVELTYPE gstDecSupportedMPEG4Levels[MAX_DEC_SUPPORTED_MPEG4_LEVELS] =
{
	OMX_VIDEO_MPEG4Level0 ,
	OMX_VIDEO_MPEG4Level0b,
	OMX_VIDEO_MPEG4Level1 ,
	OMX_VIDEO_MPEG4Level2 ,
	OMX_VIDEO_MPEG4Level3 ,
	OMX_VIDEO_MPEG4Level4 ,
	OMX_VIDEO_MPEG4Level4a,
	OMX_VIDEO_MPEG4Level5 ,
};

/*
 * Mpeg2 Supported Profile & Level
 */
const OMX_VIDEO_MPEG2PROFILETYPE gstDecSupportedMPEG2Profiles[MAX_DEC_SUPPORTED_MPEG2_PROFILES] =
{
	OMX_VIDEO_MPEG2ProfileSimple,
	OMX_VIDEO_MPEG2ProfileMain,
	OMX_VIDEO_MPEG2ProfileHigh,
};

const OMX_VIDEO_MPEG2LEVELTYPE gstDecSupportedMPEG2Levels[MAX_DEC_SUPPORTED_MPEG2_LEVELS] =
{
	OMX_VIDEO_MPEG2LevelLL,
	OMX_VIDEO_MPEG2LevelML,
	OMX_VIDEO_MPEG2LevelHL,
};

/*
 * added by kshblue (14.07.04)
 *
 * H263 Supported Profile & Level
 */
const OMX_VIDEO_H263PROFILETYPE gstDecSupportedH263Profiles[MAX_DEC_SUPPORTED_H263_PROFILES] =
{
	OMX_VIDEO_H263ProfileBaseline,
	OMX_VIDEO_H263ProfileISWV2,
};

const OMX_VIDEO_H263LEVELTYPE gstDecSupportedH263Levels[MAX_DEC_SUPPORTED_H263_LEVELS] =
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

/* Copy Image YV12 to General YV12 */
int32_t CopyImageToBufferYV12( uint8_t *srcY, uint8_t *srcU, uint8_t *srcV, uint8_t *dst, uint32_t strideY, uint32_t strideUV, uint32_t width, uint32_t height )
{
	uint32_t i;
	if( width == strideY )
	{
		memcpy( dst, srcY, width*height );
		dst += width*height;
	}
	else
	{
		for( i=0 ; i<height ; i++ )
		{
			memcpy( dst, srcY, width );
			srcY += strideY;
			dst += width;
		}
	}

	width /= 2;
	height /= 2;
	if( width == strideUV )
	{
		memcpy( dst, srcU, width*height );
		dst += width*height;
		memcpy( dst, srcV, width*height );
	}
	else
	{
		for( i=0 ; i<height ; i++ )
		{
			memcpy( dst, srcU, width );
			srcY += strideY;
			dst += width;
		}
		for( i=0 ; i<height ; i++ )
		{
			memcpy( dst, srcV, width );
			srcY += strideY;
			dst += width;
		}
	}
	return 0;
}
