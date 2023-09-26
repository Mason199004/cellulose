#ifndef MAPLE_ARENA_H
#define MAPLE_ARENA_H
#include <stdint.h>

typedef struct {
	uint64_t DataSize;
	uint64_t PtrCount;
	uint8_t* FreedPtrs;
	void** Pointers;
	uint8_t* Data;
} Arena;

typedef struct {
	void* oldPtr;
	void* newPtr;
} ArenaPtrMapKV;

typedef struct {
	uint64_t KvCount;
	ArenaPtrMapKV items[];
} ArenaPtrMap;

void* Arena_alloc(Arena* arena, uint64_t size);

int32_t Arena_free(Arena* arena, void* ptr);

void* Arena_realloc(Arena* arena, void* ptr, uint64_t newSize);

ArenaPtrMap* Arena_Compact(Arena* arena);

void Arena_init(Arena* arena);

#endif //MAPLE_ARENA_H
