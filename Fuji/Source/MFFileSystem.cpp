#include "Fuji_Internal.h"
#include "MFHeap_Internal.h"
#include "MFSystem.h"
#include "MFModule.h"
#include "MFSockets.h"
#include "MFFileSystem_Internal.h"
#include "FileSystem/MFFileSystemNative_Internal.h"
#include "FileSystem/MFFileSystemMemory_Internal.h"
#include "FileSystem/MFFileSystemCachedFile_Internal.h"
#include "FileSystem/MFFileSystemZipFile_Internal.h"
#include "FileSystem/MFFileSystemHTTP_Internal.h"
#include "MFObjectPool.h"

#if defined(FindClose)
	#undef FindClose
#endif

#define MAX_JOBS 16

struct MFFileSystemState
{
	MFObjectPool gOpenFiles;

	MFMount *pMountList;
	MFMount *pMountListEnd;

	MFPtrListDL<MFFileSystem> gFileSystems;
	MFFileSystem **ppFileSystemList;

	MFPtrListDL<MFFind> gFinds;

	// internal filesystems
	MFFileSystemHandle hNativeFileSystem;
	MFFileSystemHandle hMemoryFileSystem;
	MFFileSystemHandle hCachedFileSystem;
	MFFileSystemHandle hZipFileSystem;
	MFFileSystemHandle hHTTPFileSystem;
	MFFileSystemHandle hFTPFileSystem;

	MFFile *hDataArchive;
	MFFile *hPatchArchive;
};

static int gModuleId = -1;

// HACK: extern to the module id's of the filesystems so we can fetch their fs handles...
extern int gFileSystemCachedFileId;
extern int gFileSystemHTTPId;
extern int gFileSystemMemoryId;
extern int gFileSystemNativeId;
extern int gFileSystemZipFileId;


/**** Functions ****/

static const char * gArchiveNames[] =
{
	"Data_%s.zip",
	"Data_%s.dat",
	"Data/Data_%s.zip",
	"Data/Data_%s.dat",
	NULL
};

static const char * gPatchArchiveNames[] =
{
	"Patch_%s.zip",
	"Patch_%s.dat",
	"Data/Patch_%s.zip",
	"Data/Patch_%s.dat",
	NULL
};

MFJob *NewJob(MFFileSystem *pFS)
{
	MFJob *pJob = pFS->jobs.Create();
	MFZeroMemory(pJob, sizeof(MFJob));
	return pJob;
}

void PostJob(MFFileSystem *pFS, MFJob *pJob)
{
	int nextJob = (pFS->writeJob+1) % pFS->numJobs;
	while(nextJob == pFS->readJob)
	{
		// yield...
	}

	pFS->ppJobQueue[pFS->writeJob] = pJob;
	MFThread_SignalSemaphore(pFS->semaphore);
	pFS->writeJob = nextJob;
}

int MKFileJobThread(void *pData)
{
	MFFileSystem *pFS = (MFFileSystem*)pData;

	while(1)
	{
		// wait for a job
		MFThread_WaitSemaphore(pFS->semaphore);

		MFJob *pJob = pFS->ppJobQueue[pFS->readJob];
		MFFile *pFile = pJob->pFile;
		pJob->status = MFJS_Busy;
		pJob->result = 0;

		switch(pJob->job)
		{
			case MFFJ_Exit:
				pJob->status = MFJS_Finished;
				goto exit;
			case MFFJ_Open:
				pJob->result = pFS->callbacks.Open(pJob->pFile, &pJob->data.open.openData);
				break;
			case MFFJ_Close:
				pJob->result = pFS->callbacks.Close(pJob->pFile);
				break;
			case MFFJ_Read:
				pJob->result = pFS->callbacks.Read(pFile, pJob->data.read.pBuffer, pJob->data.read.bytes);
//				pFile->offset += pJob->result;
				break;
			case MFFJ_Write:
				pJob->result = pFS->callbacks.Write(pFile, pJob->data.write.pBuffer, pJob->data.write.bytes);
//				pFile->offset += pJob->result;
//				pFile->length = MFMax(pFile->length, pFile->offset);
				break;
			case MFFJ_Seek:
				pJob->result = pFS->callbacks.Seek(pJob->pFile, pJob->data.seek.bytes, pJob->data.seek.whence);
//				if(pJob->result > -1)
//					pFile->offset = pJob->result;
				break;
			case MFFJ_Stat:
				MFDebug_Assert(false, "Not done..");
				break;
			case MFFJ_Tell:
				pJob->result = pFile->offset;
				break;
			case MFFJ_GetSize:
				pJob->result = pFile->length;
				break;
			case MFFJ_Load:
			{
//				MFFile *pFile = MFFile_Open(
				break;
			}
			case MFFJ_Save:
				break;
			default:
				break;
		}

		pJob->status = MFJS_Finished;
		++pFS->readJob;
	}

exit:
	MFThread_ExitThread(0);
	return 0;
}

void MFFileSystem_RegisterDefaultArchives()
{
	GET_MODULE_DATA(MFFileSystemState);

	MFFile *&hDataArchive = pModuleData->hDataArchive;
	MFFile *&hPatchArchive = pModuleData->hPatchArchive;

	// try and mount the 'standard' archives...
	// TODO: ponder removing this code and forcing the use of a filesystem init callback? :/
	MFOpenDataNative dataArchive;
	dataArchive.cbSize = sizeof(MFOpenDataNative);
	dataArchive.openFlags = MFOF_Read|MFOF_Binary;
	hDataArchive = 0;
	const char **pArc = gArchiveNames;
	while(!hDataArchive && *pArc)
	{
		dataArchive.pFilename =  MFFile_SystemPath(MFStr(*pArc, MFSystem_GetPlatformString(MFSystem_GetCurrentPlatform())));
		hDataArchive = MFFile_Open(pModuleData->hNativeFileSystem, &dataArchive);
		++pArc;
	}

	MFMountDataNative mountData;
	mountData.cbSize = sizeof(MFMountDataNative);
	mountData.priority = MFMP_Normal;

	if(hDataArchive)
	{
		// attempt to cache the zip archive
		MFOpenDataCachedFile cachedOpen;
		cachedOpen.cbSize = sizeof(MFOpenDataCachedFile);
		cachedOpen.openFlags = MFOF_Read | MFOF_Binary | MFOF_Cached_CleanupBaseFile;
		cachedOpen.maxCacheSize = 1*1024*1024; // 1mb cache for zip archives should be heaps!!
		cachedOpen.pBaseFile = hDataArchive;

		MFFile *pCachedFile = MFFile_Open(MFFileSystem_GetInternalFileSystemHandle(MFFSH_CachedFileSystem), &cachedOpen);
		if(pCachedFile)
			hDataArchive = pCachedFile;

		// mount the zip archive.
		MFMountDataZipFile zipMountData;
		zipMountData.cbSize = sizeof(MFMountDataZipFile);
		zipMountData.flags = MFMF_Recursive|MFMF_FlattenDirectoryStructure;
		zipMountData.priority = MFMP_Normal;
		zipMountData.pMountpoint = "data";
		zipMountData.pZipArchive = hDataArchive;
		MFFileSystem_Mount(pModuleData->hZipFileSystem, &zipMountData);
	}

	mountData.flags = MFMF_DontCacheTOC;
	mountData.pMountpoint = "home";
	mountData.pPath = MFFile_HomePath();
	MFFileSystem_Mount(pModuleData->hNativeFileSystem, &mountData);

	// see if we can mount the patch archive..
	hPatchArchive = 0;
	pArc = gPatchArchiveNames;
	while(!hPatchArchive && *pArc)
	{
		dataArchive.pFilename =  MFFile_SystemPath(MFStr(*pArc, MFSystem_GetPlatformString(MFSystem_GetCurrentPlatform())));
		hDataArchive = MFFile_Open(pModuleData->hNativeFileSystem, &dataArchive);
		++pArc;
	}

	if(hPatchArchive)
	{
		// attempt to cache the zip archive
		MFOpenDataCachedFile cachedOpen;
		cachedOpen.cbSize = sizeof(MFOpenDataCachedFile);
		cachedOpen.openFlags = MFOF_Read | MFOF_Binary | MFOF_Cached_CleanupBaseFile;
		cachedOpen.maxCacheSize = 1*1024*1024; // 1mb cache for zip archives should be heaps!!
		cachedOpen.pBaseFile = hPatchArchive;

		MFFile *pCachedFile = MFFile_Open(MFFileSystem_GetInternalFileSystemHandle(MFFSH_CachedFileSystem), &cachedOpen);
		if(pCachedFile)
			hPatchArchive = pCachedFile;

		// mount the zip archive.
		MFMountDataZipFile zipMountData;
		zipMountData.cbSize = sizeof(MFMountDataZipFile);
		zipMountData.flags = MFMF_Recursive|MFMF_FlattenDirectoryStructure;
		zipMountData.priority = MFMP_VeryHigh;
		zipMountData.pMountpoint = "patch";
		zipMountData.pZipArchive = hPatchArchive;
		MFFileSystem_Mount(pModuleData->hZipFileSystem, &zipMountData);
	}

	if(pModuleData->hHTTPFileSystem != -1)
	{
		// register the network filesystems
		MFMountDataHTTP mountDataHTTP;
		mountDataHTTP.cbSize = sizeof(MFMountDataHTTP);
		mountDataHTTP.pMountpoint = "http";
		mountDataHTTP.priority = MFMP_Normal + 1;
		mountDataHTTP.flags = MFMF_OnlyAllowExclusiveAccess;
		MFFileSystem_Mount(pModuleData->hHTTPFileSystem, &mountDataHTTP);
	}
}

MFInitStatus MFFileSystem_InitFileSystems(int moduleId, bool bPerformInitialisation)
{
	GET_MODULE_DATA(MFFileSystemState);

	pModuleData->hNativeFileSystem = ((MFFileSystemGlobalState*)gpEngineInstance->modules[gFileSystemNativeId].pModuleData)->hFileSystemHandle;
	pModuleData->hMemoryFileSystem = ((MFFileSystemGlobalState*)gpEngineInstance->modules[gFileSystemMemoryId].pModuleData)->hFileSystemHandle;
	pModuleData->hCachedFileSystem = ((MFFileSystemGlobalState*)gpEngineInstance->modules[gFileSystemCachedFileId].pModuleData)->hFileSystemHandle;
	pModuleData->hZipFileSystem = ((MFFileSystemGlobalState*)gpEngineInstance->modules[gFileSystemZipFileId].pModuleData)->hFileSystemHandle;
	pModuleData->hHTTPFileSystem = ((MFFileSystemGlobalState*)gpEngineInstance->modules[gFileSystemHTTPId].pModuleData)->hFileSystemHandle;

	// call the filesystem init callback
	MFSystemCallbackFunction pFilesystemInitCallback = MFSystem_GetSystemCallback(MFCB_FileSystemInit);
	if(pFilesystemInitCallback)
		pFilesystemInitCallback();

	return MFIS_Succeeded;
}

uint64 MFFileSystem_RegisterFilesystemModules(int filesystemModule)
{
	uint64 fsBit = 1ULL << filesystemModule;

	// mount filesystems
	MFFileSystemHandle hNativeFileSystem = MFModule_RegisterModule("MFFileSystemNative", MFFileSystemNative_InitModule, MFFileSystemNative_DeinitModule, fsBit);
	MFFileSystemHandle hMemoryFileSystem = MFModule_RegisterModule("MFFileSystemMemory", MFFileSystemMemory_InitModule, MFFileSystemMemory_DeinitModule, fsBit);
	MFFileSystemHandle hCachedFileSystem = MFModule_RegisterModule("MFFileSystemCachedFile", MFFileSystemCachedFile_InitModule, MFFileSystemCachedFile_DeinitModule, fsBit);
	MFFileSystemHandle hZipFileSystem = MFModule_RegisterModule("MFFileSystemZipFile", MFFileSystemZipFile_InitModule, MFFileSystemZipFile_DeinitModule, fsBit);

	uint64 socketsBit = MFModule_GetBuiltinModuleMask(MFBIM_MFSockets);
	MFFileSystemHandle hHTTPFileSystem = MFModule_RegisterModule("MFFileSystemHTTP", MFFileSystemHTTP_InitModule, MFFileSystemHTTP_DeinitModule, fsBit | socketsBit);

	uint64 filesystems = 0;
	filesystems |= 1ULL << hNativeFileSystem;
	filesystems |= 1ULL << hMemoryFileSystem;
	filesystems |= 1ULL << hCachedFileSystem;
	filesystems |= 1ULL << hZipFileSystem;
	filesystems |= 1ULL << hHTTPFileSystem;

	gpEngineInstance->builtinModuleIDs[MFBIM_MFFileSystem] = (char)MFModule_RegisterModule("MFFileSystem", MFFileSystem_InitFileSystems, NULL, filesystems);
	filesystems |= MFModule_GetBuiltinModuleMask(MFBIM_MFFileSystem);

	return filesystems;
}

MFInitStatus MFFileSystem_InitModule(int moduleId, bool bPerformInitialisation)
{
	gModuleId = moduleId;

	if(!bPerformInitialisation)
		return MFIS_Succeeded;

	ALLOC_MODULE_DATA(MFFileSystemState);

	pModuleData->hNativeFileSystem = -1;
	pModuleData->hMemoryFileSystem = -1;
	pModuleData->hCachedFileSystem = -1;
	pModuleData->hZipFileSystem = -1;
	pModuleData->hHTTPFileSystem = -1;
	pModuleData->hFTPFileSystem = -1;

	pModuleData->gOpenFiles.Init(sizeof(MFFile), gDefaults.filesys.maxOpenFiles, gDefaults.filesys.maxOpenFiles);

	pModuleData->gFileSystems.Init("File Systems", gDefaults.filesys.maxFileSystems);
	pModuleData->ppFileSystemList = (MFFileSystem**)MFHeap_Alloc(sizeof(MFFileSystem*) * gDefaults.filesys.maxFileSystems);
	MFZeroMemory(pModuleData->ppFileSystemList, sizeof(MFFileSystem*) * gDefaults.filesys.maxFileSystems);

	pModuleData->gFinds.Init("File System Find Instances", gDefaults.filesys.maxFinds);

	return MFIS_Succeeded;
}

void MFFileSystem_DeinitModule()
{
	GET_MODULE_DATA(MFFileSystemState);

	if(pModuleData->hDataArchive)
	{
		MFFile_Close(pModuleData->hDataArchive);
	}

	MFFileSystemHTTP_DeinitModule();
	MFFileSystemZipFile_DeinitModule();
	MFFileSystemCachedFile_DeinitModule();
	MFFileSystemMemory_DeinitModule();
	MFFileSystemNative_DeinitModule();

	MFHeap_Free(pModuleData->ppFileSystemList);

	pModuleData->gFileSystems.Deinit();
	pModuleData->gOpenFiles.Deinit();
	pModuleData->gFinds.Deinit();
}

MFFileSystemHandle MFFileSystem_RegisterFileSystem(const char *pFilesystemName, MFFileSystemCallbacks *pCallbacks)
{
	MFDebug_Log(5, MFStr("Call: MFFileSystem_RegisterFileSystem(\"%s\")", pFilesystemName));

	GET_MODULE_DATA(MFFileSystemState);

	for(uint32 a=0; a<gDefaults.filesys.maxFileSystems; a++)
	{
		if(pModuleData->ppFileSystemList[a] == NULL)
		{
			MFDebug_Assert(pCallbacks->Open, "No Open function supplied.");
			MFDebug_Assert(pCallbacks->Close, "No Close function supplied.");
			MFDebug_Assert(pCallbacks->Read, "No Read function supplied.");
			MFDebug_Assert(pCallbacks->Write, "No Write function supplied.");
			MFDebug_Assert(pCallbacks->Seek, "No Seek function supplied.");

			MFFileSystem *pFS = pModuleData->gFileSystems.Create();
			MFZeroMemory(pFS, sizeof(MFFileSystem));
			MFString_Copy(pFS->name, pFilesystemName);
			MFCopyMemory(&pFS->callbacks, pCallbacks, sizeof(MFFileSystemCallbacks));
			pModuleData->ppFileSystemList[a] = pFS;

#if defined(USE_JOB_THREAD)
			pFS->ppJobQueue = (MFJob**)MFHeap_Alloc(sizeof(MFJob*)*MAX_JOBS);
			pFS->jobs.Init(MFStr("%s Job List", pFilesystemName), MAX_JOBS+2);
			pFS->readJob = 0;
			pFS->writeJob = 0;
			pFS->numJobs = MAX_JOBS;
			pFS->semaphore = MFThread_CreateSemaphore("Filesystem Semaphore", MAX_JOBS, 0);
			pFS->thread = MFThread_CreateThread(MFStr("%s Thread", pFilesystemName), MKFileJobThread, pFS, MFPriority_AboveNormal);
#endif

			if(pFS->callbacks.RegisterFS)
				pFS->callbacks.RegisterFS();

			return a;
		}
	}

	MFDebug_Assert(false, MFStr("Exceeded maximum of %d Filesystems. Modify 'gDefaults.filesys.maxFileSystems'.", gDefaults.filesys.maxFileSystems));

	return -1;
}

void MFFileSystem_UnregisterFileSystem(MFFileSystemHandle filesystemHandle)
{
	MFDebug_Assert((uint32)filesystemHandle < gDefaults.filesys.maxFileSystems, "Invalid filesystem");

	GET_MODULE_DATA(MFFileSystemState);

	MFFileSystem *pFS = pModuleData->ppFileSystemList[filesystemHandle];
	MFDebug_Assert(pFS, "Filesystem not mounted");

	if(pFS->callbacks.UnregisterFS)
		pFS->callbacks.UnregisterFS();

	if(pFS->thread)
	{
		// terminate job thread
		MFJob *pJob = NewJob(pFS);
		PostJob(pFS, pJob);
		while(pJob->status != MFJS_Finished)
		{
			// yield
		}

		MFThread_DestroySemaphore(pFS->semaphore);
		pFS->jobs.Deinit();
		MFHeap_Free(pFS->ppJobQueue);
	}

	pModuleData->gFileSystems.Destroy(pFS);
	pModuleData->ppFileSystemList[filesystemHandle] = NULL;
}

// interface functions
MF_API const char* MFFile_SystemPath(const char *pFilename)
{
	MFCALLSTACK;

	pFilename = pFilename ? pFilename : "";

#if defined(MF_IPHONE)
	const char *GetAppHome();
	return MFStr("%s/%s", GetAppHome(), pFilename);
#else
	return MFStr("./%s", pFilename);
#endif
}

MF_API const char* MFFile_HomePath(const char *pFilename)
{
	MFCALLSTACK;

	pFilename = pFilename ? pFilename : "";

#if defined(MF_XBOX)
	return MFStr("E:\\Home\\%s", pFilename);
#elif defined(_LINUX)
	return MFStr("~/%s", pFilename);
#elif defined(MF_IPHONE)
	const char *GetUserHome();
	return MFStr("%s/%s", GetUserHome(), pFilename);
#else
	return MFStr("Home/%s", pFilename);
#endif
}

///////////////////////////
// file access functions
MF_API MFFile* MFFile_Open(MFFileSystemHandle fileSystem, MFOpenData *pOpenData)
{
	GET_MODULE_DATA(MFFileSystemState);

	MFFile *pFile = (MFFile*)pModuleData->gOpenFiles.AllocAndZero();
	pFile->filesystem = fileSystem;

	int result = pModuleData->ppFileSystemList[fileSystem]->callbacks.Open(pFile, pOpenData);

	if(result < 0)
	{
		pModuleData->gOpenFiles.Free(pFile);
		return NULL;
	}

	return pFile;
}

MF_API int MFFile_Close(MFFile* fileHandle)
{
	GET_MODULE_DATA(MFFileSystemState);

	pModuleData->ppFileSystemList[fileHandle->filesystem]->callbacks.Close(fileHandle);
	pModuleData->gOpenFiles.Free(fileHandle);

	return 0;
}

// read/write functions
MF_API int MFFile_Read(MFFile* fileHandle, void *pBuffer, size_t bytes, bool async)
{
	GET_MODULE_DATA(MFFileSystemState);

	return pModuleData->ppFileSystemList[fileHandle->filesystem]->callbacks.Read(fileHandle, pBuffer, bytes);
}

MF_API int MFFile_Write(MFFile* fileHandle, const void *pBuffer, size_t bytes, bool async)
{
	GET_MODULE_DATA(MFFileSystemState);

	return pModuleData->ppFileSystemList[fileHandle->filesystem]->callbacks.Write(fileHandle, pBuffer, bytes);
}

// offset management (these are stdio function signature compliant)
MF_API int MFFile_Seek(MFFile* fileHandle, int bytes, MFFileSeek relativity)
{
	GET_MODULE_DATA(MFFileSystemState);

	return pModuleData->ppFileSystemList[fileHandle->filesystem]->callbacks.Seek(fileHandle, bytes, relativity);
}

MF_API int MFFile_Tell(MFFile* fileHandle)
{
	return (int)fileHandle->offset;
}

// get file stream length (retuurs -1 for an undefined stream length)
MF_API int64 MFFile_GetSize(MFFile* fileHandle)
{
	return fileHandle->length;
}

MF_API bool MFFile_IsEOF(MFFile* fileHandle)
{
	return fileHandle->offset == fileHandle->length;
}

// stdio signiture functions (these can be used as a callback to many libs and API's)
MF_API size_t MFFile_StdRead(void *pBuffer, size_t size, size_t count, void* fileHandle)
{
	return (size_t)MFFile_Read((MFFile*)fileHandle, pBuffer, (uint32)(size*count), false);
}

MF_API size_t MFFile_StdWrite(const void *pBuffer, size_t size, size_t count, void* fileHandle)
{
	return (size_t)MFFile_Write((MFFile*)fileHandle, pBuffer, (uint32)(size*count), false);
}
MF_API int MFFile_StdClose(void* fileHandle)
{
	return MFFile_Close((MFFile*)fileHandle);
}

MF_API long MFFile_StdSeek(void* fileHandle, long bytes, int relativity)
{
	long seek = (long)MFFile_Seek((MFFile*)fileHandle, (int)bytes, (MFFileSeek)relativity);
	return seek < 0 ? seek : 0;
}

MF_API long MFFile_StdTell(void* fileHandle)
{
	return (long)MFFile_Tell((MFFile*)fileHandle);
}

//////////////////////////////
// mounted filesystem access

// return a handle to a specific filesystem
MF_API MFFileSystemHandle MFFileSystem_GetInternalFileSystemHandle(MFFileSystemHandles fileSystemHandle)
{
	GET_MODULE_DATA(MFFileSystemState);

	switch(fileSystemHandle)
	{
		case MFFSH_NativeFileSystem:
			MFDebug_Assert(pModuleData->hNativeFileSystem > -1, "Native filesystem is not available...");
			if(pModuleData->hNativeFileSystem < 0)
				MFDebug_Error("Native filesystem is not available...");
			return pModuleData->hNativeFileSystem;
		case MFFSH_MemoryFileSystem:
			MFDebug_Assert(pModuleData->hMemoryFileSystem > -1, "Memory file filesystem is not available...");
			if(pModuleData->hMemoryFileSystem < 0)
				MFDebug_Error("Memory file filesystem is not available...");
			return pModuleData->hMemoryFileSystem;
		case MFFSH_CachedFileSystem:
			MFDebug_Assert(pModuleData->hCachedFileSystem > -1, "Memory file filesystem is not available...");
			if(pModuleData->hCachedFileSystem < 0)
				MFDebug_Error("Cached file filesystem is not available...");
			return pModuleData->hCachedFileSystem;
		case MFFSH_ZipFileSystem:
			MFDebug_Assert(pModuleData->hZipFileSystem > -1, "Zip file filesystem is not available...");
			if(pModuleData->hZipFileSystem < 0)
				MFDebug_Error("Zip file filesystem is not available...");
			return pModuleData->hZipFileSystem;
		case MFFSH_HTTPFileSystem:
			MFDebug_Assert(pModuleData->hHTTPFileSystem > -1, "HTTP file filesystem is not available...");
			if(pModuleData->hHTTPFileSystem < 0)
				MFDebug_Error("HTTP file filesystem is not available...");
			return pModuleData->hHTTPFileSystem;
		case MFFSH_FTPFileSystem:
			MFDebug_Assert(pModuleData->hFTPFileSystem > -1, "FTP file filesystem is not available...");
			if(pModuleData->hFTPFileSystem < 0)
				MFDebug_Error("FTP file filesystem is not available...");
			return pModuleData->hFTPFileSystem;
		default:
			MFDebug_Error(MFStr("Invalid filesystem handle: %d", fileSystemHandle));
			return -1;
	}
}

MFMount* MFFileSystem_FindVolume(const char *pVolumeName)
{
	GET_MODULE_DATA(MFFileSystemState);

	for(MFMount *pT = pModuleData->pMountList; pT; pT = pT->pNext)
	{
		if(!MFString_CaseCmp(pT->volumeInfo.pVolumeName, pVolumeName))
			return pT;
	}

	return NULL;
}

static int MFFileSystem_GetNumEntries(const char *pFindPattern, bool recursive, bool flatten, size_t *pStringLengths)
{
	MFFindData findData;
	MFFind *hFind;

	int numFiles = 0;

	hFind = MFFileSystem_FindFirst(MFString::Format("%s*", pFindPattern).CStr(), &findData);

	if(hFind)
	{
		*pStringLengths += MFString_Length(findData.pSystemPath) + 1;
	}

	while(hFind)
	{
		if(MFString_Compare(findData.pFilename, ".") && MFString_Compare(findData.pFilename, "..") && MFString_Compare(findData.pFilename, ".svn"))
		{
			if(findData.attributes & MFFA_Directory)
			{
				if(recursive)
				{
					if(flatten)
					{
						MFString newPath = MFString::Format("%s%s/", pFindPattern, findData.pFilename);
						numFiles += MFFileSystem_GetNumEntries(newPath.CStr(), recursive, flatten, pStringLengths);
					}
					else
					{
						*pStringLengths += MFString_Length(findData.pFilename) + 1;
						++numFiles;
					}
				}
			}
			else
			{
				*pStringLengths += MFString_Length(findData.pFilename) + 1;
				++numFiles;
			}
		}

		if(!MFFileSystem_FindNext(hFind, &findData))
		{
			MFFileSystem_FindClose(hFind);
			hFind = NULL;
		}
	}

	return numFiles;
}

MFTOCEntry* MFFileSystem_BuildToc(const char *pFindPattern, MFTOCEntry *pToc, MFTOCEntry *pParent, char* &pStringCache, bool recursive, bool flatten)
{
	MFFindData findData;
	MFFind *hFind;

	hFind = MFFileSystem_FindFirst(MFString::Format("%s*", pFindPattern).CStr(), &findData);

	char *pCurrentDir = pStringCache;

	if(hFind)
	{
		MFString_Copy(pCurrentDir, findData.pSystemPath);
		pStringCache += MFString_Length(findData.pSystemPath) + 1;
	}

	while(hFind)
	{
		if(MFString_Compare(findData.pFilename, ".") && MFString_Compare(findData.pFilename, "..") && MFString_Compare(findData.pFilename, ".svn"))
		{
			if(findData.attributes & MFFA_Directory)
			{
				if(recursive)
				{
					MFString newPath = MFString::Format("%s%s/", pFindPattern, findData.pFilename);
					if(flatten)
					{
						pToc = MFFileSystem_BuildToc(newPath.CStr(), pToc, pParent, pStringCache, recursive, flatten);
					}
					else
					{
						size_t stringCacheSize = 0;
						pToc->size = MFFileSystem_GetNumEntries(newPath.CStr(), recursive, flatten, &stringCacheSize);

						if(pToc->size)
						{
							MFString_Copy(pStringCache, findData.pFilename);
							pToc->pName = pStringCache;
							pStringCache += MFString_Length(findData.pFilename)+1;

							pToc->flags = MFTF_Directory;
							pToc->pFilesysData = pCurrentDir;
							pToc->pParent = pParent;
							pToc->size = 0;

							size_t sizeOfToc = sizeof(MFTOCEntry)*pToc->size;
							pToc->pChild = (MFTOCEntry*)MFHeap_Alloc(sizeof(MFTOCEntry)*sizeOfToc + stringCacheSize);

							char *pNewStringCache = ((char*)pToc->pChild)+sizeOfToc;
							MFFileSystem_BuildToc(newPath.CStr(), pToc->pChild, pToc, pNewStringCache, recursive, flatten);

							++pToc;
						}
					}
				}
			}
			else
			{
				MFString_Copy(pStringCache, findData.pFilename);
				pToc->pName = pStringCache;
				pStringCache += MFString_Length(findData.pFilename)+1;

				pToc->pFilesysData = pCurrentDir;

				pToc->pParent = pParent;
				pToc->pChild = NULL;

				MFDebug_Assert(findData.fileSize < 0x100000000LL, "Files larger than 4gb not yet supported...");
				pToc->size = (uint32)findData.fileSize;
				pToc->flags = 0;

				++pToc;
			}
		}

		if(!MFFileSystem_FindNext(hFind, &findData))
		{
			MFFileSystem_FindClose(hFind);
			hFind = NULL;
		}
	}

	return pToc;
}

int MFFileSystem_AddVolume(MFMount *pMount)
{
	GET_MODULE_DATA(MFFileSystemState);

	// hook it up..
	if(!pModuleData->pMountList)
	{
		pModuleData->pMountList = pModuleData->pMountListEnd = pMount;
		pMount->pPrev = pMount->pNext = NULL;
	}
	else
	{
		MFMount *pT = pModuleData->pMountList;

		while(pT && pT->volumeInfo.priority < pMount->volumeInfo.priority)
			pT = pT->pNext;

		if(pT)
		{
			if(pT == pModuleData->pMountList)
				pModuleData->pMountList = pMount;

			pMount->pPrev = pT->pPrev;
			pMount->pNext = pT;
			pT->pPrev = pMount;

			if(pMount->pPrev)
				pMount->pPrev->pNext = pMount;
		}
		else
		{
			pMount->pPrev = pModuleData->pMountListEnd;
			pModuleData->pMountListEnd->pNext = pMount;
			pModuleData->pMountListEnd = pMount;
		}
	}

	// build toc
	if(!(pMount->volumeInfo.flags & MFMF_DontCacheTOC))
	{
		MFDebug_Assert(pModuleData->ppFileSystemList[pMount->volumeInfo.fileSystem]->callbacks.FindFirst, "Filesystem must provide a set of find functions to cache the TOC");

		MFFindData findData;
		MFFind *hFind;

		bool flatten = (pMount->volumeInfo.flags & MFMF_FlattenDirectoryStructure) != 0;
		bool recursive = (pMount->volumeInfo.flags & MFMF_Recursive) != 0;

		pMount->volumeInfo.flags |= MFMF_DontCacheTOC;

		const char *pFindPath = MFStr("%s:", pMount->volumeInfo.pVolumeName);

		// this is a crude way to check if the directory exists..
		// TODO: improve this!!
		hFind = MFFileSystem_FindFirst(MFStr("%s*", pFindPath), &findData);

		if(!hFind)
		{
			MFDebug_Warn(1, "FileSystem: Couldnt Mount FileSystem.");
			return -1;
		}

		MFFileSystem_FindClose(hFind);

		// build the TOC
		size_t stringCacheSize = 0;
		pMount->numFiles = MFFileSystem_GetNumEntries(pFindPath, recursive, flatten, &stringCacheSize);

		int sizeOfToc = sizeof(MFTOCEntry)*pMount->numFiles;
		MFTOCEntry *pTOC = (MFTOCEntry*)MFHeap_Alloc(sizeOfToc + stringCacheSize);

		char *pStringCache = (char*)pTOC + sizeOfToc;
		MFFileSystem_BuildToc(pFindPath, pTOC, NULL, pStringCache, recursive, flatten);

		pMount->pEntries = pTOC;

		pMount->volumeInfo.flags &= ~MFMF_DontCacheTOC;
	}

	return 0;
}

// mount a filesystem
MF_API int MFFileSystem_Mount(MFFileSystemHandle fileSystem, MFMountData *pMountData)
{
	GET_MODULE_DATA(MFFileSystemState);

	MFMount *pMount;
	pMount = (MFMount*)MFHeap_AllocAndZero(sizeof(MFMount) + MFString_Length(pMountData->pMountpoint) + 1);

	pMount->volumeInfo.flags = pMountData->flags;
	pMount->volumeInfo.fileSystem = fileSystem;
	pMount->volumeInfo.priority = pMountData->priority;
	pMount->volumeInfo.pVolumeName = (const char*)&pMount[1];
	MFString_Copy((char*)&pMount[1], pMountData->pMountpoint);

	// call the mount callback
	int result = pModuleData->ppFileSystemList[fileSystem]->callbacks.FSMount(pMount, pMountData);

	if(result < 0)
	{
		MFHeap_Free(pMount);
		return -1;
	}

	return MFFileSystem_AddVolume(pMount);
}

MF_API int MFFileSystem_MountFujiPath(const char *pMountpoint, const char *pFujiPath, int priority, uint32 flags)
{
	const char *pColon = MFString_Chr(pFujiPath, ':');
	if(!pColon)
		return -1; // not a fuji path. needs a volume name!

	MFMount *pMount = MFFileSystem_FindVolume(MFStrN(pFujiPath, pColon-pFujiPath));

	if(!pMount)
		return -2; // volume not mounted

	// get the path
//	++pColon;
//	const char *pNewPath = MFStr("%s%s", pColon, MFString_EndsWith(pColon, "/") ? "" : "/");

	MFMount *pNew = (MFMount*)MFHeap_Alloc(sizeof(MFMount) + MFString_Length(pMountpoint) + 1);
	MFCopyMemory(pNew, pMount, sizeof(MFMount));

	pNew->volumeInfo.pVolumeName = (const char*)&pNew[1];
	MFString_Copy((char*)pNew->volumeInfo.pVolumeName, pMountpoint);

	pNew->volumeInfo.priority = priority;
	pNew->volumeInfo.flags = (pNew->volumeInfo.flags & ~MFMF_DontCacheTOC) | flags;

	return MFFileSystem_AddVolume(pNew);
}

MF_API int MFFileSystem_Dismount(const char *pMountpoint)
{
	GET_MODULE_DATA(MFFileSystemState);

	MFMount *pT = MFFileSystem_FindVolume(pMountpoint);

	if(pT)
	{
		// dismount
		pModuleData->ppFileSystemList[pT->volumeInfo.fileSystem]->callbacks.FSDismount(pT);

		// remove mount
		if(pModuleData->pMountList == pT)
			pModuleData->pMountList = pT->pNext;
		if(pModuleData->pMountListEnd == pT)
			pModuleData->pMountListEnd = pT->pPrev;

		if(pT->pNext)
			pT->pNext->pPrev = pT->pPrev;
		if(pT->pPrev)
			pT->pPrev->pNext = pT->pNext;

		MFHeap_Free(pT);

		return 0;
	}

	// coundnt find mount..
	return -1;
}

MFTOCEntry *MFFileSystem_GetTocEntry(const char *pFilename, MFTOCEntry *pEntry, int numEntries)
{
	MFCALLSTACK;

	const char *pSearchString = pFilename;
	size_t nameLen = MFString_Length(pFilename);

	bool isDirectory = false;
	for(size_t a=0; a<nameLen; a++)
	{
		if(pFilename[a] == '/')
		{
			isDirectory = true;
			pSearchString = MFStrN(pFilename, a);
			pFilename += a+1;
			break;
		}
	}

	for(int a=0; a<numEntries; a++)
	{
		if(!MFString_CaseCmp(pSearchString, pEntry[a].pName))
		{
			if(isDirectory)
			{
				if(pEntry[a].flags & MFTF_Directory)
				{
					return MFFileSystem_GetTocEntry(pFilename, pEntry[a].pChild, pEntry[a].size);
				}
			}
			else
			{
				if(!(pEntry[a].flags & MFTF_Directory))
				{
					return &pEntry[a];
				}
			}
		}
	}

	return NULL;
}

void MFFileSystem_ReleaseToc(MFTOCEntry *pEntry, int numEntries)
{
	MFCALLSTACK;

	for(int a=0; a<numEntries; a++)
	{
		if(pEntry[a].flags & MFTF_Directory)
		{
			MFFileSystem_ReleaseToc(pEntry[a].pChild, pEntry[a].size);
		}
	}

	MFHeap_Free(pEntry);
}

// open a file from the mounted filesystem stack
MF_API MFFile* MFFileSystem_Open(const char *pFilename, uint32 openFlags)
{
	MFDebug_Log(5, MFStr("Call: MFFileSystem_Open(\"%s\", 0x%x)", pFilename, openFlags));

	GET_MODULE_DATA(MFFileSystemState);

	MFMount *pMount = pModuleData->pMountList;
	const char *pMountpoint = NULL;

	// search for a mountpoint
	size_t len = MFString_Length(pFilename);
	for(size_t a=0; a<len; a++)
	{
		if(pFilename[a] == ':')
		{
			pMountpoint = MFStrN(pFilename, a);
			pFilename += a+1;
			break;
		}

		if(pFilename[a] == '.')
		{
			// if we have found a dot, this cant be a mountpoint
			// (mountpoints may only be alphanumeric)
			break;
		}
	}

	// search for file through the mount list...
	while(pMount)
	{
		int onlyexclusive = pMount->volumeInfo.flags & MFMF_OnlyAllowExclusiveAccess;

		if((!pMountpoint && !onlyexclusive) || (pMountpoint && !MFString_CaseCmp(pMountpoint, pMount->volumeInfo.pVolumeName)))
		{
			// open the file from a mount
			MFFile *hFile = pModuleData->ppFileSystemList[pMount->volumeInfo.fileSystem]->callbacks.FSOpen(pMount, pFilename, openFlags);

			if(hFile)
				return hFile;
		}

		pMount = pMount->pNext;
	}

	return NULL;
}

// read/write a file to a filesystem
MF_API char* MFFileSystem_Load(const char *pFilename, size_t *pBytesRead, size_t extraBytes)
{
	char *pBuffer = NULL;

	MFFile *hFile = MFFileSystem_Open(pFilename, MFOF_Read|MFOF_Binary);

	if(hFile)
	{
		int64 size = MFFile_GetSize(hFile);

		if(size > 0)
		{
			pBuffer = (char*)MFHeap_Alloc((size_t)size + extraBytes);

			int bytesRead = MFFile_Read(hFile, pBuffer, (size_t)size);

			if(extraBytes > 0)
				pBuffer[size] = 0;

			if(pBytesRead)
				*pBytesRead = (size_t)bytesRead;
		}

		MFFile_Close(hFile);
	}

	return pBuffer;
}

MF_API int MFFileSystem_Save(const char *pFilename, const char *pBuffer, size_t size)
{
	MFDebug_Log(5, MFStr("Call: MFFileSystem_Save(\"%s\")", pFilename));

	int bytesWritten = 0;

	MFFile *hFile = MFFileSystem_Open(pFilename, MFOF_Write|MFOF_Binary|MFOF_CreateDirectory);
	if(hFile)
	{
		bytesWritten = MFFile_Write(hFile, pBuffer, size, false);

		MFFile_Close(hFile);
	}

	return bytesWritten;
}

// if file does not exist, GetSize returns 0, however, a zero length file can also return 0 use 'Exists' to confirm
MF_API int64 MFFileSystem_GetSize(const char *pFilename)
{
	MFCALLSTACK;

	int64 size = 0;

	MFFile *hFile = MFFileSystem_Open(pFilename, MFOF_Read|MFOF_Binary);

	if(hFile)
	{
		size = MFFile_GetSize(hFile);
		MFFile_Close(hFile);
	}

	return size;
}

// returns true if the file can be found within the mounted filesystem stack
MF_API bool MFFileSystem_Exists(const char *pFilename)
{
	MFCALLSTACK;

	bool exists = false;

	MFFile *hFile = MFFileSystem_Open(pFilename, MFOF_Read|MFOF_Binary);

	if(hFile)
	{
		exists = true;
		MFFile_Close(hFile);
	}

	return exists;
}

MF_API const char *MFFileSystem_ResolveSystemPath(const char *pFilename, bool bAbsolute)
{
	MFFile *pFile = MFFileSystem_Open(pFilename, MFOF_Read|MFOF_Binary);

	const char *pPath = NULL;
	if(pFile)
	{
		GET_MODULE_DATA(MFFileSystemState);

		// TODO: support other filesystems that forward to the native filesystem (like cache filesystem?)
		if(pFile->filesystem == pModuleData->hNativeFileSystem)
			pPath = MFStr(pFile->fileIdentifier);
		MFFile_Close(pFile);
	}

	// convert to absolute...
	if(bAbsolute)
		pPath = MFFileNative_MakeAbsolute(pPath);

	return pPath;
}

MF_API int MFFileSystem_GetNumVolumes()
{
	GET_MODULE_DATA(MFFileSystemState);

	int numVolumes = 0;

	MFMount *pMount = pModuleData->pMountList;

	while(pMount)
	{
		++numVolumes;
		pMount = pMount->pNext;
	}

	return numVolumes;
}

MF_API void MFFileSystem_GetVolumeInfo(int volumeID, MFVolumeInfo *pVolumeInfo)
{
	GET_MODULE_DATA(MFFileSystemState);

	MFMount *pMount = pModuleData->pMountList;

	while(pMount && volumeID)
	{
		--volumeID;
		pMount = pMount->pNext;
	}

	MFDebug_Assert(pMount, "Invalid volume ID");

	*pVolumeInfo = pMount->volumeInfo;
}

MF_API MFFind* MFFileSystem_FindFirst(const char *pSearchPattern, MFFindData *pFindData)
{
	GET_MODULE_DATA(MFFileSystemState);

	const char *pMountpoint = NULL;
	MFFind *pFind = NULL;

	// search for a mountpoint
	const char *pColon = MFString_Chr(pSearchPattern, ':');
	if(pColon)
	{
		pMountpoint = MFStrN(pSearchPattern, pColon - pSearchPattern);
		pSearchPattern = pColon + 1;
	}

	MFDebug_Assert(pMountpoint, "A volume name must be specified in the search pattern.");

	// find the volume
	MFMount *pMount = MFFileSystem_FindVolume(pMountpoint);
	if(!pMount)
	{
		MFDebug_Warn(2, MFStr("MFFileSystem_FindFirst: Volume '%s' in not mounted.", pMountpoint));
		return NULL;
	}

	// search for file through the mount list...
	if(!(pMount->volumeInfo.flags & MFMF_DontCacheTOC))
	{
		if(pMount->numFiles)
		{
			pFind = pModuleData->gFinds.Create();

			pFind->pMount = pMount;
			MFString_Copy(pFind->searchPattern, pSearchPattern);

			size_t file = 0;
			for(; file < pFind->pMount->numFiles; ++file)
			{
				if(MFString_PatternMatch(pSearchPattern, pMount->pEntries[file].pName))
					break;
			}

			if(file == pFind->pMount->numFiles)
			{
				pModuleData->gFinds.Destroy(pFind);
				pFind = NULL;
			}
			else
			{
				pFind->pFilesystemData = (void*)file;

				MFString_Copy(pFindData->pFilename, pMount->pEntries[file].pName);
				MFString_Copy(pFindData->pSystemPath, (char*)pMount->pEntries[file].pFilesysData);

				pFindData->attributes = ((pMount->pEntries[file].flags & MFTF_Directory) ? MFFA_Directory : 0) |
										((pMount->pEntries[file].flags & MFTF_SymbolicLink) ? MFFA_SymLink : 0) |
										((pMount->pEntries[file].flags & MFTF_Hidden) ? MFFA_Hidden : 0);
				pFindData->fileSize = pMount->pEntries[file].size;
			}
		}
	}
	else
	{
		pFind = pModuleData->gFinds.Create();

		pFind->pMount = pMount;
		pFind->pFilesystemData = NULL;
		MFString_Copy(pFind->searchPattern, pSearchPattern);

		if(!pModuleData->ppFileSystemList[pMount->volumeInfo.fileSystem]->callbacks.FindFirst(pFind, pSearchPattern, pFindData))
		{
			pModuleData->gFinds.Destroy(pFind);
			pFind = NULL;
		}
	}

	return pFind;
}

MF_API bool MFFileSystem_FindNext(MFFind *pFind, MFFindData *pFindData)
{
	GET_MODULE_DATA(MFFileSystemState);

	if(!(pFind->pMount->volumeInfo.flags & MFMF_DontCacheTOC))
	{
		size_t id = (size_t)pFind->pFilesystemData + 1;

		for(; id < pFind->pMount->numFiles; ++id)
		{
			if(MFString_PatternMatch(pFind->searchPattern, pFind->pMount->pEntries[id].pName))
				break;
		}

		if(id < pFind->pMount->numFiles)
		{
			MFString_Copy(pFindData->pFilename, pFind->pMount->pEntries[id].pName);
			MFString_Copy(pFindData->pSystemPath, (char*)pFind->pMount->pEntries[id].pFilesysData);

			pFindData->attributes = ((pFind->pMount->pEntries[id].flags & MFTF_Directory) ? MFFA_Directory : 0) |
									((pFind->pMount->pEntries[id].flags & MFTF_SymbolicLink) ? MFFA_SymLink : 0) |
									((pFind->pMount->pEntries[id].flags & MFTF_Hidden) ? MFFA_Hidden : 0);
			pFindData->fileSize = pFind->pMount->pEntries[id].size;

			pFind->pFilesystemData = (void*)id;
		}
		else
			return false;
	}
	else
		return pModuleData->ppFileSystemList[pFind->pMount->volumeInfo.fileSystem]->callbacks.FindNext(pFind, pFindData);

	return true;
}

MF_API void MFFileSystem_FindClose(MFFind *pFind)
{
	GET_MODULE_DATA(MFFileSystemState);

	if(pFind->pMount->volumeInfo.flags & MFMF_DontCacheTOC)
		pModuleData->ppFileSystemList[pFind->pMount->volumeInfo.fileSystem]->callbacks.FindClose(pFind);

	pModuleData->gFinds.Destroy(pFind);
}

MF_API MFFile* MFFile_CreateMemoryFile(const void *pMemory, size_t size, bool writable, bool ownMemory)
{
	MFOpenDataMemory memory;
	memory.cbSize = sizeof(MFOpenDataMemory);
	memory.openFlags = MFOF_Read|MFOF_Binary|(writable ? MFOF_Write : 0);
	memory.pMemoryPointer = (void*)pMemory;
	memory.fileSize = size;
	memory.allocated = size;
	memory.ownsMemory = false;

	return MFFile_Open(MFFileSystem_GetInternalFileSystemHandle(MFFSH_MemoryFileSystem), &memory);
}
