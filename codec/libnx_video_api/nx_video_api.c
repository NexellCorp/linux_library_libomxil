/*
 *	Copyright (C) 2016 Nexell Co. All Rights Reserved
 *	Nexell Co. Proprietary & Confidential
 *
 *	NEXELL INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
 *  AND	WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
 *  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
 *  FOR A PARTICULAR PURPOSE.
 *
 *	File		: nx_video_api.c
 *	Brief		: V4L2 Video En/Decoder
 *	Author		: SungWon Jo (doriya@nexell.co.kr)
 *	History		: 2016.04.25 : Create
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <nx_fourcc.h>

#include <sys/stat.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>
#include <linux/videodev2_nxp_media.h>

#include <nx_video_alloc.h>
#include <nx_video_api.h>

/*----------------------------------------------------------------------------*/
#define NX_V4L2_ENC_NAME		"nx-vpu-enc"
#define NX_V4L2_DEC_NAME		"nx-vpu-dec"

#define VIDEODEV_MINOR_MAX		63

/*
 *		Find Device Node
 */

/*----------------------------------------------------------------------------*/
static int32_t V4l2VpuOpen( const char *pDevName )
{
	int fd = -1;

	bool found = false;
	struct stat s;
	FILE *stream_fd;
	char filename[64], name[64];
	int32_t i = 0;
	char *pChar = NULL;

	do {
		if( i > VIDEODEV_MINOR_MAX )
			break;

		/* video device node */
		sprintf(filename, "/dev/video%d", i);

		/* if the node is video device */
		if ((lstat(filename, &s) == 0) && S_ISCHR(s.st_mode) && ((int)((unsigned short)(s.st_rdev) >> 8) == 81))
		{
			/* open sysfs entry */
			sprintf(filename, "/sys/class/video4linux/video%d/name", i);
			stream_fd = fopen(filename, "r");
			if (stream_fd == NULL) {
				printf("failed to open sysfs entry for videodev \n");
				i++;
				continue;
			}

			/* read sysfs entry for device name */
			pChar = fgets(name, sizeof(name), stream_fd);
			fclose(stream_fd);

			/* check read size */
			if (pChar == NULL)
			{
				printf("failed to read sysfs entry for videodev \n");
			}
			else
			{
				if (strncmp(name, pDevName, strlen(pDevName)) == 0)
				{
					printf("node found for device %s: /dev/video%d \n", pDevName, i );
					found = true;
				}
			}
		}

		i++;
	} while (found == false);

	if (found)
	{
		sprintf(filename, "/dev/video%d", i - 1);
		fd = open(filename,  O_RDWR);
	}

	return fd;
}


/*
 *	V4L2 Encoder
 */

/*----------------------------------------------------------------------------*/
#define MAX_CTRL_NUM 			32

struct NX_V4L2ENC_INFO {
	int32_t		fd;
	int32_t		codecType;

	NX_MEMORY_HANDLE hBitStreamBuf;
	uint8_t* 	pSeqBuf;
	int32_t		seqSize;
};

/*----------------------------------------------------------------------------*/
NX_V4L2ENC_HANDLE NX_V4l2EncOpen( int32_t v4l2Type )
{
	NX_V4L2ENC_HANDLE hEnc = (NX_V4L2ENC_HANDLE)malloc( sizeof(struct NX_V4L2ENC_INFO) );
	memset( hEnc, 0, sizeof(struct NX_V4L2ENC_INFO) );

	hEnc->fd = V4l2VpuOpen( NX_V4L2_ENC_NAME );
	if( 0 > hEnc->fd )
	{
		printf("Fail, VPU Encoder Open.\n");
		goto ERROR_EXIT;
	}

	hEnc->codecType = v4l2Type;

	return hEnc;

ERROR_EXIT:
	if( hEnc )
	{
		free( hEnc );
	}

	return NULL;
}

/*----------------------------------------------------------------------------*/
VPU_ERROR_E NX_V4l2EncClose( NX_V4L2ENC_HANDLE hEnc )
{
	enum v4l2_buf_type type;

	if( NULL == hEnc )
	{
		printf("Fail, Invalid Handle.\n");
		return VID_ERR_FAIL;
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	if( 0 != ioctl(hEnc->fd, VIDIOC_STREAMOFF, &type) )
	{
		printf("Fail, ioctl(): VIDIOC_STREAMOFF. (Input)\n");
	}

	type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	if( 0 != ioctl(hEnc->fd, VIDIOC_STREAMOFF, &type) )
	{
		printf("Fail, ioctl(): VIDIOC_STREAMOFF. (Output)\n");
	}

	if( hEnc->hBitStreamBuf )
	{
		NX_FreeMemory( hEnc->hBitStreamBuf );
	}

	if( hEnc->pSeqBuf )
	{
		free( hEnc->pSeqBuf );
	}

	if( 0 <= hEnc->fd )
	{
		close( hEnc->fd );
		hEnc->fd = -1;
	}

	free( hEnc );

	return VID_ERR_NONE;
}

/*----------------------------------------------------------------------------*/
VPU_ERROR_E NX_V4l2EncInit( NX_V4L2ENC_HANDLE hEnc, NX_V4L2ENC_PARAM *pParam )
{
	struct v4l2_buffer			buf;
	struct v4l2_plane			planes[3];
	struct v4l2_requestbuffers	req;

	struct v4l2_format			fmt;
	struct v4l2_ext_control		ext_ctrl[MAX_CTRL_NUM];
	struct v4l2_ext_controls	ext_ctrls;

	enum v4l2_buf_type	type;

	if( NULL == hEnc )
	{
		printf("Fail, Invalid Handle.\n");
		return VID_ERR_FAIL;
	}

	/* Set Input Format */
	memset( &fmt, 0, sizeof(fmt) );
	fmt.type					= V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	fmt.fmt.pix_mp.pixelformat	= V4L2_PIX_FMT_YUV420M;
	fmt.fmt.pix_mp.width		= pParam->width;
	fmt.fmt.pix_mp.height		= pParam->height;
	fmt.fmt.pix_mp.num_planes	= 3;

	if( 0 != ioctl(hEnc->fd, VIDIOC_S_FMT, &fmt) )
	{
		printf("Fail, ioctl(): VIDIOC_S_FMT. (Input)\n");
		return VID_ERR_FAIL;
	}

	/* Set Output Format */
	memset( &fmt, 0, sizeof(fmt) );
	fmt.type								= V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	fmt.fmt.pix_mp.pixelformat				= hEnc->codecType;
	fmt.fmt.pix_mp.plane_fmt[0].sizeimage	= pParam->width * pParam->height * 3 / 4;

	if( 0 != ioctl(hEnc->fd, VIDIOC_S_FMT, &fmt) )
	{
		printf("Fail, ioctl(): VIDIOC_S_FMT. (Output)\n");
		return VID_ERR_FAIL;
	}

	/* Set Encoder Configuration */
	ext_ctrl[0].id		= V4L2_CID_MPEG_VIDEO_FPS_NUM;
	ext_ctrl[0].value	= pParam->fpsNum;
	ext_ctrl[1].id		= V4L2_CID_MPEG_VIDEO_FPS_DEN;
	ext_ctrl[1].value	= pParam->fpsDen;
	ext_ctrl[2].id		= V4L2_CID_MPEG_VIDEO_GOP_SIZE;
	ext_ctrl[2].value	= pParam->gopSize;

	ext_ctrl[3].id		= V4L2_CID_MPEG_VIDEO_CYCLIC_INTRA_REFRESH_MB;
	ext_ctrl[3].value	= pParam->numIntraRefreshMbs;
	ext_ctrl[4].id		= V4L2_CID_MPEG_VIDEO_SEARCH_RANGE;
	ext_ctrl[4].value	= pParam->searchRange;

	ext_ctrl[5].id		= V4L2_CID_MPEG_VIDEO_FRAME_RC_ENABLE;
	ext_ctrl[5].value	= ( pParam->bitrate ) ? ( 1 ) : ( 0 );
	ext_ctrl[6].id		= V4L2_CID_MPEG_VIDEO_BITRATE;
	ext_ctrl[6].value	= pParam->bitrate;
	ext_ctrl[7].id		= V4L2_CID_MPEG_VIDEO_VBV_SIZE;
	ext_ctrl[7].value	= pParam->rcVbvSize;
	ext_ctrl[8].id		= V4L2_CID_MPEG_VIDEO_RC_DELAY;
	ext_ctrl[8].value	= pParam->RCDelay;
	ext_ctrl[9].id		= V4L2_CID_MPEG_VIDEO_RC_GAMMA_FACTOR;
	ext_ctrl[9].value	= pParam->gammaFactor;
	ext_ctrl[10].id		= V4L2_CID_MPEG_VIDEO_FRAME_SKIP_MODE;
	ext_ctrl[10].value	= pParam->disableSkip;

	if( hEnc->codecType == V4L2_PIX_FMT_H264 )
	{
		ext_ctrl[11].id		= V4L2_CID_MPEG_VIDEO_H264_AUD_INSERT;
		ext_ctrl[11].value	= pParam->enableAUDelimiter;
		ext_ctrls.count = 12;

		if( (pParam->bitrate == 0) || (pParam->initialQp > 0) )
		{
			ext_ctrl[12].id		= V4L2_CID_MPEG_VIDEO_H264_I_FRAME_QP;
			ext_ctrl[12].value	= pParam->initialQp;
			ext_ctrl[13].id		= V4L2_CID_MPEG_VIDEO_H264_P_FRAME_QP;
			ext_ctrl[13].value	= pParam->initialQp;
			ext_ctrl[14].id		= V4L2_CID_MPEG_VIDEO_H264_MAX_QP;
			ext_ctrl[14].value	= pParam->maximumQp;
			ext_ctrls.count += 3;
		}
	}
	else if( hEnc->codecType == V4L2_PIX_FMT_MPEG4 )
	{
		ext_ctrls.count = 11;

		if( (pParam->bitrate == 0) || (pParam->initialQp > 0) )
		{
			ext_ctrl[11].id		= V4L2_CID_MPEG_VIDEO_MPEG4_I_FRAME_QP;
			ext_ctrl[11].value	= pParam->initialQp;
			ext_ctrl[12].id		=  V4L2_CID_MPEG_VIDEO_MPEG4_P_FRAME_QP;
			ext_ctrl[12].value	= pParam->initialQp;
			ext_ctrl[13].id		=  V4L2_CID_MPEG_VIDEO_MPEG4_MAX_QP;
			ext_ctrl[13].value	= pParam->maximumQp;
			ext_ctrls.count += 3;	
		}
	}
	else if( hEnc->codecType == V4L2_PIX_FMT_H263 )
	{
		ext_ctrl[11].id		= V4L2_CID_MPEG_VIDEO_H263_PROFILE;
		ext_ctrl[11].value	= pParam->profile;
		ext_ctrls.count = 12;

		if( (pParam->bitrate == 0) || (pParam->initialQp > 0) )
		{
			ext_ctrl[12].id		= V4L2_CID_MPEG_VIDEO_H263_I_FRAME_QP;
			ext_ctrl[12].value	= pParam->initialQp;
			ext_ctrl[13].id		=  V4L2_CID_MPEG_VIDEO_H263_P_FRAME_QP;
			ext_ctrl[13].value	= pParam->initialQp;
			ext_ctrl[14].id		=  V4L2_CID_MPEG_VIDEO_H263_MAX_QP;
			ext_ctrl[14].value	= pParam->maximumQp;
			ext_ctrls.count += 3;
		}
	}

	ext_ctrls.ctrl_class	= V4L2_CTRL_CLASS_MPEG;
	ext_ctrls.controls		= ext_ctrl;

	if( 0 != ioctl(hEnc->fd, VIDIOC_S_EXT_CTRLS, &ext_ctrls) )
	{
		printf("Fail, ioctl(): VIDIOC_S_EXT_CTRLS.\n");
		return VID_ERR_FAIL;
	}

	/* Request Input Buffer */
	memset( &req, 0, sizeof(req) );
	req.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	req.count	= 1;
	req.memory	= V4L2_MEMORY_DMABUF;

	if( 0 != ioctl(hEnc->fd, VIDIOC_REQBUFS, &req) )
	{
		printf("Fail, ioctl(): VIDIOC_REQBUFS. (Input)\n");
		return VID_ERR_FAIL;
	}

	/* Request Output Buffer */
	memset( &req, 0, sizeof(req) );
	req.type	= V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	req.count	= 1;
	req.memory	= V4L2_MEMORY_DMABUF;

	if( 0 != ioctl(hEnc->fd, VIDIOC_REQBUFS, &req) )
	{
		printf("Fail, ioctl(): VIDIOC_REQBUFS. (Output)\n");
		return VID_ERR_FAIL;
	}

	/* Input / Output Stream On */
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	if ( 0 != ioctl(hEnc->fd, VIDIOC_STREAMON, &type) ) {
		printf("Fail, ioctl(): VIDIOC_STREAMON. (Input)\n");
		return VID_ERR_FAIL;
	}

	type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	if( 0 != ioctl(hEnc->fd, VIDIOC_STREAMON, &type) )
	{
		printf("Fail, ioctl(): VIDIOC_STREAMON. (Output)\n");
		return VID_ERR_FAIL;
	}

	/* Allocate Output Buffer */
	hEnc->hBitStreamBuf = NX_AllocateMemory( pParam->width * pParam->height * 3 / 4, 4096 );
	if( hEnc->hBitStreamBuf )
	{
		if( 0 != NX_MapMemory(hEnc->hBitStreamBuf) )
		{
			printf("Fail, NX_MapMemory().\n");
			return VID_ERR_FAIL;
		}
	}
	else
	{
		printf("Fail, NX_AllocateMemory().\n");
		return VID_ERR_FAIL;
	}	

	/* Parse Sequence Header */
	memset(&buf, 0, sizeof(buf));
	buf.type	= V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	buf.m.planes= planes;
	buf.length	= 1;
	buf.memory	= V4L2_MEMORY_DMABUF;
	buf.index	= 0;

	buf.m.planes[0].m.fd		= hEnc->hBitStreamBuf->fd;
	buf.m.planes[0].length		= hEnc->hBitStreamBuf->size;
	buf.m.planes[0].bytesused	= hEnc->hBitStreamBuf->size;
	buf.m.planes[0].data_offset	= 0;

	if( 0 != ioctl(hEnc->fd, VIDIOC_QBUF, &buf) )
	{
		printf("Fail, ioctl(): VIDIOC_QBUF. (Output ES)\n");
		return VID_ERR_FAIL;
	}

	memset(&buf, 0, sizeof(buf));
	buf.type	= V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	buf.m.planes= planes;
	buf.length	= 1;
	buf.memory	= V4L2_MEMORY_DMABUF;
	buf.index	= 0;

	if( 0 != ioctl(hEnc->fd, VIDIOC_DQBUF, &buf) )
	{
		printf("Fail, ioctl(): VIDIOC_DQBUF. (Output ES)\n");
		return VID_ERR_FAIL;
	}

	if( 0 < buf.m.planes[0].bytesused )
	{
		hEnc->seqSize = buf.m.planes[0].bytesused;
		hEnc->pSeqBuf = (uint8_t*)malloc( hEnc->seqSize );

		memcpy( hEnc->pSeqBuf, (void *)hEnc->hBitStreamBuf->pBuffer, hEnc->seqSize );
	}

	return VID_ERR_NONE;
}

/*----------------------------------------------------------------------------*/
VPU_ERROR_E NX_V4l2EncGetSeqInfo( NX_V4L2ENC_HANDLE hEnc, uint8_t **ppSeqBuf, int32_t *iSeqSize )
{
	if( NULL == hEnc )
	{
		printf("Fail, Invalid Handle.\n");
		return VID_ERR_FAIL;
	}

	*ppSeqBuf = hEnc->pSeqBuf;
	*iSeqSize = hEnc->seqSize;

	return VID_ERR_NONE;
}

/*----------------------------------------------------------------------------*/
VPU_ERROR_E NX_V4l2EncEncodeFrame( NX_V4L2ENC_HANDLE hEnc, NX_V4L2ENC_IN *pEncIn, NX_V4L2ENC_OUT *pEncOut )
{
	int i;
	struct v4l2_plane planes[3];
	struct v4l2_buffer buf;

	memset( pEncOut, 0, sizeof(NX_V4L2ENC_OUT) );

	/* Queue Input Buffer */
	memset( &buf, 0, sizeof(buf) );
	buf.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	buf.m.planes= planes;
	buf.length	= 3;
	buf.memory	= V4L2_MEMORY_DMABUF;
	buf.index	= 0;

	for( i = 0; i < 3; i++ )
	{
		buf.m.planes[i].m.fd	= pEncIn->pImage->fd[i];
		buf.m.planes[i].length	= pEncIn->pImage->size[i];
	}

	if( 0 != ioctl(hEnc->fd, VIDIOC_QBUF, &buf) )
	{
		printf("Fail, ioctl(): VIDIOC_QBUF. (Input)\n");
		return VID_ERR_FAIL;
	}

	/* Queue Output Buffer */
	memset(&buf, 0, sizeof(buf));
	buf.type	= V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	buf.m.planes= planes;
	buf.length	= 1;
	buf.index	= 0;
	buf.memory	= V4L2_MEMORY_DMABUF;

	buf.m.planes[0].m.fd		= hEnc->hBitStreamBuf->fd;
	buf.m.planes[0].length		= hEnc->hBitStreamBuf->size;
	buf.m.planes[0].bytesused	= hEnc->hBitStreamBuf->size;
	buf.m.planes[0].data_offset	= 0;

	if( 0 != ioctl(hEnc->fd, VIDIOC_QBUF, &buf) )
	{
		printf("Fail, ioctl(): VIDIOC_QBUF. (Output)\n");
		return VID_ERR_FAIL;
	}

	/* Dequeue Input Buffer */
	memset(&buf, 0, sizeof(buf));
	buf.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	buf.m.planes= planes;
	buf.length	= 3;
	buf.memory	= V4L2_MEMORY_DMABUF;

	if( 0 != ioctl(hEnc->fd, VIDIOC_DQBUF, &buf) )
	{
		printf("Fail, ioctl(): VIDIOC_DQBUF. (Input)\n");
		return VID_ERR_FAIL;
	}

	/* Dequeue Output Buffer */
	memset( &buf, 0, sizeof(buf) );
	buf.type	= V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	buf.m.planes= planes;
	buf.length	= 1;
	buf.memory	= V4L2_MEMORY_DMABUF;

	if( 0 != ioctl(hEnc->fd, VIDIOC_DQBUF, &buf) )
	{
		printf("Fail, ioctl(): VIDIOC_DQBUF. (Output)\n");
		return VID_ERR_FAIL;
	}

	/* Return Encoding Result */
	pEncOut->outBuf	= (uint8_t*)hEnc->hBitStreamBuf->pBuffer;
	pEncOut->bufSize= buf.m.planes[0].bytesused;

	if		( buf.reserved == 0x08 ) 	pEncOut->frameType = PIC_TYPE_I;
	else if	( buf.reserved == 0x10 ) 	pEncOut->frameType = PIC_TYPE_P;
	else if ( buf.reserved == 0x20 ) 	pEncOut->frameType = PIC_TYPE_B;
	else								pEncOut->frameType = PIC_TYPE_UNKNOWN;

	return VID_ERR_NONE;
}



/*
 *		V4L2 Decoder
 */

/*----------------------------------------------------------------------------*/
#define MAX_DEC_WIDTH			1920
#define MAX_DEC_HEIGHT			1088

struct NX_V4L2DEC_INFO {
	int32_t		fd;
	int32_t		codecType;
	int32_t 	instIndex;

	int32_t 	width;
	int32_t 	height;

	int32_t 	numFrameBuffers;
	NX_MEMORY_HANDLE 		hBitStreamBuf;
	NX_VID_MEMORY_HANDLE 	hFrameBuffer[MAX_DEC_FRAME_BUFFERS];

	int32_t 	useExternalFrameBuffer;
};

/*----------------------------------------------------------------------------*/
NX_V4L2DEC_HANDLE NX_V4l2DecOpen( int32_t v4l2Type )
{
	NX_V4L2DEC_HANDLE hDec = (NX_V4L2DEC_HANDLE)malloc( sizeof(struct NX_V4L2DEC_INFO) );
	memset( hDec, 0, sizeof(struct NX_V4L2DEC_INFO) );

	hDec->fd = V4l2VpuOpen( NX_V4L2_DEC_NAME );
	if( 0 > hDec->fd )
	{
		printf("Fail, Decoder Open Fail.\n");
		goto ERROR_EXIT;
	}

	hDec->codecType = v4l2Type;

	return hDec;

ERROR_EXIT:
	if( hDec )
	{
		free( hDec );
	}

	return NULL;
}

/*----------------------------------------------------------------------------*/
VPU_ERROR_E NX_V4l2DecClose( NX_V4L2DEC_HANDLE hDec )
{
	enum v4l2_buf_type type;
	int32_t i;

	if( NULL == hDec )
	{
		printf("Feil, Invalid Handle.\n");
		return VID_ERR_FAIL;
	}

	type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	if( 0 != ioctl(hDec->fd, VIDIOC_STREAMOFF, &type) )
	{
		printf("Fail, ioctl(): VIDIOC_STREAMOFF. (Input)\n");
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	if( 0 != ioctl(hDec->fd, VIDIOC_STREAMOFF, &type) )
	{
		printf("Fail, ioctl(): VIDIOC_STREAMOFF. (Output)\n");
	}

	if( 0 <= hDec->fd )
	{
		close( hDec->fd );
		hDec->fd = -1;
	}

	if( 0 == hDec->useExternalFrameBuffer )
	{
		for( i = 0; i < hDec->numFrameBuffers; i++ )
		{
			if( hDec->hFrameBuffer[i] )
			{
				NX_FreeVideoMemory( hDec->hFrameBuffer[i] );
				hDec->hFrameBuffer[i] = NULL;
			}
		}
	}
	free( hDec );

	return VID_ERR_NONE;
}


/*----------------------------------------------------------------------------*/
VPU_ERROR_E NX_V4l2DecParseVideoCfg( NX_V4L2DEC_HANDLE hDec, NX_V4L2DEC_SEQ_IN *pSeqIn, NX_V4L2DEC_SEQ_OUT *pSeqOut )
{
	struct v4l2_format fmt;
	struct v4l2_requestbuffers req;
	struct v4l2_buffer buf;
	struct v4l2_plane planes[3];
	struct v4l2_crop crop;
	enum v4l2_buf_type type;

	memset( pSeqOut, 0, sizeof(NX_V4L2DEC_SEQ_OUT) );

	if( NULL == hDec )
	{
		printf("Fail, Invalid Handle.\n");
		return VID_ERR_FAIL;
	}

	/* Set Input Format */
	memset( &fmt, 0, sizeof(fmt) );
	fmt.type					= V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	fmt.fmt.pix_mp.pixelformat	= hDec->codecType;

	if( (pSeqIn->width == 0) || (pSeqIn->height == 0) )
		fmt.fmt.pix_mp.plane_fmt[0].sizeimage = MAX_DEC_WIDTH * MAX_DEC_HEIGHT * 3 / 4;	
	else
		fmt.fmt.pix_mp.plane_fmt[0].sizeimage = pSeqIn->width * pSeqIn->height * 3 / 4;		

	fmt.fmt.pix_mp.width		= pSeqIn->width;
	fmt.fmt.pix_mp.height		= pSeqIn->height;
	fmt.fmt.pix_mp.num_planes	= 1;

	if( 0 != ioctl(hDec->fd, VIDIOC_S_FMT, &fmt) )
	{
		printf("Fail, ioctl(): VIDIOC_S_FMT. (Input)\n");
		return VID_ERR_FAIL;
	}

	/* Set Input Request Buffer */
	memset(&req, 0, sizeof(req));
	req.type	= V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	req.count	= 1;
	req.memory	= V4L2_MEMORY_DMABUF;

	if( 0 != ioctl(hDec->fd, VIDIOC_REQBUFS, &req) )
	{
		printf("Fail, ioctl(): VIDIOC_REQBUFS. (Input)\n");
		return VID_ERR_FAIL;
	}

	/* Allocate Input Buffer */
	hDec->hBitStreamBuf = NX_AllocateMemory( MAX_DEC_WIDTH * MAX_DEC_HEIGHT * 3 / 4, 4096 );
	if( hDec->hBitStreamBuf )
	{
		if( 0 != NX_MapMemory(hDec->hBitStreamBuf) )
		{
			printf("Fail, NX_MapMemory().\n");
			return VID_ERR_FAIL;
		}
	}
	else
	{
		printf("Fail, NX_AllocateMemory().\n");
		return VID_ERR_FAIL;
	}

	/* Parse Sequence Header */
	memcpy( (void *)hDec->hBitStreamBuf->pBuffer, pSeqIn->seqInfo, pSeqIn->seqSize );

	memset( &buf, 0, sizeof(buf) );
	buf.type	= V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	buf.m.planes= planes;
	buf.length	= 1;
	buf.memory	= V4L2_MEMORY_DMABUF;
	buf.index	= 0;
	buf.flags	= 0;

	buf.m.planes[0].m.fd		= hDec->hBitStreamBuf->fd;
	buf.m.planes[0].length		= hDec->hBitStreamBuf->size;
	buf.m.planes[0].bytesused	= pSeqIn->seqSize;
	buf.m.planes[0].data_offset	= 0;

	if( 0 != ioctl(hDec->fd, VIDIOC_QBUF, &buf) )
	{
		printf("Fail, ioctl(): VIDIOC_QBUF. (Input ES)\n");
		return VID_ERR_FAIL;
	}


	type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	if ( ioctl(hDec->fd, VIDIOC_STREAMON, &type) != 0 )
	{
		printf("Fail, ioctl(): VIDIOC_STREAMON. (Input)\n");
		return VID_ERR_FAIL;
	}

	memset(&buf, 0, sizeof(buf));
	buf.type	= V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	buf.m.planes= planes;
	buf.length	= 1;
	buf.memory	= V4L2_MEMORY_DMABUF;

	if ( ioctl(hDec->fd, VIDIOC_DQBUF, &buf) != 0 )
	{
		printf("Fail, ioctl(): VIDIOC_DQBUF. (Input ES)\n");
		return VID_ERR_FAIL;
	}

	/* Get Image Information */
	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

	if( 0 != ioctl( hDec->fd, VIDIOC_G_FMT, &fmt ) ) {
		printf("Fail, ioctl(): VIDIOC_G_FMT.\n");
		return VID_ERR_FAIL;
	}

	// pSeqOut->width			= fmt.fmt.pix_mp.width;		// TBD
	pSeqOut->width			= fmt.fmt.pix_mp.plane_fmt[0].bytesperline;
	pSeqOut->height			= fmt.fmt.pix_mp.height;
	pSeqOut->minBuffers		= fmt.fmt.raw_data[0];
	pSeqOut->numBuffers 	= fmt.fmt.raw_data[0] + pSeqIn->addNumBuffers;
	hDec->numFrameBuffers	= fmt.fmt.raw_data[0];

	memset( &crop, 0, sizeof(crop) );
	crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

	if( 0 != ioctl( hDec->fd, VIDIOC_G_CROP, &crop ) ) {
		printf("Fail, ioctl(): VIDIOC_G_CROP.\n");
		return VID_ERR_FAIL;
	}

	pSeqOut->dispLeft	= crop.c.left;
	pSeqOut->dispTop	= crop.c.top;
	pSeqOut->dispRight	= crop.c.left + crop.c.width;
	pSeqOut->dispBottom	= crop.c.top + crop.c.height;

	/* Set Output format */
	memset( &fmt, 0, sizeof(fmt) );
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	fmt.fmt.pix_mp.pixelformat	= V4L2_PIX_FMT_YUV420M;
	fmt.fmt.pix_mp.width		= pSeqOut->width;
	fmt.fmt.pix_mp.height		= pSeqOut->height;
	fmt.fmt.pix_mp.num_planes	= 3;

	if( 0 != ioctl(hDec->fd, VIDIOC_S_FMT, &fmt) )
	{
		printf("Fail, ioctl(): VIDIOC_S_FMT. (Output)\n");
		return VID_ERR_FAIL;
	}

	return VID_ERR_NONE;
}

/*----------------------------------------------------------------------------*/
VPU_ERROR_E NX_V4l2DecInit( NX_V4L2DEC_HANDLE hDec, NX_V4L2DEC_SEQ_IN *pSeqIn )
{
	struct v4l2_buffer buf;
	struct v4l2_requestbuffers req;
	struct v4l2_plane planes[3];
	enum v4l2_buf_type type;
	int i, j;

	/* Calculate Buffer Number */
	if( 0 < pSeqIn->numBuffers )
	{
		hDec->useExternalFrameBuffer = true;
		if( 2 > pSeqIn->numBuffers - hDec->numFrameBuffers )
		{
			printf("External Buffer too small.(min=%d, buffers=%d)\n", hDec->numFrameBuffers, pSeqIn->numBuffers );
		}

		hDec->numFrameBuffers = pSeqIn->numBuffers;
	}
	else
	{
		hDec->numFrameBuffers += pSeqIn->addNumBuffers;
	}

	/* Request Output Buffer */
	memset(&req, 0, sizeof(req));
	req.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	req.count	= hDec->numFrameBuffers;
	req.memory	= V4L2_MEMORY_DMABUF;

	if( 0 != ioctl(hDec->fd, VIDIOC_REQBUFS, &req) )
	{
		printf("Fail, ioctl(): VIDIOC_S_FMT. (Output)\n");
		return VID_ERR_FAIL;
	}

	memset( &buf, 0, sizeof(buf) );
	buf.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	buf.m.planes= planes;
	buf.length	= 3;
	buf.memory	= V4L2_MEMORY_DMABUF;

	/* Allocate Buffer ( Internal or External ) */
	for( i = 0; i < hDec->numFrameBuffers; i++ )
	{
		if( true == hDec->useExternalFrameBuffer )
		{
			hDec->hFrameBuffer[i] = pSeqIn->pMemHandle[i];
		}
		else
		{
			hDec->hFrameBuffer[i] = NX_AllocateVideoMemory( pSeqIn->width, pSeqIn->height, 3, FOURCC_MVS0, 4096 );
			if( hDec->hFrameBuffer[i] )
			{
				if( 0 != NX_MapVideoMemory( hDec->hFrameBuffer[i] ) )
				{
					printf("Video Memory Mapping Failed\n");
					return VID_ERR_FAIL;
				}
			}
			else
			{
				printf("Frame Memory allocation Failed\n");
				return VID_ERR_FAIL;
			}
		}

		buf.index = i;
		
		for( j=0 ; j<3 ; j++ )
		{
			buf.m.planes[j].m.fd	= hDec->hFrameBuffer[i]->fd[j];
			buf.m.planes[j].length	= hDec->hFrameBuffer[i]->size[j];
		}

		if( 0 != ioctl(hDec->fd, VIDIOC_QBUF, &buf) )
		{
			printf("Fail, ioctl(): VIDIOC_QBUF. (Output)\n");
			return VID_ERR_FAIL;
		}
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	if( 0 != ioctl(hDec->fd, VIDIOC_STREAMON, &type) )
	{
		printf("Fail, ioctl(): VIDIOC_STREAMON. (Output)\n");
		return VID_ERR_FAIL;
	}

	return VID_ERR_NONE;
}

/*----------------------------------------------------------------------------*/
VPU_ERROR_E NX_V4l2DecDecodeFrame( NX_V4L2DEC_HANDLE hDec, NX_V4L2DEC_IN *pDecIn, NX_V4L2DEC_OUT *pDecOut )
{
	struct v4l2_buffer buf;
	struct v4l2_plane planes[3];

	if( NULL == hDec )
	{
		printf("Fail, Invalid Handle.\n");
		goto ERROR_EXIT;
	}

	/* Ready Input Stream */
	memcpy( (void *)hDec->hBitStreamBuf->pBuffer, pDecIn->strmBuf, pDecIn->strmSize );
	
	/* Queue Input Buffer */
	memset( &buf, 0, sizeof(buf) );
	buf.type	= V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	buf.m.planes= planes;
	buf.length	= 1;
	buf.memory	= V4L2_MEMORY_DMABUF;
	buf.index	= 0;
	buf.timestamp.tv_sec	= pDecIn->timeStamp/1000;
	buf.timestamp.tv_usec	= (pDecIn->timeStamp % 1000) * 1000;
	buf.flags	= pDecIn->eos ? 1 : 0;

	buf.m.planes[0].m.fd		= hDec->hBitStreamBuf->fd;
	buf.m.planes[0].length		= hDec->hBitStreamBuf->size;
	buf.m.planes[0].bytesused	= pDecIn->strmSize;
	buf.m.planes[0].data_offset	= 0;

	if( 0 != ioctl(hDec->fd, VIDIOC_QBUF, &buf) )
	{
		printf("Fail, ioctl(): VIDIOC_QBUF. (Input)\n");
		goto ERROR_EXIT;
	}

	if( 0 < hDec->hBitStreamBuf->size )
	{
		/* Dequeue Input ES Buffer -> Get Decoded Order Result */
		memset( &buf, 0, sizeof(buf) );
		buf.type	= V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
		buf.m.planes= planes;
		buf.length	= 1;			
		buf.memory	= V4L2_MEMORY_DMABUF;

		if ( ioctl(hDec->fd, VIDIOC_DQBUF, &buf) != 0 ) {
			printf("Fail, ioctl(): VIDIOC_DQBUF. (Input)\n");
			goto ERROR_EXIT;
		}

		pDecOut->outDecIdx = buf.index;
		pDecOut->isInterlace = buf.field;
		pDecOut->outFrmReliable_0_100[DECODED_FRAME] = buf.reserved;
		pDecOut->timeStamp[DECODED_FRAME] = ((uint64_t)buf.timestamp.tv_sec)*1000 + buf.timestamp.tv_usec/1000;

		if		( buf.reserved2 == 0x08 ) 	pDecOut->picType[DECODED_FRAME] = PIC_TYPE_I;
		else if	( buf.reserved2 == 0x10 ) 	pDecOut->picType[DECODED_FRAME] = PIC_TYPE_P;
		else if ( buf.reserved2 == 0x20 ) 	pDecOut->picType[DECODED_FRAME] = PIC_TYPE_B;
		else								pDecOut->picType[DECODED_FRAME] = PIC_TYPE_UNKNOWN;
	}

	/* Dequeue Output YUV Buffer -> Get Display Order Result */
	memset(&buf, 0, sizeof(buf));
	buf.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	buf.m.planes= planes;
	buf.length	= 3;
	buf.memory	= V4L2_MEMORY_DMABUF;

	if( 0 != ioctl(hDec->fd, VIDIOC_DQBUF, &buf) )
	{
		printf("Fail, ioctl(): VIDIOC_DQBUF. (Output)\n");
		goto ERROR_EXIT;
	}

	pDecOut->outImgIdx = buf.index;

	if( 0 <= pDecOut->outImgIdx )
	{
		pDecOut->outImg								= *hDec->hFrameBuffer[buf.index];
		pDecOut->timeStamp[DISPLAY_FRAME]			= ((uint64_t)buf.timestamp.tv_sec)*1000 + buf.timestamp.tv_usec/1000;
		pDecOut->isInterlace						= buf.field;
		pDecOut->outFrmReliable_0_100[DISPLAY_FRAME]= buf.reserved;

		if		( buf.reserved2 == 0x08 ) 	pDecOut->picType[DISPLAY_FRAME] = PIC_TYPE_I;
		else if	( buf.reserved2 == 0x10 ) 	pDecOut->picType[DISPLAY_FRAME] = PIC_TYPE_P;
		else if ( buf.reserved2 == 0x20 ) 	pDecOut->picType[DISPLAY_FRAME] = PIC_TYPE_B;
		else								pDecOut->picType[DISPLAY_FRAME] = PIC_TYPE_UNKNOWN;
	}
	
ERROR_EXIT:
	return VID_ERR_NONE;	
}

/*----------------------------------------------------------------------------*/
VPU_ERROR_E NX_V4l2DecClrDspFlag( NX_V4L2DEC_HANDLE hDec, NX_VID_MEMORY_HANDLE hFrameBuf, int32_t iFrameIdx )
{
	struct v4l2_buffer buf;
	struct v4l2_plane planes[3];
	int32_t index = -1;
	int32_t i;

	if( NULL == hDec )
	{
		printf("Fail, Invalid Handle.\n");
		return VID_ERR_FAIL;
	}

	/* Search Buffer Index */
	if( hFrameBuf != NULL )
	{
		for( i = 0; i < MAX_DEC_FRAME_BUFFERS; i++ )
		{
			if( hFrameBuf == hDec->hFrameBuffer[i] )
			{
				index = i;
				break;
			}
		}
	}
	else
	{
		index = iFrameIdx;
	}

	if( index < 0 )
	{
		printf("Fail, Invalid FrameBuffer or FrameIndex.\n");
		return VID_ERR_FAIL;
	}

	memset( &buf, 0, sizeof(buf) );
	buf.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	buf.index	= index;
	buf.m.planes= planes;
	buf.length	= 3;
	buf.memory	= V4L2_MEMORY_DMABUF;

	for( i = 0; i < 3; i++ )
	{
		buf.m.planes[i].m.fd = hDec->hFrameBuffer[index]->fd[i];
		buf.m.planes[i].length = hDec->hFrameBuffer[index]->size[i];
	}

	/* Queue Output Buffer */
	if( 0 != ioctl(hDec->fd, VIDIOC_QBUF, &buf) )
	{
		printf("Fail, ioctl(): VIDIOC_QBUF.(Clear Display Index, index = %d)\n", index);
		return VID_ERR_FAIL;
	}	

	return VID_ERR_NONE;	
}

/*----------------------------------------------------------------------------*/
VPU_ERROR_E NX_V4l2DecFlush( NX_V4L2DEC_HANDLE hDec )
{
	/*
	 *		Not YET..
	 */

	return VID_ERR_NONE;
}
