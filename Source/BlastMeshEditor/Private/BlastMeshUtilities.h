// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "EngineDefines.h"
#include "MeshDescription.h"

#include "BlastFracture.h"


class UStaticMesh;
class UBlastMesh;
struct FSkeletalMaterial;
struct FRawMesh;
class UMaterialInterface;

namespace Nv
{
	namespace Blast
	{
		class Mesh;
		struct AuthoringResult;
	}
}

Nv::Blast::Mesh* CreateAuthoringMeshFromRawMesh(const FRawMesh& RawMesh, const FTransform3f& UE4ToBlastTransform);

Nv::Blast::Mesh* CreateAuthoringMeshFromRenderData(const FStaticMeshRenderData& RenderData,
                                                   const TMap<FName, int32>& MaterialMap,
                                                   const FTransform3f& UE4ToBlastTransform);

void CreateSkeletalMeshFromAuthoring(TSharedPtr<FFractureSession> FractureSession, const UStaticMesh& SourceMesh);

void CreateSkeletalMeshFromAuthoring(TSharedPtr<FFractureSession> FractureSession, UMaterialInterface* InteriorMaterial);
