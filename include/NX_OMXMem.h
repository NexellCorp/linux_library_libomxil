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
 *	File		: NX_OMXMem.h
 *	Brief		: OAL Memory Module
 *	Author		: HyunChul Jun (hcjun@nexell.co.kr)
 *	History		: 2016.04.25 : Create
 */ 

#ifndef __NX_OMXMem_h__
#define __NX_OMXMem_h__

#include <stdlib.h>
#include <memory.h>

#if 1	/* Android/Linux/Windows CE etc */
#define	NxMalloc			malloc
#define	NxFree				free
#define NxRealloc			realloc
#define NxMemset			memset
#define NxMemcpy			memcpy

#else	/* Custom Debug Allocator or other Allocator */

#endif

#endif	/* __NX_OMXMem_h__ */
