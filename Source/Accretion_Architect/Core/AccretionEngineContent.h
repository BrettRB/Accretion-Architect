// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Engine content the gray-box visuals are built from (no project assets needed).
 * Load these with ConstructorHelpers in constructors so they are cooked into packaged builds.
 */
namespace AccretionEngineContent
{
	/** 100 cm diameter sphere (50 cm radius). */
	inline const TCHAR* SphereMesh = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	constexpr float SphereMeshRadius = 50.f;

	/** Unlit opaque, EmissiveColor = vertex color. */
	inline const TCHAR* VertexColorUnlitMaterial = TEXT("/Engine/EngineDebugMaterials/VertexColorViewMode_ColorOnly.VertexColorViewMode_ColorOnly");

	/** Unlit opaque, EmissiveColor = "Color" parameter (HDR allowed). */
	inline const TCHAR* UnlitColorMaterial = TEXT("/Engine/EngineDebugMaterials/LevelColorationUnlitMaterial.LevelColorationUnlitMaterial");

	/** Default lit opaque, BaseColor = "Color" parameter. */
	inline const TCHAR* LitColorMaterial = TEXT("/Engine/EngineDebugMaterials/M_SimpleOpaque.M_SimpleOpaque");

	/** Unlit additive, EmissiveColor = "Texture" * "Color". */
	inline const TCHAR* AdditiveMaterial = TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial");

	inline const TCHAR* WhiteTexture = TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture");

	inline const FName ColorParameter(TEXT("Color"));
	inline const FName TextureParameter(TEXT("Texture"));
}
