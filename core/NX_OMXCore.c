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
 *	File		: NX_OMXCore.h
 *	Brief		: OpenMAX IL Core
 *	Author		: HyunChul Jun (hcjun@nexell.co.kr)
 *	History		: 2016.04.25 : Create
 */

/* for android debug */
#define	LOG_TAG				"NX_OMXCore"

#include <dlfcn.h>   /* For dynamic loading */
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>	/* sprintf */

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <NX_OMXComponentMgr.h>
#include <NX_OMXMem.h>
#include <NX_OMXDebugMsg.h>

#ifndef UNUSED_PARAM
#define	UNUSED_PARAM(X)		X=X
#endif

#define TRACE_CORE			0
#if (TRACE_CORE)
#define	TRACE(...)			NX_RelMsg(NX_DBG_DEBUG  , __VA_ARGS__)
#else
#define	TRACE(...)			NX_RelMsg(NX_DBG_TRACE  , __VA_ARGS__)
#endif

#if 1	/* USE Dynamic Loader */
#define NX_OMX_Init			OMX_Init
#define NX_OMX_Deinit		OMX_Deinit
#define NX_OMX_GetHandle	OMX_GetHandle
#define NX_OMX_FreeHandle	OMX_FreeHandle
#define NX_OMX_SetupTunnel	OMX_SetupTunnel
#endif

static OMX_U32				gstCurOMXComponents = 0;	/* Number of Registerd Components */
static OMX_U32				gstCurOMXCompRoles = 0;		/* Number of Registerd Component Roles */
static OMX_U32				gstOMXCoreRefCnt = 0;
static OMX_BOOL				gstNXOMXInit = OMX_FALSE;
static NX_COMPONENT_INFO	gstOMXComponentList[NX_MAX_COMPONENTS];

static NX_COMPONENT_REG_INFO gstCompRegInfo[NX_MAX_COMPONENTS];
static OMX_U32				 gstNumRegInfo = 0;

pthread_mutex_t gstOMXCoreMutex = PTHREAD_MUTEX_INITIALIZER;

/* Debug Message */
static OMX_ERRORTYPE NX_OMXRegisterComponent( const char *Name, const char* SoName, const char* Role ,OMX_ERRORTYPE (*ComponentInit)( OMX_IN OMX_HANDLETYPE hComponent ) )
{
	if( strlen(Name) > OMX_MAX_STRINGNAME_SIZE-1 )
		return OMX_ErrorBadParameter;
	if( strlen(Role) > OMX_MAX_STRINGNAME_SIZE-1 )
		return OMX_ErrorBadParameter;
	if( strlen(SoName) > OMX_MAX_STRINGNAME_SIZE-1 )
		return OMX_ErrorBadParameter;

	gstCompRegInfo[gstNumRegInfo].CompName = strdup(Name);
	gstCompRegInfo[gstNumRegInfo].CompRole = strdup(Role);
	gstCompRegInfo[gstNumRegInfo].CompSoName = strdup(SoName);

	gstCompRegInfo[gstNumRegInfo].ComponentInit = ComponentInit;
	gstNumRegInfo++;
	return OMX_ErrorNone;
}

static OMX_ERRORTYPE RegisterAllComponents()
{
	/* OMX IL */
	NX_OMXRegisterComponent("OMX.NX.VIDEO_DECODER.avc",		"libNX_OMX_VIDEO_DECODER",		"video_decoder.avc",		NULL);
	NX_OMXRegisterComponent("OMX.NX.VIDEO_DECODER.mpeg4",	"libNX_OMX_VIDEO_DECODER",		"video_decoder.mpeg4",		NULL);
	NX_OMXRegisterComponent("OMX.NX.VIDEO_DECODER.mpeg2",	"libNX_OMX_VIDEO_DECODER",		"video_decoder.mpeg2",		NULL);
	NX_OMXRegisterComponent("OMX.NX.VIDEO_DECODER.h263",	"libNX_OMX_VIDEO_DECODER",		"video_decoder.h263",		NULL);
	NX_OMXRegisterComponent("OMX.NX.VIDEO_DECODER.3gpp",	"libNX_OMX_VIDEO_DECODER",		"video_decoder.h263",		NULL);
	NX_OMXRegisterComponent("OMX.NX.VIDEO_DECODER.3gpp",	"libNX_OMX_VIDEO_DECODER",		"video_decoder.3gpp",		NULL);
	NX_OMXRegisterComponent("OMX.NX.VIDEO_DECODER.mp43",	"libNX_OMX_VIDEO_DECODER",		"video_decoder.mp43",		NULL);
	NX_OMXRegisterComponent("OMX.NX.VIDEO_DECODER.flv",		"libNX_OMX_VIDEO_DECODER",		"video_decoder.x-flv",		NULL);
	NX_OMXRegisterComponent("OMX.NX.VIDEO_DECODER.wmv",		"libNX_OMX_VIDEO_DECODER",		"video_decoder.x-ms-wmv",	NULL);
	NX_OMXRegisterComponent("OMX.NX.VIDEO_DECODER.wvc1",	"libNX_OMX_VIDEO_DECODER",		"video_decoder.x-ms-wmv",	NULL);
	NX_OMXRegisterComponent("OMX.NX.VIDEO_DECODER.vc1",		"libNX_OMX_VIDEO_DECODER",		"video_decoder.x-ms-wmv",	NULL);
	NX_OMXRegisterComponent("OMX.NX.VIDEO_DECODER.rv",		"libNX_OMX_VIDEO_DECODER",		"video_decoder.x-pn-realvideo",NULL);
	NX_OMXRegisterComponent("OMX.NX.VIDEO_DECODER.vp8",		"libNX_OMX_VIDEO_DECODER",		"video_decoder.vp8",		NULL);

	NX_OMXRegisterComponent("OMX.NX.VIDEO_ENCODER.avc",		"libNX_OMX_VIDEO_ENCODER",		"video_encoder.avc",		NULL);
	NX_OMXRegisterComponent("OMX.NX.VIDEO_ENCODER.mpeg4",	"libNX_OMX_VIDEO_ENCODER",		"video_encoder.mpeg4",		NULL);
	NX_OMXRegisterComponent("OMX.NX.VIDEO_ENCODER.h263",	"libNX_OMX_VIDEO_ENCODER",		"video_encoder.h263",		NULL);
	NX_OMXRegisterComponent("OMX.NX.VIDEO_ENCODER.3gpp",	"libNX_OMX_VIDEO_ENCODER",		"video_encoder.h263",		NULL);
	return OMX_ErrorNone;
}

static OMX_ERRORTYPE NX_OMXBuildComponentTable()
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_U32 j = 0, i = 0, regComps = 0, regRoles = 0;

	if( gstNXOMXInit != OMX_FALSE )
	{
		return eError;
	}

	memset( &gstOMXComponentList, 0, sizeof(gstOMXComponentList) );
	memset( &gstCompRegInfo, 0, sizeof(gstCompRegInfo) );

	/* Register component name & role & init function. */
	RegisterAllComponents();

	/* Initialize Component List */
	NxMemset( &gstOMXComponentList[0], 0, sizeof(gstOMXComponentList) );

	for( i=0 ; i<NX_MAX_COMPONENTS ; i++ )
	{
		if( i >= gstNumRegInfo )
		{
			break;	/* Exit Main Loop */
		}
		/* Search Components */
		for( j=0 ; j<regComps ; j++ )
		{
			/* Search Matching Components */
			if( !strcmp( gstCompRegInfo[i].CompName, gstOMXComponentList[j].strName ) )
			{
				/* Add Role */
				gstOMXComponentList[j].pRoles[gstOMXComponentList[j].nRoles] = gstCompRegInfo[i].CompRole;
				gstOMXComponentList[j].nRoles++;
				regRoles ++;
				break;
			}
		}
		/* Add New Components */
		if( regComps == j )
		{
			gstOMXComponentList[regComps].strName   = gstCompRegInfo[i].CompName;
			gstOMXComponentList[regComps].strSoName = gstCompRegInfo[i].CompSoName;
			gstOMXComponentList[regComps].nRoles    = 1;
			gstOMXComponentList[regComps].pRoles[0] = gstCompRegInfo[i].CompRole;
			gstOMXComponentList[regComps].handle[0] = NULL;
			gstOMXComponentList[regComps].nRefCount = 0;
			gstOMXComponentList[regComps].ComponentInit = gstCompRegInfo[i].ComponentInit;
			regComps ++;
			regRoles ++;
		}
	}

	gstCurOMXComponents = regComps;
	gstCurOMXCompRoles = regRoles;
	gstNXOMXInit = OMX_TRUE;

	if( 0==gstCurOMXComponents )
	{
		/* FIXME : Insert Error Message */
	}

	return eError;
}

void NX_OMXDestroyComponentTable()
{
	OMX_S32 i =0;
	for( i = 0 ; i<NX_MAX_COMPONENTS ; i++ )
	{
		if( gstCompRegInfo[i].CompName )
		{
			free( gstCompRegInfo[i].CompName );
		}
		if( gstCompRegInfo[i].CompRole )
		{
			free( gstCompRegInfo[i].CompRole );
		}
		if( gstCompRegInfo[i].CompSoName )
		{
			free( gstCompRegInfo[i].CompSoName );
		}
	}
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY NX_OMX_Init(void)
{
	pthread_mutex_lock( &gstOMXCoreMutex );
	//TRACE("\t %s()++\n", __FUNCTION__);

	if( 0 == gstOMXCoreRefCnt )
	{
		/* Load All Components */
		NX_OMXBuildComponentTable();
	}
	gstOMXCoreRefCnt ++;
	//TRACE("\t %s()--\n", __FUNCTION__);
	pthread_mutex_unlock( &gstOMXCoreMutex );
	return OMX_ErrorNone;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY NX_OMX_Deinit(void)
{
	/* Unload All components */
	pthread_mutex_lock( &gstOMXCoreMutex );
	//TRACE("\t %s()++\n", __FUNCTION__);
	if( gstOMXCoreRefCnt > 0 )
		gstOMXCoreRefCnt --;
	if( gstOMXCoreRefCnt == 0 ){
		NX_OMXDestroyComponentTable();
	}
	//TRACE("\t %s()--\n", __FUNCTION__);
	pthread_mutex_unlock( &gstOMXCoreMutex );
	return OMX_ErrorNone;
}

/*
 *
 * TODO : Whether to use the information Registerd the NX_OMX_ComponentNameEnum if not
 *        Modified by the judgment whether to use the information present in substantially.
 */
OMX_API OMX_ERRORTYPE OMX_APIENTRY NX_OMX_ComponentNameEnum(
	OMX_OUT OMX_STRING cComponentName,
	OMX_IN  OMX_U32 nNameLength,
	OMX_IN  OMX_U32 nIndex)
{
	OMX_ERRORTYPE err = OMX_ErrorNone;
//	TRACE("\t %s(index = %ld, nNameLength=%ld)++\n", __FUNCTION__, nIndex, nNameLength);
	if( OMX_TRUE!=gstNXOMXInit || nIndex > gstCurOMXComponents-1 ){
		return OMX_ErrorNoMore;
	}
	pthread_mutex_lock( &gstOMXCoreMutex );

	if( nNameLength-1 < strlen(gstOMXComponentList[nIndex].strName) ){
		err = OMX_ErrorInvalidComponentName;
	}else{
		strcpy( cComponentName, gstOMXComponentList[nIndex].strName );
	}
	pthread_mutex_unlock( &gstOMXCoreMutex );
//	TRACE("\t %s(err=%d,Name=%s)--\n", __FUNCTION__, err, cComponentName);
	return err;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY NX_OMX_GetHandle(
	OMX_OUT OMX_HANDLETYPE* pHandle,
	OMX_IN  OMX_STRING cComponentName,
	OMX_IN  OMX_PTR pAppData,
	OMX_IN  OMX_CALLBACKTYPE* pCallBacks)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_COMPONENTTYPE *pComponent = NULL;
	OMX_ERRORTYPE	(*ComponentInit)( OMX_IN OMX_HANDLETYPE hComponent ) = NULL;
	OMX_U32 i=0;
	OMX_BOOL found = OMX_FALSE;
	NX_COMPONENT_INFO *pCompInfo=NULL;

	pthread_mutex_lock( &gstOMXCoreMutex );
//	TRACE("\t %s(compName=%s)++\n", __FUNCTION__, cComponentName);

	/* Find Component Name From ComponentList */
	for( i=0 ; i < gstCurOMXComponents ; i++ )
	{
		if( !strncmp( cComponentName, gstOMXComponentList[i].strName, strlen(gstOMXComponentList[i].strName) ) )
		{
			pCompInfo = &gstOMXComponentList[i];
			found = OMX_TRUE;
			break;
		}
	}

	if( found == OMX_FALSE )
	{
		pthread_mutex_unlock( &gstOMXCoreMutex );
		return OMX_ErrorInsufficientResources;
	}

	/* Dynamic Component Loader */
	/* The CompoentInit is not NULL in static loader. */
	if( NULL == pCompInfo->ComponentInit )
	{
		char soname[512];
		sprintf( soname, "%s.so", pCompInfo->strSoName);

		if( pCompInfo->hModule == NULL )
		{
			pCompInfo->hModule = (OMX_HANDLETYPE)dlopen( soname, RTLD_NOW );
			if( pCompInfo->hModule != NULL )
			{
				ComponentInit = dlsym(pCompInfo->hModule, "OMX_ComponentInit");
			}else
			{
				ErrMsg("\t Error : Cannot ldopen failed(%s, soName=%s)\n", dlerror(), soname);
			}
		}
	}
	else
	{
		TRACE("Alread opened shared object!(pCompnentInit = %p)\n", pCompInfo->ComponentInit);
		ComponentInit = pCompInfo->ComponentInit;
	}

	if( NULL == ComponentInit )
	{
		pthread_mutex_unlock( &gstOMXCoreMutex );
		ErrMsg("\t Error : Invalid component name (%s).\n", cComponentName );
		return OMX_ErrorInvalidComponentName;
	}

	pCompInfo->ComponentInit = ComponentInit;

	/* Component Allocation & SetCallbacks */
	pComponent = (OMX_COMPONENTTYPE *)NxMalloc(sizeof(OMX_COMPONENTTYPE));
	if( NULL == pComponent )
	{
		pthread_mutex_unlock( &gstOMXCoreMutex );
		ErrMsg("\t Error : Component not found(%s).\n", cComponentName );
		return OMX_ErrorComponentNotFound;
	}
	NxMemset( pComponent, 0, sizeof(OMX_COMPONENTTYPE) );

	/* Call real component initialize */
	eError = ComponentInit( pComponent );
	if( eError == OMX_ErrorNone )
	{
		eError = pComponent->SetCallbacks( (OMX_HANDLETYPE*)pComponent, pCallBacks, pAppData );
	}

	if( eError == OMX_ErrorNone )
	{
		*pHandle = (OMX_HANDLETYPE*)pComponent;
		pComponent->pApplicationPrivate = pAppData;
	}
	else
	{
		NxFree( pComponent );
		ErrMsg("\t Error : ComponentInit() Failed, (%s, eError=%ld(0x%08x) ).\n", cComponentName, eError, eError );
	}
//	TRACE("\t %s()--\n", __FUNCTION__);
	pthread_mutex_unlock( &gstOMXCoreMutex );
	return eError;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY NX_OMX_FreeHandle(
	OMX_IN  OMX_HANDLETYPE hComponent)
{
	OMX_COMPONENTTYPE *pComponent = hComponent;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	pthread_mutex_lock( &gstOMXCoreMutex );
//	TRACE("\t %s()++\n", __FUNCTION__);
	if( pComponent )
	{
		eError = pComponent->ComponentDeInit( hComponent );
		NxFree( hComponent );
	}
	else
	{
		eError = OMX_ErrorUndefined;
	}
//	TRACE("\t %s()--\n", __FUNCTION__);
	pthread_mutex_unlock( &gstOMXCoreMutex );
	return eError;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY NX_OMX_SetupTunnel(
	OMX_IN  OMX_HANDLETYPE hOutput,
	OMX_IN  OMX_U32 nPortOutput,
	OMX_IN  OMX_HANDLETYPE hInput,
	OMX_IN  OMX_U32 nPortInput)
{
	UNUSED_PARAM(hOutput);
	UNUSED_PARAM(nPortOutput);
	UNUSED_PARAM(hInput);
	UNUSED_PARAM(nPortInput);
	return OMX_ErrorNotImplemented;
}

OMX_API OMX_ERRORTYPE   NX_OMX_GetContentPipe(
	OMX_OUT OMX_HANDLETYPE *hPipe,
	OMX_IN OMX_STRING szURI)
{
	UNUSED_PARAM(hPipe);
	UNUSED_PARAM(szURI);
	return OMX_ErrorNotImplemented;
}

OMX_API OMX_ERRORTYPE NX_OMX_GetComponentsOfRole(
	OMX_IN      OMX_STRING role,
	OMX_INOUT   OMX_U32 *pNumComps,
	OMX_INOUT   OMX_U8  **compNames)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_U32 i=0,j=0,numMatch = 0;
	pthread_mutex_lock(&gstOMXCoreMutex);
//	TRACE("\t %s()++\n",__FUNCTION__);
	for(i=0 ; i<gstCurOMXComponents ; i++)
	{
		for(j=0 ; j<gstOMXComponentList[i].nRoles ; j++)
		{
			if(!strcmp(role,gstOMXComponentList[i].pRoles[j]))
			{
				/* Add Component List */
				strcpy((OMX_STRING)compNames[numMatch],gstOMXComponentList[i].strName);
				numMatch ++;
			}
		}
	}
	*pNumComps = numMatch;
//	TRACE("\t %s()--\n",__FUNCTION__);
	pthread_mutex_unlock(&gstOMXCoreMutex);
	return eError;
}

OMX_API OMX_ERRORTYPE NX_OMX_GetRolesOfComponent (
	OMX_IN      OMX_STRING compName,
	OMX_INOUT   OMX_U32 *pNumRoles,
	OMX_OUT     OMX_U8 **roles)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_U32 i=0,j=0,numMatch = 0;
	pthread_mutex_lock(&gstOMXCoreMutex);
//	TRACE("\t %s(*pNumRoles=%ld, compName=%s)++\n",__FUNCTION__,*pNumRoles,compName);

	for(i=0 ; i<gstCurOMXComponents ; i++)
	{
		if(!strcmp(compName,gstOMXComponentList[i].strName))
		{
			numMatch = gstOMXComponentList[i].nRoles;
			break;
		}
	}

	if(numMatch == 0)
	{
		pthread_mutex_unlock(&gstOMXCoreMutex);
		return OMX_ErrorComponentNotFound;
	}

	if(roles == NULL)
	{
		*pNumRoles = numMatch;
	}
	else
	{
		if(*pNumRoles >= gstOMXComponentList[i].nRoles)
		{
			for(j=0 ; j<gstOMXComponentList[i].nRoles ; j++)
			{
				strcpy((OMX_STRING)roles[j],gstOMXComponentList[i].pRoles[j]);
			}
			*pNumRoles = gstOMXComponentList[i].nRoles;
		}
		else
		{
			eError = OMX_ErrorBadParameter;
		}
	}
//	TRACE("\t %s()--\n",__FUNCTION__);
	pthread_mutex_unlock(&gstOMXCoreMutex);
	return eError;
}
