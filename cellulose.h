#ifndef CELLULOSE_LIBRARY_H
#define CELLULOSE_LIBRARY_H

#include "Arena.h"
#include <stdint.h>
#include <stdio.h>

typedef struct {
	char Name[16];
	void* ValueOrPointer;
} PakNode;

typedef struct {
	char MAGIC[5];
	uint64_t reserved;
	Arena arena;
	uint64_t NodeCount;
	PakNode nodes[];
} CPak;

struct ReadMap
{
	char Name[16];
	PakNode (*Func)(PakNode* node, FILE* file, ArenaPtrMap* map);
};

struct WriteMap
{
	char Name[16];
	uint64_t (*Func)(PakNode* node, FILE* file, uint64_t DataLoc, ArenaPtrMap* map);
};

typedef struct {
	struct ReadMap* ReadMap;
	struct WriteMap* WriteMap;
	long Count;
	bool HasAny;
} NodeMap;

int32_t LoadPak(const char* path, CPak* proj, NodeMap* map);

int32_t SavePak(const char* path, CPak* proj, NodeMap* map);

#endif //CELLULOSE_LIBRARY_H
