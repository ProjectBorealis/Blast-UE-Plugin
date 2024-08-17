// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Input/SButton.h"
#include "IDetailsView.h"

#include "BlastMeshEditor.h"

//////////////////////////////////////////////////////////////////////////
// SSelectStaticMeshDialog
//////////////////////////////////////////////////////////////////////////

class SSelectStaticMeshDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSelectStaticMeshDialog)
		{
		}

		SLATE_ARGUMENT_DEFAULT(TObjectPtr<UStaticMesh>, Mesh){};
	SLATE_END_ARGS()

	// Constructs this widget with InArgs
	void Construct(const FArguments& InArgs);

	void MeshSelected();
	FReply LoadClicked();
	FReply CancelClicked();

	// Show the dialog, returns true if successfully edited fracture script
	struct FLoadMeshResult
	{
		TObjectPtr<UStaticMesh> Mesh;
		bool bCleanMesh = false;
	};

	static FLoadMeshResult ShowWindow(const TObjectPtr<UStaticMesh>& DefaultMesh = {});

	void CloseContainingWindow();

	TSharedPtr<SButton> LoadButton;
	TSharedPtr<IDetailsView> MeshView;
	TObjectPtr<class UBlastStaticMeshHolder> StaticMeshHolder;
	bool bLoadConfirmed = false;
};

//////////////////////////////////////////////////////////////////////////
// SFixChunkHierarchyDialog
//////////////////////////////////////////////////////////////////////////

class SFixChunkHierarchyDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFixChunkHierarchyDialog)
		{
		}

	SLATE_END_ARGS()

	// Constructs this widget with InArgs
	void Construct(const FArguments& InArgs);

	FReply OnClicked(bool isFix);

	// Show the dialog, returns true if successfully edited fracture script
	static bool ShowWindow(TSharedPtr<class FBlastFracture> Fracturer, UBlastFractureSettings* FractureSettings,
	                       TSet<int32>& SelectedChunkIndices);

	void CloseContainingWindow();

	TSharedPtr<IDetailsView> PropertyView;
	TObjectPtr<class UBlastFixChunkHierarchyProperties> Properties;
	bool IsFix = false;
};

//////////////////////////////////////////////////////////////////////////
// SBlastRootImportSettingsDialog
//////////////////////////////////////////////////////////////////////////

class SBlastRootImportSettingsDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBlastRootImportSettingsDialog)
	{
	}
	SLATE_END_ARGS()

	// Constructs this widget with InArgs
	void Construct(const FArguments& InArgs);

	FReply ImportClicked();
	FReply CancelClicked();

	// Show the dialog, returns true if successfully edited fracture script
	struct FImportSettingsResult
	{
		bool bCleanMesh = false;
	};

	static TOptional<FImportSettingsResult> ShowWindow();

	void CloseContainingWindow();

	TSharedPtr<IDetailsView> SettingsView;
	TObjectPtr<class UBlastImportSettings> ImportSettingsHolder;
	bool bLoadConfirmed = false;
};

//////////////////////////////////////////////////////////////////////////
// SUVCoordinatesDialog
//////////////////////////////////////////////////////////////////////////

class SFitUvCoordinatesDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFitUvCoordinatesDialog)
		{
		}

	SLATE_END_ARGS()

	// Constructs this widget with InArgs
	void Construct(const FArguments& InArgs);

	inline void OnSquareSizeChanged(float value)
	{
		mSquareSize = value;
	}

	inline TOptional<float> getSquareSize() const
	{
		return mSquareSize;
	}

	inline void OnIsSelectedToggleChanged(ECheckBoxState vl)
	{
		isOnlySelectedToggle = vl;
	}

	inline ECheckBoxState getIsOnlySelectedToggle() const
	{
		return isOnlySelectedToggle;
	}

	FReply OnClicked(bool isFix);

	// Show the dialog, returns true if successfully edited fracture script
	static bool ShowWindow(TSharedPtr<FBlastFracture> Fracturer, UBlastFractureSettings* FractureSettings,
	                       TSet<int32>& ChunkIndices);

	void CloseContainingWindow();

	bool shouldFix;
	float mSquareSize;
	ECheckBoxState isOnlySelectedToggle;
};

//////////////////////////////////////////////////////////////////////////
// SRebuildCollisionMeshDialog
//////////////////////////////////////////////////////////////////////////

class SRebuildCollisionMeshDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRebuildCollisionMeshDialog)
		{
		}

	SLATE_END_ARGS()

	// Constructs this widget with InArgs
	void Construct(const FArguments& InArgs);

	FReply OnClicked(bool InIsRebuild);

	// Show the dialog, returns true if successfully edited fracture script
	static bool ShowWindow(TSharedPtr<class FBlastFracture> Fracturer, UBlastFractureSettings* FractureSettings,
	                       TSet<int32>& ChunkIndices);

	void CloseContainingWindow();

	TSharedPtr<IDetailsView> PropertyView;
	TObjectPtr<class UBlastRebuildCollisionMeshProperties> Properties;
	bool IsRebuild = false;
};

//////////////////////////////////////////////////////////////////////////
// SCopyCollisionMeshToChunkDialog
//////////////////////////////////////////////////////////////////////////

class SCopyCollisionMeshToChunkDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCopyCollisionMeshToChunkDialog)
	{
	}

	SLATE_END_ARGS()

	// Constructs this widget with InArgs
	void Construct(const FArguments& InArgs);

	FReply OnClicked(bool Cancel);

	// Show the dialog, returns true if successfully edited fracture script
	static bool ShowWindow(UBlastMesh* Mesh, TSet<int32>& ChunkIndices);

	void CloseContainingWindow();

	void MeshSelected();

	TSharedPtr<IDetailsView> PropertyView;
	TSharedPtr<SButton> CopyButton;
	TObjectPtr<class UBlastStaticMeshCopyCollisionProperties> Properties;
	bool bActionCancelled = true;
};


////////////////////////////////////////////////////////////////////////////
//// SUVCoordinatesDialog
////////////////////////////////////////////////////////////////////////////

class SExportAssetToFileDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SExportAssetToFileDialog)
		{
		}

	SLATE_END_ARGS()

	// Constructs this widget with InArgs
	void Construct(const FArguments& InArgs);

	FReply OnClicked(bool isFix);

	// Show the dialog, returns true if successfully edited fracture script
	static bool ShowWindow(TSharedPtr<FBlastFracture> Fracturer, UBlastFractureSettings* FractureSettings);

	void CloseContainingWindow();
};
