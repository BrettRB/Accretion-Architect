// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Builds many tiny, double-sided, vertex-colored octahedra in one triangle list. Seen from a distance
 * they read as points of light. Used for the sky's starfield and the victory galaxy.
 */
struct FStarMeshBuilder
{
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FLinearColor> Colors;

	/** Adds a star at Location (relative to the mesh component) with half-size Size. */
	void AddStar(const FVector& Location, float Size, const FLinearColor& Color, const FVector& FacingHint = FVector::UpVector)
	{
		FVector Axis = FacingHint.GetSafeNormal();
		if (Axis.IsZero())
		{
			Axis = FVector::UpVector;
		}
		FVector U, V;
		Axis.FindBestAxisVectors(U, V);

		const int32 Base = Vertices.Num();
		Vertices.Append({ Location + U * Size, Location - U * Size, Location + V * Size, Location - V * Size, Location + Axis * Size, Location - Axis * Size });
		for (int32 Index = 0; Index < 6; ++Index)
		{
			Colors.Add(Color);
		}

		static constexpr int32 Faces[8][3] = {
			{ 4, 0, 2 }, { 4, 2, 1 }, { 4, 1, 3 }, { 4, 3, 0 },
			{ 5, 2, 0 }, { 5, 1, 2 }, { 5, 3, 1 }, { 5, 0, 3 },
		};
		for (const auto& Face : Faces)
		{
			Triangles.Append({ Base + Face[0], Base + Face[1], Base + Face[2] });
			Triangles.Append({ Base + Face[0], Base + Face[2], Base + Face[1] });
		}
	}

	/** Star in Direction at Distance from the origin, with an angular half-size in radians. */
	void AddSkyStar(const FVector& Direction, float Distance, float AngularSize, const FLinearColor& Color)
	{
		const FVector Dir = Direction.GetSafeNormal();
		if (!Dir.IsZero())
		{
			AddStar(Dir * Distance, Distance * AngularSize, Color, Dir);
		}
	}

	/** Star tint by spectral class, weighted toward the cooler, more common stars. */
	static FLinearColor RandomStarTint(FRandomStream& Random)
	{
		const float Roll = Random.FRand();
		if (Roll < 0.10f) return FLinearColor(0.65f, 0.75f, 1.f);  // O/B: blue-white
		if (Roll < 0.40f) return FLinearColor(0.9f, 0.93f, 1.f);   // A/F: white
		if (Roll < 0.70f) return FLinearColor(1.f, 0.93f, 0.78f);  // G: yellow-white
		if (Roll < 0.90f) return FLinearColor(1.f, 0.75f, 0.5f);   // K: orange
		return FLinearColor(1.f, 0.55f, 0.4f);                     // M: red
	}

	/** Roughly normal-distributed value with mean 0 and standard deviation ~1. */
	static float Gaussian(FRandomStream& Random)
	{
		return (Random.FRand() + Random.FRand() + Random.FRand() - 1.5f) * 2.f;
	}
};
