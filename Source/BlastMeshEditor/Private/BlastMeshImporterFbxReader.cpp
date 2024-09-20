#include "BlastMeshImporterFbxReader.h"

#include "BlastMeshFbxUtils.h"

#include <fbxsdk.h>

#include "BlastGlobals.h"
#include "BlastMesh.h"

using namespace Nv::Blast;

FbxAMatrix FbxFileReader::getTransformForNode(FbxNode* node)
{
	//The geometry transform contains the information about the pivots in the mesh node relative to the node's transform
	FbxAMatrix geometryTransform(node->GetGeometricTranslation(FbxNode::eSourcePivot), node->GetGeometricRotation(FbxNode::eSourcePivot), node->GetGeometricScaling(FbxNode::eSourcePivot));
	FbxAMatrix nodeTransform = node->EvaluateGlobalTransform();

	return nodeTransform * geometryTransform;
}

FImportedMeshData FbxFileReader::LoadFromFile(const FString& FilePath)
{
	FImportedMeshData MeshData;

	// Wrap in a shared ptr so that when it deallocates we get an auto destroy and all of the other assets created don't leak.
	TSharedPtr<FbxManager> sdkManager = MakeShareable<FbxManager>(FbxManager::Create(), [=](FbxManager* manager)
	{
		manager->Destroy();
	});

	FbxIOSettings* ios = FbxIOSettings::Create(sdkManager.Get(), IOSROOT);
	// Set some properties on the io settings

	sdkManager->SetIOSettings(ios);


	FbxImporter* importer = FbxImporter::Create(sdkManager.Get(), "");

	bool importStatus = importer->Initialize(TCHAR_TO_UTF8(*FilePath), -1, sdkManager->GetIOSettings());
	if (!importStatus)
	{
		UE_LOG(LogBlastIO, Warning, TEXT("Call to FbxImporter::Initialize failed - %hs"), importer->GetStatus().GetErrorString());
		return MeshData;
	}

	FbxScene* scene = FbxScene::Create(sdkManager.Get(), "importScene");
	importStatus = importer->Import(scene);
	if (!importStatus)
	{
		UE_LOG(LogBlastIO, Warning, TEXT("Call to FbxImporter::Import failed - %hs"), importer->GetStatus().GetErrorString());
		return MeshData;
	}

	int32 matCount = scene->GetMaterialCount();

	for (int32 i = 0; i < matCount; ++i)
	{
		MeshData.MaterialNames.Add(UTF8_TO_TCHAR(scene->GetMaterial(i)->GetName()));
	}

	//This removes axis and unit conversion nodes so it converts the entire scene to the header specified axis and units
	FbxRootNodeUtility::RemoveAllFbxRoots(scene);

	FbxAxisSystem blastAxisSystem = FbxUtils::getBlastFBXAxisSystem();
	FbxAxisSystem sourceSetup = scene->GetGlobalSettings().GetAxisSystem();
	if (sourceSetup != blastAxisSystem)
	{
		UE_LOG(LogBlastIO, Log, TEXT("Converting to Blast coordinates from existing axis %s"), *FbxUtils::FbxAxisSystemToString(sourceSetup));
		blastAxisSystem.ConvertScene(scene);
	}

	FbxSystemUnit blastUnits = FbxUtils::getBlastFBXUnit();
	FbxSystemUnit sourceUnits = scene->GetGlobalSettings().GetSystemUnit();
	if (sourceUnits != blastUnits)
	{
		UE_LOG(LogBlastIO, Log, TEXT("Converting to Blast units from existing units %s"), *FbxUtils::FbxSystemUnitToString(sourceUnits));
		blastUnits.ConvertScene(scene);
	}

	FbxGeometryConverter geoConverter(sdkManager.Get());

	// Recurse the fbx tree and find all meshes
	TArray<FbxNode*> meshNodes;
	getFbxMeshes(scene->GetRootNode(), meshNodes);

	UE_LOG(LogBlastIO, Log, TEXT("Found %d meshes."), meshNodes.Num());

	if (meshNodes.IsEmpty())
	{
		return MeshData;
	}

	MeshData.Chunks.AddDefaulted(meshNodes.Num());
	MeshData.ParentIndex.Init(INDEX_NONE, meshNodes.Num());
	for (int32 NodeIdx = 0; NodeIdx < meshNodes.Num(); ++NodeIdx)
	{
		// make use of the fact that children of nodes will always come later than their parents, so only need to search up until this index
		for (int32 SearchIdx = 0; SearchIdx < NodeIdx; ++SearchIdx)
		{
			if (meshNodes[NodeIdx]->GetParent() == meshNodes[SearchIdx])
			{
				MeshData.ParentIndex[NodeIdx] = SearchIdx;
				break;
			}
		}

		if (NodeIdx)
			ensure(MeshData.ParentIndex[NodeIdx] != INDEX_NONE);
		else
			ensure(MeshData.ParentIndex[NodeIdx] == INDEX_NONE);

		MeshData.Chunks[NodeIdx] = FbxFileReader::extractMeshData(geoConverter, meshNodes[NodeIdx]);
	}

	return MeshData;
}

void FbxFileReader::getFbxMeshes(FbxNode* node, TArray<FbxNode*>& meshNodes)
{
	FbxMesh* mesh = node->GetMesh();

	if (mesh)
	{
		meshNodes.Add(node);
	}

	int childCount = node->GetChildCount();
	for (int i = 0; i < childCount; i++)
	{
		FbxNode* childNode = node->GetChild(i);

		getFbxMeshes(childNode, meshNodes);
	}
}

FMeshData FbxFileReader::extractMeshData(FbxGeometryConverter& geoConverter, FbxNode* meshNode)
{
	FMeshData Data;

	FbxMesh* mesh = meshNode->GetMesh();
	// Verify that the mesh is triangulated.
	bool bAllTriangles = mesh->IsTriangleMesh();
	if (!bAllTriangles)
	{
		// try letting the FBX SDK triangulate it
		mesh = FbxCast<FbxMesh>(geoConverter.Triangulate(mesh, false));
		bAllTriangles = mesh->IsTriangleMesh();
	}

	int polyCount = mesh->GetPolygonCount();
	if (!bAllTriangles)
	{
		UE_LOG(LogBlastIO, Warning, TEXT("Mesh %hs has %d polygons but not all of them are triangles. Mesh must be triangulated."), mesh->GetName(), polyCount);
		return Data;
	}

	FbxStringList uvSetNames;
	mesh->GetUVSetNames(uvSetNames);

	const FTransform3f UEToBlastTransform = UBlastMesh::GetTransformUEToBlastCoordinateSystem();

	/*FTransform3f ImportTransform(FTransform3f::Identity);
	//This pretty confusing, but the internal -Y flip becomes a -X flip due to the Y->X front conversion defined above
	if (SkeletalMeshImportData != nullptr)
	{
		if (SkeletalMeshImportData->bConvertScene && SkeletalMeshImportData->bForceFrontXAxis)
		{
			BlastToUE4Transform.SetRotation(FRotator3f(0, -90.0f, 0).Quaternion());
		}

		ImportTransform = FTransform3f(FRotator3f(SkeletalMeshImportData->ImportRotation), FVector3f(SkeletalMeshImportData->ImportTranslation), FVector3f(SkeletalMeshImportData->ImportUniformScale));
	}
	UEToBlastTransform = UEToBlastTransform * ImportTransform;*/

	const char* uvSetName = uvSetNames.GetCount() == 0 ? nullptr : uvSetNames.GetStringAt(0);

	int* polyVertices = mesh->GetPolygonVertices();

	uint32 vertIndex = 0;

	// Construct the matrices for the conversion from right handed to left handed system
	FbxAMatrix TotalMatrix = getTransformForNode(meshNode);
	FbxAMatrix TotalMatrixForNormal;
	TotalMatrixForNormal = TotalMatrix.Inverse();
	TotalMatrixForNormal = TotalMatrixForNormal.Transpose();

	int32 matElements = mesh->GetElementMaterialCount();
	if (matElements > 1)
	{
		UE_LOG(LogBlastIO, Log, TEXT("Mesh has more than 1 material mappings, first one will be used."));
	}
	auto matLayer = mesh->GetElementMaterial(0);
	auto smLayer = mesh->GetElementSmoothing();

	const int triangleIndexMappingUnflipped[3] = {0, 1, 2};
	const int triangleIndexMappingFlipped[3] = {2, 1, 0};
	const int* triangleIndexMapping = TotalMatrix.Determinant() < 0 ? triangleIndexMappingFlipped : triangleIndexMappingUnflipped;

	FbxVector4 normVec;
	FbxVector2 uvVec;
	bool bUnmapped;
	for (int32 i = 0; i < polyCount; i++)
	{
		for (int32 vi = 0; vi < 3; vi++)
		{
			int polyCPIdx = polyVertices[i * 3 + triangleIndexMapping[vi]];
			FbxVector4 vert = mesh->GetControlPointAt(polyCPIdx);
			vert = TotalMatrix.MultT(vert);
			FVector3f Vert {(float)vert[0], (float)-vert[1], (float)vert[2]};
			Data.Verts.Add(ToNvVector(UEToBlastTransform.TransformPosition(Vert)));

			if (mesh->GetPolygonVertexNormal(i, vi, normVec))
			{
				normVec = TotalMatrixForNormal.MultT(normVec);
				FVector3f Normal {(float)normVec[0], (float)-normVec[1], (float)normVec[2]};
				Data.Normals.Add(ToNvVector(UEToBlastTransform.TransformVector(Normal)));
			}
			else
			{
				Data.Normals.Add({0.f, 0.f, 0.f});
			}

			if (uvSetName && mesh->GetPolygonVertexUV(i, vi, uvSetName, uvVec, bUnmapped))
			{
				// Data.UVs.Add({(float)uvVec[0], 1.f - (float)uvVec[1]});
				Data.UVs.Add({(float)uvVec[0], (float)uvVec[1]});
			}
			else
			{
				Data.UVs.Add({0.f, 0.f});
			}

			Data.Indices.Add(vertIndex++);
		}

		if (matLayer)
		{
			Data.MaterialIDs.Add(matLayer->GetIndexArray().GetAt(i));
		}

		if (smLayer)
		{
			Data.SmoothingGroups.Add(smLayer->GetDirectArray().GetAt(i));
		}
	}

	return Data;
}
