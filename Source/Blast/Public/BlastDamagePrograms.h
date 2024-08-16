#pragma once

#include "CoreMinimal.h"

#include "BlastBaseDamageProgram.h"

///////////////////////////////////////////////////////////////////////////////
// This file contains default/basic damage programs for BlastPlugin. Users 
// are welcome to implement their own by looking at these examples.
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//  Radial Damage
///////////////////////////////////////////////////////////////////////////////

/**
Radial Damage Program with falloff
*/
struct BLAST_API BlastRadialDamageProgram : public FBlastBaseDamageProgram
{
	BlastRadialDamageProgram(float damage, float minRadius, float maxRadius, float impulseStrength = 0.0f, bool bVelChange = false, bool bRandomizeImpulse = false, float ImpulseRandomizationDivider = 2.f);

	// Damage amount
	float Damage;

	// Inner radius of damage
	float MinRadius;

	// Outer radius of damage
	float MaxRadius;

	// Impulse to apply after splitting
	float ImpulseStrength;

	// If true, the impulse will ignore mass of objects and will always result in a fixed velocity change
	bool bImpulseVelChange;

	// If true, we'll randomize impulses a bit
	bool bRandomizeImpulse;

	// Use this divider to min/max impulse randomization
	float ImpulseRandomizationDivider;

	virtual bool Execute(uint32 actorIndex, FBodyInstance* actorBody, const FInput& input, UBlastMeshComponent& owner) const override;

	virtual FCollisionShape GetCollisionShape() const override
	{
		return FCollisionShape::MakeSphere(MaxRadius);
	}


	virtual void ExecutePostActorCreated(uint32 actorIndex, FBodyInstance* actorBody, const FInput& input, UBlastMeshComponent& owner) const override;
};


///////////////////////////////////////////////////////////////////////////////
//  Capsule Damage
///////////////////////////////////////////////////////////////////////////////

/**
Capsule Falloff Damage Program

Can be used for laser/cutting-like narrow capsules (kind of a swords) for example.
*/
struct BLAST_API BlastCapsuleDamageProgram : public FBlastBaseDamageProgram
{
	BlastCapsuleDamageProgram(float damage, float halfHeight, float minRadius, float maxRadius, float impulseStrength = 0.0f, bool bVelChange = false);

	// Damage amount
	float Damage;

	// Capsule Half Height
	float HalfHeight;

	// Inner radius of damage
	float MinRadius;

	// Outer radius of damage
	float MaxRadius;

	// Impulse to apply after splitting
	float ImpulseStrength;

	// If true, the impulse will ignore mass of objects and will always result in a fixed velocity change
	bool bImpulseVelChange;


	virtual bool Execute(uint32 actorIndex, FBodyInstance* actorBody, const FInput& input, UBlastMeshComponent& owner) const override;

	virtual FCollisionShape GetCollisionShape() const override
	{
		return FCollisionShape::MakeCapsule(MaxRadius, HalfHeight);
	}

	virtual void ExecutePostActorCreated(uint32 actorIndex, FBodyInstance* actorBody, const FInput& input, UBlastMeshComponent& owner) const override;
};


///////////////////////////////////////////////////////////////////////////////
//  Shear Damage
///////////////////////////////////////////////////////////////////////////////

/**
Shear Damage Program
*/
struct BLAST_API BlastShearDamageProgram : public FBlastBaseDamageProgram
{
	BlastShearDamageProgram(float damage, float minRadius, float maxRadius, float impulseStrength = 0.0f, bool bVelChange = false);

	// Damage amount
	float Damage;

	// Inner radius of damage
	float MinRadius;

	// Outer radius of damage
	float MaxRadius;

	// Impulse to apply after splitting
	float ImpulseStrength;

	// If true, the impulse will ignore mass of objects and will always result in a fixed velocity change
	bool bImpulseVelChange;


	virtual bool Execute(uint32 actorIndex, FBodyInstance* actorBody, const FInput& input, UBlastMeshComponent& owner) const override;

	virtual FCollisionShape GetCollisionShape() const override
	{
		return FCollisionShape::MakeSphere(MaxRadius);
	}

	virtual void ExecutePostActorCreated(uint32 actorIndex, FBodyInstance* actorBody, const FInput& input, UBlastMeshComponent& owner) const override;
};
