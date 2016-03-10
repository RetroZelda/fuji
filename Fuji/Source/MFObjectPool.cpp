#include "Fuji_Internal.h"
#include "MFHeap.h"
#include "MFObjectPool.h"

void MFObjectPool::Init(size_t _objectSize, int numObjects, int growObjects, void *_pMemory, size_t _bytes)
{
	objectSize = _objectSize;
	maxItems = numObjects;
	grow = growObjects;
	allocated = 0;

	bytes = _objectSize * numObjects;
	if(_pMemory)
	{
		MFDebug_Assert((uint32)bytes <= _bytes, "Supplied allocation is too small!");
		pMemory = (char*)_pMemory;
		bOwnMemory = false;
	}
	else
	{
		pMemory = (char*)MFHeap_Alloc(bytes + sizeof(void**)*numObjects);
		bOwnMemory = true;
	}

	ppItems = (void**)(pMemory + bytes);
	for(int a=0; a<numObjects; ++a)
		ppItems[a] = pMemory + _objectSize*a;

	pNext = NULL;

	mutex = MFThread_CreateMutex("MFObjectPool alloc mutex");
}

void MFObjectPool::Deinit()
{
	if(!pMemory)
		return;

	MFThread_LockMutex(mutex);

	if(pNext)
	{
		pNext->Deinit();
		MFHeap_Free(pNext);
		pNext = NULL;
	}

	MFThread_DestroyMutex(mutex);

	if(bOwnMemory)
		MFHeap_Free(pMemory);
	pMemory = NULL;
}

void *MFObjectPool::Alloc()
{
	MFThread_LockMutex(mutex);

	void *pAlloc = NULL;

	if(allocated < maxItems)
	{
		pAlloc = ppItems[allocated++];
	}
	else
	{
		if(pNext)
		{
			pAlloc = pNext->Alloc();
		}
		else if(grow)
		{
			MFHeap *pHeap = MFHeap_GetAllocHeap(pMemory);
			MFHeap *pOld = MFHeap_SetActiveHeap(pHeap);
			pNext = (MFObjectPool*)MFHeap_Alloc(sizeof(MFObjectPool));
			pNext->Init(objectSize, grow, grow);
			MFHeap_SetActiveHeap(pOld);
			pAlloc = pNext->Alloc();
		}
	}

	MFThread_ReleaseMutex(mutex);

	return pAlloc;
}

void *MFObjectPool::AllocAndZero()
{
	void *pNew = Alloc();
	if(pNew)
		MFZeroMemory(pNew, objectSize);
	return pNew;
}

int MFObjectPool::Free(void *pItem)
{
	MFThread_LockMutex(mutex);

	int bFreed = 0;
	if(pItem >= pMemory && pItem < pMemory + bytes)
	{
		for(int a=0; a<allocated; ++a)
		{
			if(ppItems[a] == pItem)
			{
				void *pI = ppItems[a];
				ppItems[a] = ppItems[--allocated];
				ppItems[allocated] = pI;

				bFreed = 1;
				break;
			}
		}
	}

	if(!bFreed && pNext)
		bFreed = pNext->Free(pItem);

	MFThread_ReleaseMutex(mutex);

	return bFreed;
}

bool MFObjectPool::Owns(const void *pItem) const
{
	const MFObjectPool *pThis = this;

	while(pThis)
	{
		if(pItem >= pThis->pMemory && pItem < pThis->pMemory + pThis->bytes)
			return true;
		pThis = pThis->pNext;
	}
	return false;
}

size_t MFObjectPool::GetTotalMemory() const
{
	return objectSize * GetNumReserved();
}

size_t MFObjectPool::GetAllocatedMemory() const
{
	return objectSize * GetNumAllocated();
}

size_t MFObjectPool::GetOverheadMemory() const
{
	return sizeof(void**) * GetNumReserved() + sizeof(*this);
}

int MFObjectPool::GetNumReserved() const
{
	return maxItems + (pNext ? pNext->GetNumReserved() : 0);
}

int MFObjectPool::GetNumAllocated() const
{
	return allocated + (pNext ? pNext->GetNumAllocated() : 0);
}

void *MFObjectPool::GetItem(int index) const
{
	if(index < allocated)
		return ppItems[index];

	if(pNext)
		return pNext->GetItem(index - allocated);

	return NULL;
}

void MFObjectPoolGroup::Init(const MFObjectPoolGroupConfig *_pPools, int _numPools)
{
	pConfig = (MFObjectPoolGroupConfig*)MFHeap_Alloc(sizeof(MFObjectPoolGroupConfig)*_numPools + sizeof(MFObjectPool)*_numPools);
	pPools = (MFObjectPool*)&pConfig[_numPools];
	numPools = _numPools;

	MFCopyMemory(pConfig, _pPools, sizeof(MFObjectPoolGroupConfig)*_numPools);

	mutex = MFThread_CreateMutex("MFObjectPoolGroup alloc mutex");

	for(int a=0; a<_numPools; ++a)
	{
		pPools[a].Init(pConfig[a].objectSize, pConfig[a].numObjects, pConfig[a].growObjects);
	}
}

void MFObjectPoolGroup::Deinit()
{
	MFThread_LockMutex(mutex);

	for(int a=0; a<numPools; ++a)
		pPools[a].Deinit();

	MFThread_DestroyMutex(mutex);

	MFHeap_Free(pConfig);
}

void *MFObjectPoolGroup::Alloc(size_t bytes, size_t *pAllocated)
{
	MFThread_LockMutex(mutex);

	void *pAlloc = NULL;

	for(int a=0; a<numPools; ++a)
	{
		if(bytes <= pConfig[a].objectSize)
		{
			if(pAllocated)
				*pAllocated = pConfig[a].objectSize;
			pAlloc = pPools[a].Alloc();
			break;
		}
	}

	if(!pAlloc)
	{
		++overflows;
		if(pAllocated)
			*pAllocated = bytes;
		MFHeap *pHeap = MFHeap_GetAllocHeap(pConfig);
		pAlloc = MFHeap_Alloc(bytes, pHeap);
	}

	MFThread_ReleaseMutex(mutex);

	return pAlloc;
}

void *MFObjectPoolGroup::AllocAndZero(size_t bytes, size_t *pAllocated)
{
	size_t size = 0;
	void *pNew = Alloc(bytes, &size);
	if(pNew)
		MFZeroMemory(pNew, size);
	if(pAllocated)
		*pAllocated = size;
	return pNew;
}

void MFObjectPoolGroup::Free(void *pItem)
{
	MFThread_LockMutex(mutex);

	for(int a=0; a<numPools; ++a)
	{
		if(pPools[a].Free(pItem))
			goto free_done;
	}

	--overflows;
	MFHeap_Free(pItem);

free_done:
	MFThread_ReleaseMutex(mutex);
}

bool MFObjectPoolGroup::Owns(const void *pItem) const
{
	for(int a=0; a<numPools; ++a)
	{
		if(pPools[a].Owns(pItem))
			return true;
	}
	return false;
}

size_t MFObjectPoolGroup::GetTotalMemory() const
{
	size_t total = 0;
	for(int a=0; a<numPools; ++a)
		total += pPools[a].GetTotalMemory();
	return total;
}

size_t MFObjectPoolGroup::GetAllocatedMemory() const
{
	size_t allocated = 0;
	for(int a=0; a<numPools; ++a)
		allocated += pPools[a].GetAllocatedMemory();
	return allocated;
}

size_t MFObjectPoolGroup::GetOverheadMemory() const
{
	size_t overhead = 0;
	for(int a=0; a<numPools; ++a)
		overhead += pPools[a].GetOverheadMemory();
	return overhead;
}

int MFObjectPoolGroup::GetNumReserved() const
{
	int reserved = 0;
	for(int a=0; a<numPools; ++a)
		reserved += pPools[a].GetNumReserved();
	return reserved;
}

int MFObjectPoolGroup::GetNumAllocated() const
{
	int allocated = 0;
	for(int a=0; a<numPools; ++a)
		allocated += pPools[a].GetNumReserved();
	return allocated;
}
