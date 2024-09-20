#include "BlastMesh.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Animation/Skeleton.h"
#include "PhysicsEngine/BodySetup.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshModel.h"
#include "UObject/ObjectSaveContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BlastMesh)

#if WITH_EDITOR
#include "Engine/SkinnedAssetCommon.h"
#include "RawMesh.h"
#include "RawIndexBuffer.h"
#include "NvBlastGlobals.h"
#endif

#define LOCTEXT_NAMESPACE "Blast"

UBlastMesh::UBlastMesh(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer),
	Mesh(nullptr),
	Skeleton(nullptr),
	PhysicsAsset(nullptr)
{
}

bool UBlastMesh::IsValidBlastMesh()
{
	return Mesh != nullptr && PhysicsAsset != nullptr && GetLoadedAsset() != nullptr;
}

#if WITH_EDITOR

FTransform3f UBlastMesh::GetTransformUEToBlastCoordinateSystem()
{
	return GetTransformBlastToUECoordinateSystem().Inverse();
}

FTransform3f UBlastMesh::GetTransformBlastToUECoordinateSystem()
{
	//Blast coordinate system interpretation is : X = right, Y = forward, Z = up. centimeters
	//UE4 is X = forward, Y = right, Z = up, centimeters

	FTransform3f BlastToUE4Transform(FTransform::Identity);
	BlastToUE4Transform.SetRotation(FRotator3f(0, 90.0f, 0).Quaternion()); // rotate from X=right in blast to X=forward in UE
	BlastToUE4Transform.SetScale3D(FVector3f(-1.0f, 1.0f, 1.0f)); // because of rotation, the Y=forward of blast is now Y=left, so invert that
	return BlastToUE4Transform;
}

void UBlastMesh::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

#endif


void UBlastMesh::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
#if WITH_EDITORONLY_DATA
	//This is used by the reimport code to find the AssetImportData
	if (AssetImportData)
	{
		OutTags.Emplace(UObject::SourceFileTagName(), AssetImportData->GetSourceData().ToJson(), FAssetRegistryTag::TT_Hidden);
	}
#endif

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	Super::GetAssetRegistryTags(OutTags);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

void UBlastMesh::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITORONLY_DATA
	FractureHistory.GetCurrentToolData() = FractureToolData;
#endif

	//Make sure our instanced sub objects have run PostLoad so they are fully initialized before we use them
	if (Mesh)
	{
		Mesh->ConditionalPostLoad();
	}

	if (PhysicsAsset)
	{
		PhysicsAsset->ConditionalPostLoad();
	}

	if (Skeleton)
	{
		Skeleton->ConditionalPostLoad();
	}

	RebuildIndexToBoneNameMap();

#if WITH_EDITOR
	// Make sure the order corresponds to that in the asset
	RebuildCookedBodySetupsIfRequired();

	if (Mesh)
	{
		for (int32 I = 0; I < Mesh->GetMaterials().Num(); I++)
		{
			//Fix up files where this is null from the old import/fracture code
			FSkeletalMaterial& Mat = Mesh->GetMaterials()[I];
			if (Mat.MaterialSlotName.IsNone() && Mat.ImportedMaterialSlotName.IsNone())
			{
				Mat.ImportedMaterialSlotName = FName("MaterialSlot", I);
			}

			if (Mat.MaterialSlotName.IsNone())
			{
				Mat.MaterialSlotName = Mat.ImportedMaterialSlotName;
			}
			else if (Mat.ImportedMaterialSlotName.IsNone())
			{
				Mat.ImportedMaterialSlotName = Mat.MaterialSlotName;
			}
		}
	}
#endif
}

void UBlastMesh::PreSave(FObjectPreSaveContext SaveContext)
{
#if WITH_EDITOR
	//Since can only do this in the editor, just make 100% sure this up to date if we are cooking
	RebuildCookedBodySetupsIfRequired();
#endif
	Super::PreSave(SaveContext);
}

void UBlastMesh::RebuildIndexToBoneNameMap()
{
	// Building chunk to bone maps
	if (Mesh != nullptr)
	{
		const uint32 ChunkCount = GetChunkCount();
		const auto& BoneNames = GetChunkIndexToBoneName();

		ChunkIndexToBoneIndex.SetNum(ChunkCount);
		for (uint32 i = 0; i < ChunkCount; i++)
		{
			FName boneName = BoneNames[i];
			int32 boneIndex = Mesh->GetRefSkeleton().FindBoneIndex(boneName);
			ChunkIndexToBoneIndex[i] = boneIndex;
		}
	}
	else
	{
		ChunkIndexToBoneIndex.Empty();
	}
}

#if WITH_EDITOR
void UBlastMesh::RebuildCookedBodySetupsIfRequired(bool bForceRebuild)
{
	int32 BoneCount = IsValidBlastMesh() ? Mesh->GetRefSkeleton().GetRawBoneNum() : 0;
	if (bForceRebuild || BoneCount != ComponentSpaceInitialBoneTransforms.Num())
	{
		Mesh->CalculateInvRefMatrices(); //will do nothing if already cached
		ComponentSpaceInitialBoneTransforms.SetNum(BoneCount);
		for (int32 B = 0; B < BoneCount; B++)
		{
			ComponentSpaceInitialBoneTransforms[B] = FTransform(Mesh->GetComposedRefPoseMatrix(B));
		}
	}

	int32 ChunkCount = IsValidBlastMesh() ? GetChunkCount() : 0;
	if (CookedChunkData.Num() != ChunkCount)
	{
		CookedChunkData.SetNum(ChunkCount);
	}

	for (int32 ChunkIndex = 0; ChunkIndex < ChunkCount; ChunkIndex++)
	{
		FBlastCookedChunkData& CurCookedChunkData = CookedChunkData[ChunkIndex];
		int32 BoneIndex = ChunkIndexToBoneIndex[ChunkIndex];
		//Would be nice to remove the Index -> Name -> Index lookup, but the PhysicsAsset seems to require it
		const uint32 BodySetupIndex = PhysicsAsset->FindBodyIndex(Mesh->GetRefSkeleton().GetBoneName(BoneIndex));

		if (PhysicsAsset->SkeletalBodySetups.IsValidIndex(BodySetupIndex))
		{
			//Transform these ahead of time and cache since InitialBoneTransform is constant
			//Always make the initial actor at the component space origin, this allows the actor space to correspond to the at-rest position which Blast internally uses
			USkeletalBodySetup* PhysicsAssetBodySetup = PhysicsAsset->SkeletalBodySetups[BodySetupIndex];
			//Whenever this setup is changed the guid is changed
			if (bForceRebuild || CurCookedChunkData.SourceBodySetupGUID != PhysicsAssetBodySetup->BodySetupGuid)
			{
				//rebuild this one
				UBodySetup* CookedTransformedBodySetup = NewObject<UBodySetup>(this);
				CookedTransformedBodySetup->bGenerateMirroredCollision = false;
				//Copy the settings, but not the actual colliders
				CookedTransformedBodySetup->CopyBodySetupProperty(PhysicsAssetBodySetup);
				//We are on the root bone now
				CookedTransformedBodySetup->BoneName = NAME_None;

				//Copy the bodies, transforming them into bone-space
				const FTransform InitialBoneTransform = GetComponentSpaceInitialBoneTransform(BoneIndex);
				const FKAggregateGeom& SrcAggGeom = PhysicsAssetBodySetup->AggGeom;
				FKAggregateGeom& DestAggGeom = CookedTransformedBodySetup->AggGeom;

				DestAggGeom.SphereElems.Reset(SrcAggGeom.SphereElems.Num());
				for (auto& E : SrcAggGeom.SphereElems)
				{
					DestAggGeom.SphereElems.Emplace(E.GetFinalScaled(FVector(1.0f), InitialBoneTransform));
				}
				DestAggGeom.BoxElems.Reset(SrcAggGeom.BoxElems.Num());
				for (auto& E : SrcAggGeom.BoxElems)
				{
					DestAggGeom.BoxElems.Emplace(E.GetFinalScaled(FVector(1.0f), InitialBoneTransform));
				}
				DestAggGeom.SphylElems.Reset(SrcAggGeom.SphylElems.Num());
				for (auto& E : SrcAggGeom.SphylElems)
				{
					DestAggGeom.SphylElems.Emplace(E.GetFinalScaled(FVector(1.0f), InitialBoneTransform));
				}

				const int32 ConvexCount = SrcAggGeom.ConvexElems.Num();
				DestAggGeom.ConvexElems.Reset(ConvexCount);
				for (int32 C = 0; C < ConvexCount; C++)
				{
					DestAggGeom.ConvexElems.Emplace(SrcAggGeom.ConvexElems[C]);
					auto& Last = DestAggGeom.ConvexElems.Last();

					Last.SetTransform(Last.GetTransform() * InitialBoneTransform);
					//This is not strictly required, but why not for simplicity?
					Last.BakeTransformToVerts();
				}

				CookedTransformedBodySetup->CreatePhysicsMeshes();

				CurCookedChunkData.CookedBodySetup = CookedTransformedBodySetup;
				CurCookedChunkData.SourceBodySetupGUID = PhysicsAssetBodySetup->BodySetupGuid;
			}
		}
		else
		{
			//Clear out this entry
			CurCookedChunkData.SourceBodySetupGUID = FGuid();
			CurCookedChunkData.CookedBodySetup = nullptr;
		}
	}
}

TArray<FRawMesh> UBlastMesh::GetRenderMeshes(int32 LODIndex) const
{
	TArray<FRawMesh> RawMeshes;
	
	FSkeletalMeshImportData ImportData;
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	Mesh->LoadLODImportedData(LODIndex, ImportData);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	const int32 ChunkCount = GetChunkCount();
	RawMeshes.AddDefaulted(ChunkCount);
	ensure(ChunkIndexToBoneIndex.Num() == ChunkCount);

	TArray<uint32> TempBuffer;
	TArray<int32> BoneIndexToChunkIndex;
	BoneIndexToChunkIndex.Init(INDEX_NONE, ChunkCount + 1);
	for (int32 I = 0; I < ChunkCount; I++)
	{
		BoneIndexToChunkIndex[ChunkIndexToBoneIndex[I]] = I;
	}

	TArray<TSet<int32>> ChunkIndices;
	TMap<int32, int32> VertToChunkIndex;
	ChunkIndices.AddDefaulted(ChunkCount);
	VertToChunkIndex.Reserve(ImportData.Influences.Num());
	for (const auto& Influence : ImportData.Influences)
	{
		if (!BoneIndexToChunkIndex.IsValidIndex(Influence.BoneIndex))
		{
			continue;
		}

		const int32 ChunkIndex = BoneIndexToChunkIndex[Influence.BoneIndex];
		if (!RawMeshes.IsValidIndex(ChunkIndex))
		{
			continue;
		}

		ChunkIndices[ChunkIndex].Add(Influence.VertexIndex);
		VertToChunkIndex.Add(Influence.VertexIndex, ChunkIndex);
	}

	TMap<int32, int32> VertIndexToVertIndexInChunk;
	for (const auto& [VertIdx, ChunkIdx] : VertToChunkIndex)
	{
		VertIndexToVertIndexInChunk.Add(VertIdx, RawMeshes[ChunkIdx].VertexPositions.Num());
		RawMeshes[ChunkIdx].VertexPositions.Add(ImportData.Points[VertIdx]);
	}

	for (const auto& Face : ImportData.Faces)
	{
		const int32 VertIdx = ImportData.Wedges[Face.WedgeIndex[0]].VertexIndex;
		if (const int32* ChunkIdx = VertToChunkIndex.Find(VertIdx))
		{
			for (int32 Vert = 0; Vert < 3; Vert++)
			{
				const auto& Wedge = ImportData.Wedges[Face.WedgeIndex[Vert]];
				RawMeshes[*ChunkIdx].WedgeIndices.Add(VertIndexToVertIndexInChunk[Wedge.VertexIndex]);
				RawMeshes[*ChunkIdx].WedgeColors.Add(Wedge.Color);
				for (uint32 UV = 0; UV < ImportData.NumTexCoords; UV++)
				{
					RawMeshes[*ChunkIdx].WedgeTexCoords[UV].Add(Wedge.UVs[UV]);
				}
				RawMeshes[*ChunkIdx].WedgeTangentX.Add(Face.TangentX[Vert]);
				RawMeshes[*ChunkIdx].WedgeTangentY.Add(Face.TangentY[Vert]);
				RawMeshes[*ChunkIdx].WedgeTangentZ.Add(Face.TangentZ[Vert]);
			}
			RawMeshes[*ChunkIdx].FaceMaterialIndices.Add(Face.MatIndex);
			RawMeshes[*ChunkIdx].FaceSmoothingMasks.Add(Face.SmoothingGroups);
		}
	}

	return RawMeshes;
}

void UBlastMesh::CopyFromLoadedAsset(const NvBlastAsset* AssetToCopy, const FGuid& NewAssetGUID)
{
	FractureToolData = FractureHistory.GetCurrentToolData();

	Super::CopyFromLoadedAsset(AssetToCopy, NewAssetGUID);
}

#endif

const TArray<FName>& UBlastMesh::GetChunkIndexToBoneName()
{
	const uint32 ChunkCount = GetChunkCount();
	if (ChunkIndexToBoneName.Num() != ChunkCount)
	{
		ChunkIndexToBoneName.SetNum(ChunkCount);
		for (uint32 i = 0; i < ChunkCount; i++)
		{
			ChunkIndexToBoneName[i] = GetDefaultChunkBoneNameFromIndex(i);
		}
		MarkPackageDirty();
	}
	return ChunkIndexToBoneName;
}


const TArray<FBlastCookedChunkData>& UBlastMesh::GetCookedChunkData()
{
#if WITH_EDITOR
	//Maybe not the best to check this every time, but it's only in the editor
	RebuildCookedBodySetupsIfRequired();
#endif
	return CookedChunkData;
}


const TArray<FBlastCookedChunkData>& UBlastMesh::GetCookedChunkData_AssumeUpToDate() const
{
	return CookedChunkData;
}

const FString UBlastMesh::ChunkPrefix = TEXT("chunk_");

FName UBlastMesh::GetDefaultChunkBoneNameFromIndex(int32 ChunkIndex)
{
	return FName(*FString::Printf(TEXT("chunk_%d"), ChunkIndex));
}


void FBlastCookedChunkData::PopulateBodySetup(UBodySetup* NewBodySetup) const
{
	//These should already be null but just incase
	NewBodySetup->ClearPhysicsMeshes();

	//Make sure they are loaded
	CookedBodySetup->CreatePhysicsMeshes();

	//The assignment operators clear these so make sure we cache them before we touch the arrays
	ConvexMeshTempList ConvexMeshes;
	for (auto& C : CookedBodySetup->AggGeom.ConvexElems)
	{
#if BLAST_USE_PHYSX
		physx::PxConvexMesh* Mesh = C.GetConvexMesh();
		if (Mesh)
		{
			Mesh->acquireReference();
		}
		ConvexMeshes.Add(Mesh);
#else
		ConvexMeshes.Add(C.GetChaosConvexMesh());
#endif
	}

	NewBodySetup->CopyBodyPropertiesFrom(CookedBodySetup);

	UpdateAfterShapesAdded(NewBodySetup, ConvexMeshes);
}

void FBlastCookedChunkData::AppendToBodySetup(UBodySetup* NewBodySetup) const
{
	//Make sure they are loaded
	CookedBodySetup->CreatePhysicsMeshes();

	//The assignment operators clear these so make sure we cache them before we touch the arrays
	ConvexMeshTempList ConvexMeshes;
	for (auto& C : NewBodySetup->AggGeom.ConvexElems)
	{
#if BLAST_USE_PHYSX
		//Already add-refed these before
		ConvexMeshes.Add(C.GetConvexMesh());
#else
		ConvexMeshes.Add(C.GetChaosConvexMesh());
#endif
	}

	for (const auto& C : CookedBodySetup->AggGeom.ConvexElems)
	{
#if BLAST_USE_PHYSX
		physx::PxConvexMesh* Mesh = C.GetConvexMesh();
		if (Mesh)
		{
			Mesh->acquireReference();
		}
		ConvexMeshes.Add(Mesh);
#else
		ConvexMeshes.Add(C.GetChaosConvexMesh());
#endif
	}

	//Should we check the PhysicalMaterial, etc are the same
	NewBodySetup->AddCollisionFrom(CookedBodySetup);

	UpdateAfterShapesAdded(NewBodySetup, MoveTemp(ConvexMeshes));
}

void FBlastCookedChunkData::UpdateAfterShapesAdded(UBodySetup* NewBodySetup, ConvexMeshTempList ConvexMeshes)
{
	//Always make sure these get set since they are cleared on copy
	bool bAllThere = true;
	const int32 ConvexCount = NewBodySetup->AggGeom.ConvexElems.Num();
	for (int32 C = 0; C < ConvexCount; C++)
	{
		auto& New = NewBodySetup->AggGeom.ConvexElems[C];

		bAllThere &= ConvexMeshes.IsValidIndex(C) && ConvexMeshes[C];
		if (!bAllThere)
		{
			break;
		}

#if BLAST_USE_PHYSX
		New.SetConvexMesh(ConvexMeshes[C]);
#else
		New.SetConvexMeshObject(MoveTemp(ConvexMeshes[C]));
#endif
	}

	//If any are missing we need to fallback to runtime cooking
	NewBodySetup->bCreatedPhysicsMeshes = bAllThere;
}

#undef LOCTEXT_NAMESPACE
