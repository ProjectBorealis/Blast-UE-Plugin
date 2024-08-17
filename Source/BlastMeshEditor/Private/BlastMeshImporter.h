#pragma once


#include "CoreMinimal.h"

#include "NvBlastExtAuthoringTypes.h"
#include "NvCTypes.h"

struct FMeshData
{
	// vertex positions
	TArray<NvcVec3> Verts;

	// vertex normals
	TArray<NvcVec3> Normals;

	// vertex UVs
	TArray<NvcVec2> UVs;

	// indices
	TArray<uint32> Indices;

	// per-face material IDs
	TArray<int32> MaterialIDs;

	// per-face smoothing group IDs
	TArray<int32> SmoothingGroups;
};

struct FImportedMeshData
{
	// list of all "chunks" in the mesh
	TArray<FMeshData> Chunks;

	// For each entry in Collections, there will be a corresponding ParentIndex entry, linking a specific chunk collection to its parent
	TArray<int32> ParentIndex;
	
	// list of all materials used in this mesh
	TArray<FString> MaterialNames;

	// return a list of children belonging to the specified chunk
	TArray<int32> GetListOfChildren(int32 ChunkIdx) const
	{
		TArray<int32> Result;
		for (int32 Idx = 0; Idx < ParentIndex.Num(); ++Idx)
		{
			if (ChunkIdx == ParentIndex[Idx])
			{
				Result.Add(Idx);
			}
		}
		return Result;
	}
};

/**
	An interface for Blast mesh file reader
*/
class IBlastMeshFileReader
{
public:
	/*
		Load from the specified file path
	*/
	virtual FImportedMeshData LoadFromFile(const FString& FilePath) = 0;
};

