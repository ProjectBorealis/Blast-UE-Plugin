// Copyright (c) 2017 NVIDIA Corporation. All rights reserved.


#pragma once

#include "NvBlastTypes.h"
#include "NvCTypes.h"
#include "NvPreprocessor.h"

struct NvBlastAsset;

namespace Nv
{
	namespace Blast
	{
		struct AuthoringResult;
		struct CollisionHull;

		struct Material
		{
			const char* name;
			const char* diffuse_tex;
		};

		struct ExporterMeshData
		{
			NvBlastAsset* asset; //Blast asset

			uint32 positionsCount; //Number of positions

			uint32 normalsCount; //Number of normals

			uint32 uvsCount; //Number of textures uv

			NvcVec3* positions; //Array of positions

			NvcVec3* normals; //Array of normals

			NvcVec2* uvs; //Array of textures uv

			uint32 meshCount; //Number of meshes (chunks)

			uint32 submeshCount; //Number of submeshes

			Material* submeshMats;


			/**
				Indices offsets for posIndex, normIndex and texIndex
				First position index: posIndex[submeshOffsets[meshId * submeshCount + submeshId]]
				Total number of indices: submeshOffsets[meshCount * submeshCount]
			*/
			uint32* submeshOffsets;

			uint32* posIndex; //Array of position indices

			uint32* normIndex; //Array of normals indices

			uint32* texIndex; //Array of texture indices


			/**
				Hull offsets. Contains meshCount + 1 element.
				First hull for i-th mesh: hulls[hullsOffsets[i]]
				hullsOffsets[meshCount+1] is total number of hulls
			*/
			uint32* hullsOffsets;

			CollisionHull** hulls; //Array of pointers to hull for all meshes

			~ExporterMeshData()
			{
				//delete[] hulls;
				//delete[] hullsOffsets;
				delete[] normals;
				//delete[] normIndex;
				delete[] posIndex;
				delete[] positions;
				delete[] submeshOffsets;
				//delete[] texIndex;
				delete[] submeshMats;
				delete[] uvs;
			}
		};

		/**
			An interface for Blast mesh file reader
		*/
		class IMeshFileReader
		{
		public:

			/**
				Delete this object
			*/
			virtual void			release() = 0;

			/*
				Load from the specified file path
			*/
			virtual void			loadFromFile(const char* filename) = 0;

			/**
				Number of loaded vertices
			*/
			virtual uint32		getVerticesCount() const = 0;

			/**
				Number of loaded indices
			*/
			virtual uint32		getIndicesCount() const = 0;

			/**
				Get loaded vertex positions
			*/
			virtual NvcVec3* getPositionArray() = 0;

			/**
				Get loaded vertex normals
			*/
			virtual NvcVec3* getNormalsArray() = 0;

			/**
				Get loaded vertex uv-coordinates
			*/
			virtual NvcVec2* getUvArray() = 0;

			/**
				Get loaded per triangle material ids.
			*/
			virtual int32* getMaterialIds() = 0;

			/**
					Get loaded per triangle smoothing groups.
			*/
			virtual int32* getSmoothingGroups() = 0;

			/**
				Get material name.
			*/
			virtual const char* getMaterialName(int32 id) = 0;

			/**
				Get material count.
			*/
			virtual int32			getMaterialCount() = 0;



			/**
				Get loaded triangle indices
			*/
			virtual uint32* getIndexArray() = 0;


			/**
				Check whether file contained an collision geometry
			*/
			virtual bool			isCollisionLoaded() = 0;

			/**
				Retrieve collision geometry if it exist
				\note User should call NVBLAST_FREE for hulls and hullsOffset when it not needed anymore

				\param[out] hullsOffset		Array of hull offsets for hulls array. The size is meshCount + 1.
				\param[out] hulls			Array of hull. The first i-th mesh hull: hulls[hullsOffset[i]]. The size is written to hullsOffset[meshCount]
				\return						Number of meshes (meshCount)
			*/
			virtual uint32		getCollision(uint32*& hullsOffset, CollisionHull**& hulls) = 0;

		};

		/**
			An interface for fbx file reader
		*/
		class IFbxFileReader : public IMeshFileReader
		{
		public:
			/**
			Retrieve bone influence if it exist, this is a bone index for each vertex in the mesh
			\note User should call NVBLAST_FREE for out when it not needed anymore

			\param[out] out			Array of bone influences.
			\return					Number of bones influences (boneCount)
			*/
			virtual uint32 getBoneInfluences(uint32*& out) = 0;

			/**
				Return number of bones in fbx file
			*/
			virtual uint32 getBoneCount() = 0;
		};

		/**
			An interface for Blast mesh file writer
		*/
		class IMeshFileWriter
		{
		public:

			virtual ~IMeshFileWriter() {};

			/**
				Delete this object
			*/
			virtual void release() = 0;

			/**
			Append rendermesh to scene. Meshes constructed from arrays of triangles.
			*/
			virtual bool appendMesh(const AuthoringResult& aResult, const char* assetName, bool nonSkinned = false) = 0;

			/**
			Append rendermesh to scene. Meshes constructed from arrays of vertices and indices
			*/
			virtual bool appendMesh(const ExporterMeshData& meshData, const char* assetName, bool nonSkinned = false) = 0;

			/**
			Save scene to file.
			*/
			virtual bool saveToFile(const char* assetName, const char* outputPath) = 0;

			/**
				Set material index for interior surface. By default new material will be created;
			*/
			virtual void setInteriorIndex(int32 index) = 0;
		};


		/**
		An interface to export collision data
		*/
		class ICollisionExporter
		{
		public:
			/**
			Delete this object
			*/
			virtual void	release() = 0;

			/**
			Method creates file with given path and serializes given array of arrays of convex hulls to it in JSON format.
			\param[in] path			Output file path.
			\param[in] chunkCount	The number of chunks, may be less than the number of collision hulls.
			\param[in] hullOffsets	Collision hull offsets. Contains chunkCount + 1 element. First collision hull for i-th chunk: hull[hullOffsets[i]]. hullOffsets[chunkCount+1] is total number of hulls.
			\param[in] hulls		Array of pointers to convex hull descriptors, contiguously grouped for chunk[0], chunk[1], etc.
			*/
			virtual bool	writeCollision(const char* path, uint32 chunkCount, const uint32* hullOffsets, const CollisionHull* const* hulls) = 0;
		};

	}
}

/**
	Creates an instance of IMeshFileWriter for writing an obj file.
*/
NV_C_API Nv::Blast::IMeshFileWriter* NvBlastExtExporterCreateObjFileWriter();

/**
	Creates an instance of IMeshFileWriter for writing a fbx file.

	\param[in] outputFBXAscii	If true writes fbx in ascii format otherwise write in binary.
*/
NV_C_API Nv::Blast::IMeshFileWriter* NvBlastExtExporterCreateFbxFileWriter(bool outputFBXAscii = false);

/**
Creates an instance of ICollisionExporter for writing a collision json file.
*/
NV_C_API Nv::Blast::ICollisionExporter* NvBlastExtExporterCreateJsonFileWriter();
