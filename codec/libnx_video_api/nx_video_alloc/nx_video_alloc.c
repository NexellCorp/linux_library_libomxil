/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: SeongO, Park <ray@nexell.co.kr>
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>	//	PROT_READ/PROT_WRITE/MAP_SHARED/mmap/munmap

#include <drm/nexell_drm.h>
#include <nx_video_alloc.h>

#define	DRM_DEVICE_NAME	"/dev/dri/card0"


#define DRM_IOCTL_NR(n)         _IOC_NR(n)
#define DRM_IOC_VOID            _IOC_NONE
#define DRM_IOC_READ            _IOC_READ
#define DRM_IOC_WRITE           _IOC_WRITE
#define DRM_IOC_READWRITE       _IOC_READ|_IOC_WRITE
#define DRM_IOC(dir, group, nr, size) _IOC(dir, group, nr, size)

static int drm_ioctl(int32_t drm_fd, uint32_t request, void *arg)
{
	int ret;

	do {
		ret = ioctl(drm_fd, request, arg);
	} while (ret == -1 && (errno == EINTR || errno == EAGAIN));
	return ret;
}

static int drm_command_write_read(int fd, uint32_t command_index,
				  void *data, uint32_t size)
{
	uint32_t request;

	request = DRM_IOC(DRM_IOC_READ|DRM_IOC_WRITE, DRM_IOCTL_BASE,
			  DRM_COMMAND_BASE + command_index, size);
	if (drm_ioctl(fd, request, data))
		return -errno;
	return 0;
}

/**
 * return gem_fd
 */
static int alloc_gem(int drm_fd, int size, int flags)
{
	struct nx_drm_gem_create arg = { 0, };
	int ret;

	arg.size = size;
	arg.flags = flags;

	ret = drm_command_write_read(drm_fd, DRM_NX_GEM_CREATE, &arg,
				     sizeof(arg));
	if (ret) {
		perror("drm_command_write_read\n");
		return ret;
	}

	// printf("[DRM ALLOC] gem %d, size %d, flags 0x%x\n",
	//        arg.handle, size, flags);

	return arg.handle;
}

static void free_gem(int drm_fd, int gem)
{
	struct drm_gem_close arg = {0, };

	arg.handle = gem;
	drm_ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &arg);
}

/**
 * return dmabuf fd
 */
static int gem_to_dmafd(int drm_fd, int gem_fd)
{
	int ret;
	struct drm_prime_handle arg = {0, };

	arg.handle = gem_fd;
	ret = drm_ioctl(drm_fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &arg);
	if (0 != ret) {
		return -1;
	}
	return arg.fd;
}



//
//	Nexell Private Video Memory Allocator for DRM
//

#ifndef ALIGN
#define	ALIGN(X,N)	( (X+N-1) & (~(N-1)) )
#endif

#define	ALIGNED16(X)	ALIGN(X,16)


//	Nexell Private Memory Allocator
NX_MEMORY_INFO *NX_AllocateMemory( int size, int align )
{
	int gemFd = -1;
	int dmaFd = -1;
	int drmFd = open(DRM_DEVICE_NAME, O_RDWR);
	int32_t flags = 0;
	NX_MEMORY_INFO *pMem;

	if (drmFd < 0)
		return NULL;

	gemFd = alloc_gem(drmFd, size, flags);
	if (gemFd < 0)
		goto ErrorExit;

	dmaFd = gem_to_dmafd(drmFd, gemFd);
	if (dmaFd < 0)
		goto ErrorExit;

	pMem = (NX_MEMORY_INFO *)calloc(1, sizeof(NX_MEMORY_INFO));
	pMem->fd = dmaFd;
	pMem->size = size;
	pMem->align = align;

	free_gem( drmFd, gemFd );
	close( drmFd );
	return pMem;

ErrorExit:
	if( gemFd > 0 )
	{
		free_gem( drmFd, gemFd );
	}
	if( drmFd > 0 )
		close( drmFd );
	return NULL;
}

void NX_FreeMemory( NX_MEMORY_INFO *pMem )
{
	if( pMem )
	{
		if( pMem->pBuffer )
		{
			munmap( pMem->pBuffer, pMem->size );
		}
		close(pMem->fd);
		free( pMem );
	}
}


//	Video Specific Allocator Wrapper
//
//	Suport Format & Planes
//		YUV420 Format :
//			1 Plane : I420, NV12
//			2 Plane : NV12
//			3 Plane : I420
//
NX_VID_MEMORY_INFO * NX_AllocateVideoMemory( int width, int height, int32_t planes, uint32_t format, int align )
{
	int gemFd[NX_MAX_PLANES] = {0, };
	int dmaFd[NX_MAX_PLANES] = {0, };
	int drmFd = open(DRM_DEVICE_NAME, O_RDWR);
	int32_t flags = 0, i=0;
	int32_t luStride, cStride;
	int32_t luVStride, cVStride;
	int32_t stride[NX_MAX_PLANES];
	int32_t size[NX_MAX_PLANES];
	NX_VID_MEMORY_INFO *pVidMem;

	if (drmFd < 0)
		return NULL;

	//
	//	NOTE : This is for Just YUV420 
	//	FIXME
	//

#if 1 // Just for YUV420 Format
	//	Luma
	luStride = ALIGN(width, 32);
	luVStride = ALIGN(height, 16);
	//	Chroma
	cStride = luStride/2;
	cVStride = ALIGN(height/2, 16);
#endif

	//	Decide Memory Size
	switch( planes )
	{
		case 1:
			size[0] = luStride*luVStride + cStride*cVStride*2;
			stride[0] = 0;
			gemFd[0] = alloc_gem(drmFd, size[0], flags);
			if (gemFd[0] < 0)
				goto ErrorExit;
			dmaFd[0] = gem_to_dmafd(drmFd, gemFd[0]);
			if (dmaFd[0] < 0)
				goto ErrorExit;
			break;
		case 2:
			//	Buffer 1
			size[0] = luStride*luVStride;
			stride[0] = luStride;
			gemFd[0] = alloc_gem(drmFd, size[0], flags);
			if (gemFd[0] < 0)
				goto ErrorExit;
			dmaFd[0] = gem_to_dmafd(drmFd, gemFd[0]);
			if (dmaFd[0] < 0)
				goto ErrorExit;

			//	Buffer 2
			size[1] = cStride*cVStride*2;
			stride[1] = cStride * 2;
			gemFd[1] = alloc_gem(drmFd, size[1], flags);
			if (gemFd[1] < 0)
				goto ErrorExit;
			dmaFd[1] = gem_to_dmafd(drmFd, gemFd[1]);
			if (dmaFd[1] < 0)
				goto ErrorExit;
			break;
		case 3:
			//	Buffer 1
			size[0] = luStride*luVStride;
			stride[0] = luStride;
			gemFd[0] = alloc_gem(drmFd, size[0], flags);
			if (gemFd[0] < 0)
				goto ErrorExit;
			dmaFd[0] = gem_to_dmafd(drmFd, gemFd[0]);
			if (dmaFd[0] < 0)
				goto ErrorExit;

			//	Buffer 2
			size[1] = cStride*cVStride;
			stride[1] = cStride;
			gemFd[1] = alloc_gem(drmFd, size[1], flags);
			if (gemFd[1] < 0)
				goto ErrorExit;
			dmaFd[1] = gem_to_dmafd(drmFd, gemFd[1]);
			if (dmaFd[1] < 0)
				goto ErrorExit;

			//	Buffer 3
			size[2] = cStride*cVStride;
			stride[2] = cStride;
			gemFd[2] = alloc_gem(drmFd, size[2], flags);
			if (gemFd[2] < 0)
				goto ErrorExit;
			dmaFd[2] = gem_to_dmafd(drmFd, gemFd[2]);
			if (dmaFd[2] < 0)
				goto ErrorExit;
			break;
		break;
	}

	pVidMem = (NX_VID_MEMORY_INFO *)calloc(1, sizeof(NX_VID_MEMORY_INFO));
	pVidMem->width = width;
	pVidMem->height = height;
	pVidMem->align = align;
	pVidMem->planes = planes;
	pVidMem->format = format;
	for( i=0 ; i<planes ; i++ )
	{
		pVidMem->fd[i] = dmaFd[i];
		pVidMem->size[i] = size[i];
		pVidMem->stride[i] = stride[i];

		// printf("damFd = %d\n", dmaFd[i]);
		free_gem( drmFd, gemFd[i] );
	}

	close( drmFd );
	return pVidMem;

ErrorExit:
	for( i=0 ; i<planes ; i++ )
	{
		if( gemFd[i] > 0 )
		{
			free_gem( drmFd, gemFd[i] );
		}
		if( dmaFd[i] > 0 )
		{
			close( dmaFd[i] );
		}
	}
	if( drmFd > 0 )
		close( drmFd );

	return NULL;
}

void NX_FreeVideoMemory( NX_VID_MEMORY_INFO * pMem )
{
	int32_t i;
	if( pMem )
	{
		for( i=0; i < pMem->planes ; i++ )
		{
			if( pMem->pBuffer[i] )
			{
				munmap( pMem->pBuffer[i], pMem->size[i] );
			}
			close(pMem->fd[i]);
		}
		free( pMem );
	}
}


//
//		Memory Mapping/Unmapping Memory
//
int NX_MapMemory( NX_MEMORY_INFO *pMem )
{
	void *pBuf;
	if( !pMem )
		return -1;

	//	Already Mapped
	if( pMem->pBuffer )
		return -1;

	pBuf = mmap( 0, pMem->size, PROT_READ|PROT_WRITE, MAP_SHARED, pMem->fd, 0 );
	if( pBuf == MAP_FAILED )
	{
		return -1;
	}
	pMem->pBuffer = pBuf;
	return 0;
}


int NX_UnmapMemory( NX_MEMORY_INFO *pMem )
{
	if( !pMem )
		return -1;

	if( !pMem->pBuffer )
		return -1;

	if( 0 != munmap( pMem->pBuffer, pMem->size ) )
		return -1;

	pMem->pBuffer = NULL;
	return 0;
}

int NX_MapVideoMemory( NX_VID_MEMORY_INFO *pMem )
{
	int32_t i;
	void *pBuf;
	if( !pMem )
		return -1;

	//	Already Mapped
	for( i=0 ; i < pMem->planes; i ++ )
	{
		if( pMem->pBuffer[i] )
			return -1;
		else
		{
			pBuf = mmap( 0, pMem->size[i], PROT_READ|PROT_WRITE, MAP_SHARED, pMem->fd[i], 0 );
			if( pBuf == MAP_FAILED )
			{
				return -1;
			}
		}
		pMem->pBuffer[i] = pBuf;
	}
	return 0;
}

int NX_UnmapVideoMemory( NX_VID_MEMORY_INFO *pMem )
{
	int32_t i;
	if( !pMem )
		return -1;
	for( i=0; i < pMem->planes ; i++ )
	{
		if( pMem->pBuffer[i] )
		{
			munmap( pMem->pBuffer[i], pMem->size[i] );
		}
		else
			return -1;
	}
	return 0;
}
