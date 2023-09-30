#include "cellulose.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define u64 uint64_t
#define i32 int32_t
#define u8 uint8_t

u64 WriteNode(PakNode* node, FILE* file, u64 DataLoc, ArenaPtrMap* map, NodeMap* nodemap) //yes the error handling here leaks some memory, cry about it
{
	for (int i = 0; i < nodemap->Count; ++i)
	{
		if (strcmp(node->Name, nodemap->WriteMap[i].Name) == 0)
		{
			return nodemap->WriteMap[i].Func(node, file, DataLoc, map);
		}
	}
	if (nodemap->HasAny)
	{
		return nodemap->WriteMap[0].Func(node, file, DataLoc, map);
	}
	return 0;
}

PakNode ReadNode(PakNode* node, FILE* file, ArenaPtrMap* map, NodeMap* nodemap)
{
	for (int i = 0; i < nodemap->Count; ++i)
	{
		if (strcmp(node->Name, nodemap->ReadMap[i].Name) == 0)
		{
			return nodemap->ReadMap[i].Func(node, file, map);
		}
	}
	if (nodemap->HasAny)
	{
		return nodemap->ReadMap[0].Func(node, file, map);
	}
	return (PakNode){.Name = "ERROR", .ValueOrPointer = NULL};
}

i32 LoadPak(const char* path, CPak* proj, NodeMap* nodemap)
{
	FILE* file = fopen(path, "rb");
	if (file == NULL) return -1;

	if (fread(proj, sizeof(CPak), 1, file) == 0) return -1;

	if (strcmp(proj->MAGIC, "C_PAK") != 0)
	{
		return -1;
	}

	void* temp = realloc(proj, sizeof(CPak) + (sizeof(PakNode) * proj->NodeCount));
	if (temp == NULL) return -1;
	proj = temp;

	if (fread(proj->nodes, sizeof(PakNode), proj->NodeCount, file) < proj->NodeCount) return -1;

	proj->arena.Data = malloc(proj->arena.DataSize);
	if (fread(proj->arena.Data, proj->arena.DataSize, 1, file) == 0) return -1;

	proj->arena.Pointers = malloc(sizeof(void*) * proj->arena.PtrCount);
	if (fread(proj->arena.Pointers, sizeof(void*), proj->arena.PtrCount, file) < proj->arena.PtrCount) return -1;

	proj->arena.FreedPtrs = malloc(proj->arena.PtrCount);
	if (fread(proj->arena.FreedPtrs, sizeof(u8), proj->arena.PtrCount, file) < proj->arena.PtrCount) return -1;

	ArenaPtrMap* map = malloc(sizeof(ArenaPtrMap) + (sizeof(ArenaPtrMapKV) * proj->arena.PtrCount));

	for (int i = 0; i < proj->arena.PtrCount; ++i)
	{
		map->items[i].oldPtr = proj->arena.Pointers[i];
		map->items[i].newPtr = proj->arena.Data + (u64)(proj->arena.Pointers[i] - (sizeof(CPak) + (sizeof(PakNode) * proj->NodeCount)));
		proj->arena.Pointers[i] = map->items[i].newPtr;
	}

	for (int i = 0; i < proj->NodeCount; ++i)
	{
		PakNode tempnode = ReadNode(&proj->nodes[i], file, map, nodemap);
		if (strcmp(tempnode.Name, "ERROR") == 0) return -1;
		proj->nodes[i] = tempnode;
	}
	return 0;
}

i32 SavePak(const char* path, CPak* proj, NodeMap* nodemap)
{
	FILE* file = fopen(path, "wb");
	if (file == NULL) return -1;

	u64 DataLoc = sizeof(CPak) + (proj->NodeCount * sizeof(PakNode));
	ArenaPtrMap* map = Arena_Compact(&proj->arena);

	for (int i = 0; i < proj->arena.PtrCount; ++i)
	{
		for (int j = 0; j < map->KvCount; ++j)
		{
			if (proj->arena.Pointers[i] == map->items[j].newPtr)
			{
				map->items[j].newPtr = (void*)((void*)proj->arena.Data - proj->arena.Pointers[i] + DataLoc);
				proj->arena.Pointers[i] = map->items[j].newPtr;
			}
		}
	}

	u64 old = ftell(file);
	fseek(file, DataLoc, SEEK_SET);
	if (fwrite(proj->arena.Data, proj->arena.DataSize, 1, file) == 0)
	{
		fclose(file);
		return -1;
	}
	proj->arena.Data = (u8*)DataLoc;
	DataLoc += proj->arena.DataSize;
	if (fwrite(proj->arena.Pointers, proj->arena.PtrCount * sizeof(void*), 1, file) == 0)
	{
		fclose(file);
		return -1;
	}
	proj->arena.Pointers = (void**) DataLoc;
	DataLoc += proj->arena.PtrCount * sizeof(void*);
	if (fwrite(proj->arena.FreedPtrs, proj->arena.PtrCount * sizeof(u8), 1, file) == 0)
	{
		fclose(file);
		return -1;
	}
	proj->arena.FreedPtrs = (void*)DataLoc;
	DataLoc += proj->arena.PtrCount * sizeof(u8);
	fseek(file, old, SEEK_SET);

	if (fwrite(proj, sizeof(CPak), 1, file) == 0)
	{
		fclose(file);
		return -1;
	}


	for (int i = 0; i < proj->NodeCount; ++i)
	{
		u64 loc = ftell(file);
		DataLoc += WriteNode(&proj->nodes[i], file, DataLoc, map, nodemap);
		fseek(file, loc + sizeof(PakNode), SEEK_SET);
	}
	free(map);
	fclose(file);
	return 0;
}
