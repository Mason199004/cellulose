#include <malloc.h>
#include <mem.h>
#include "Arena.h"

#define u64 uint64_t
#define i32 int32_t
#define u8 uint8_t

void Arena_init(Arena* arena, int DataSize)
{
    memset(arena, 0, sizeof(Arena));
	arena->Data = malloc(DataSize);
}

void* Arena_alloc(Arena* arena, u64 size)
{
	u64 idx = arena->PtrCount++;
	void* temp = realloc(arena->Pointers, arena->PtrCount * sizeof(void*));
	if (temp == NULL) return NULL;
	arena->Pointers = temp;

	//void* temp2 = realloc(arena->Data, arena->DataSize + size);
	//if (temp2 == NULL) return NULL;
	//arena->Data = temp2;

	void* temp3 = realloc(arena->FreedPtrs, arena->PtrCount * sizeof(u8*));
	if (temp3 == NULL) return NULL;
	arena->FreedPtrs = temp3;
	arena->FreedPtrs[arena->PtrCount - 1] = 0;

	u64 ptr = arena->DataSize + (u64)arena->Data;
	arena->DataSize += size;

	arena->Pointers[idx] = (void *) ptr;
	return (void*)ptr;
}

i32 Arena_free(Arena* arena, void* ptr)
{
	u64 idx = 0;
	for (int i = 0; i < arena->PtrCount; ++i) {
		if (arena->Pointers[i] == ptr)
		{
			idx = i;
		}
	}
	if (idx == 0) return -1;

	if (arena->PtrCount - 1 == idx)
	{
		u64 size = (arena->DataSize - (ptr - (void*)arena->Data));
		arena->DataSize -= size;
		//void* temp = realloc(arena->Data, arena->DataSize);
		//if (temp == NULL) return -1;
		//arena->Data = temp;

		void* temp2 = realloc(arena->Pointers, --arena->PtrCount * sizeof(void*));
		if (temp2 == NULL) return -1;
		arena->Pointers = temp2;

		void* temp3 = realloc(arena->FreedPtrs, arena->PtrCount * sizeof(u8*));
		if (temp3 == NULL) return -1;
		arena->FreedPtrs = temp3;
	}
	else
	{
		u64 size = arena->Pointers[idx + 1] - ptr;

		memset(ptr, 0, size);
		arena->FreedPtrs[idx] = 1;
	}
	return 0;
}

void* Arena_realloc(Arena* arena, void* ptr, u64 newSize)
{
	u64 idx = 0;
	for (int i = 0; i < arena->PtrCount; ++i) {
		if (arena->Pointers[i] == ptr)
		{
			idx = i;
		}
	}
	if (idx == 0) return NULL;

	if (idx == arena->PtrCount - 1)
	{
		u64 size = (arena->DataSize - (ptr - (void*)arena->Data));
		u8 temp[size];
		memcpy(ptr, temp, size);

		Arena_free(arena, ptr);
		void* newPtr = Arena_alloc(arena, newSize);
		if (newPtr == NULL) return NULL;

		memcpy(newPtr, temp, (newSize - size < 0) ? newSize : size);
		return newPtr;
	}
	else
	{
		u64 size = arena->Pointers[idx + 1] - ptr;
		u8 temp[size];
		memcpy(temp, ptr, size);

		Arena_free(arena, ptr);
		void* newPtr = Arena_alloc(arena, newSize);
		if (newPtr == NULL) return NULL;

		memcpy(newPtr, temp, (newSize - size < 0) ? newSize : size);
		return newPtr;
	}
}

i32 Arena_RemoveSingle(Arena* arena, u64 idx)
{
	if (idx == arena->PtrCount - 1)
	{
		u64 size = (arena->DataSize - (arena->Pointers[idx] - (void*)arena->Data));
		arena->DataSize -= size;
		//void* temp = realloc(arena->Data, arena->DataSize);
		//if (temp == NULL) return -1;
		//arena->Data = temp;

		void* temp2 = realloc(arena->Pointers, --arena->PtrCount * sizeof(void*));
		if (temp2 == NULL) return -1;
		arena->Pointers = temp2;
	}
	else
	{
		u64 size = arena->Pointers[idx] - arena->Pointers[idx + 1];
		void* idxPtr = arena->Pointers[idx];
		void* nextPtr = arena->Pointers[idx + 1];
		for (u64 i = idx + 1; i < arena->PtrCount; ++i) {
			arena->Pointers[i] -= size;
		}

		u64 afterSize = arena->DataSize - size - (idxPtr - (void*)arena->Data);
		for (int i = 0; i < afterSize; ++i) {
			((u8*)(idxPtr))[i] = ((u8*)(nextPtr))[i];
		}
		for (int i = 0; i < arena->PtrCount - 1 - idx; ++i) {
			arena->Pointers[idx + i] = arena->Pointers[idx + 1 + i];
		}
		for (int i = 0; i < arena->PtrCount - 1 - idx; ++i) {
			arena->FreedPtrs[idx + i] = arena->FreedPtrs[idx + 1 + i];
		}

		arena->PtrCount--;
		arena->DataSize -= size;

		//void* temp = realloc(arena->Data, arena->DataSize);
		void* temp2 = realloc(arena->Pointers, arena->PtrCount * sizeof(void*));
		void* temp3 = realloc(arena->FreedPtrs, arena->PtrCount * sizeof(u8*));

		if (temp2 == NULL || temp3 == NULL) return -1;
		//arena->Data = temp;
		arena->Pointers = temp2;
		arena->FreedPtrs = temp3;
	}
	return 0;
}

ArenaPtrMap* Arena_Compact(Arena* arena)
{
	u64 finalCount = 0;
	for (int i = 0; i < arena->PtrCount; ++i)
	{
		if (!arena->FreedPtrs[i])
		{
			finalCount++;
		}
	}
	ArenaPtrMap* map = malloc(sizeof(ArenaPtrMap) + (sizeof(ArenaPtrMapKV) * finalCount));
    map->KvCount = finalCount;

	for (int i = 0; i < arena->PtrCount; ++i) {
		if (!arena->FreedPtrs[i])
		{
			map->items[i].oldPtr = arena->Pointers[i];
		}
	}
	u64 i = 0;
	while (i < finalCount)
	{
		if (arena->FreedPtrs[i])
		{
			Arena_RemoveSingle(arena, i);
		}
		else
		{
			i++;
		}
	}

	for (int j = 0; j < arena->PtrCount; ++j)
	{
		map->items[j].newPtr = arena->Pointers[j];
	}

	return map;
}