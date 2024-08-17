#pragma once

#include "BlastMeshImporter.h"

#include "fbxsdk.h"
#include <vector>
#include <map>
#include "NvBlastExtAuthoringTypes.h"

namespace Nv
{
	namespace Blast
	{
		class Mesh;

		class FbxFileReader : public IBlastMeshFileReader
		{
		public:
			FbxFileReader() = default;

			virtual ~FbxFileReader()
			{
			}

			/*
			Load from the specified file path, returning the loaded mesh data
			*/
			virtual FImportedMeshData LoadFromFile(const FString& FilePath) override;

		private:
			static FbxAMatrix getTransformForNode(FbxNode* node);
			static void getFbxMeshes(FbxNode* node, TArray<FbxNode*>& meshNodes);
			static FMeshData extractMeshData(FbxGeometryConverter& geoConverter, FbxNode* mesh);
		};
	}
}
