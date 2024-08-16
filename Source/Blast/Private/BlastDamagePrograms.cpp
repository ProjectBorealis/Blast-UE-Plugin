#include "BlastDamagePrograms.h"

#include "BlastMeshComponent.h"

#include "blast-sdk/extensions/shaders/NvBlastExtDamageShaders.h"

BlastRadialDamageProgram::BlastRadialDamageProgram(float damage, float minRadius, float maxRadius, float impulseStrength, bool bVelChange, bool bRandomizeImpulse, float ImpulseRandomizationDivider)
	: Damage(damage)
	, MinRadius(minRadius)
	, MaxRadius(maxRadius)
	, ImpulseStrength(impulseStrength)
	, bImpulseVelChange(bVelChange)
	, bRandomizeImpulse(bRandomizeImpulse)
	, ImpulseRandomizationDivider(ImpulseRandomizationDivider)
{
}

bool BlastRadialDamageProgram::Execute(uint32 actorIndex, FBodyInstance* actorBody, const FInput& input, UBlastMeshComponent& owner) const
{
	const float normalizedDamage = input.material->GetNormalizedDamage(Damage);
	if (normalizedDamage == 0.f)
	{
		return false;
	}

	NvBlastExtRadialDamageDesc damage[] = {{normalizedDamage, {input.localOrigin.X, input.localOrigin.Y, input.localOrigin.Z}, MinRadius, MaxRadius}};

	NvBlastExtProgramParams programParams(damage, input.material);

	programParams.accelerator = owner.GetAccelerator();

	NvBlastDamageProgram program = {NvBlastExtFalloffGraphShader, NvBlastExtFalloffSubgraphShader};

	return owner.ExecuteBlastDamageProgram(actorIndex, program, programParams, DamageType);
}

void BlastRadialDamageProgram::ExecutePostActorCreated(uint32 actorIndex, FBodyInstance* actorBody, const FInput& input, UBlastMeshComponent& owner) const
{
	if (ImpulseStrength > 0.f && actorBody->IsInstanceSimulatingPhysics())
	{
		// Do not increase impulse a lot during randomization
		const float FinalImpulseStrength = bRandomizeImpulse ? FMath::RandRange(ImpulseStrength - ImpulseStrength / ImpulseRandomizationDivider, ImpulseStrength + ImpulseStrength / (ImpulseRandomizationDivider * 2.f)) : ImpulseStrength;
		actorBody->AddRadialImpulseToBody(FVector(input.worldOrigin), MaxRadius, FinalImpulseStrength, 0, bImpulseVelChange);
	}
}

BlastCapsuleDamageProgram::BlastCapsuleDamageProgram(float damage, float halfHeight, float minRadius, float maxRadius, float impulseStrength, bool bVelChange)
	: Damage(damage)
	, HalfHeight(halfHeight)
	, MinRadius(minRadius)
	, MaxRadius(maxRadius)
	, ImpulseStrength(impulseStrength)
	, bImpulseVelChange(bVelChange)
{
}

bool BlastCapsuleDamageProgram::Execute(uint32 actorIndex, FBodyInstance* actorBody, const FInput& input, UBlastMeshComponent& owner) const
{
	FVector3f CapsuleDir = input.localRot.RotateVector(FVector3f::UpVector);

	FVector3f pointA = input.localOrigin + CapsuleDir * HalfHeight;
	FVector3f pointB = input.localOrigin - CapsuleDir * HalfHeight;

	const float normalizedDamage = input.material->GetNormalizedDamage(Damage);
	if (normalizedDamage == 0.f)
	{
		return false;
	}

	NvBlastExtCapsuleRadialDamageDesc damage[] = {{normalizedDamage, {pointA.X, pointA.Y, pointA.Z}, {pointB.X, pointB.Y, pointB.Z}, MinRadius, MaxRadius}};

	NvBlastExtProgramParams programParams(damage, input.material);

	programParams.accelerator = owner.GetAccelerator();

	NvBlastDamageProgram program = {NvBlastExtCapsuleFalloffGraphShader, NvBlastExtCapsuleFalloffSubgraphShader};

	return owner.ExecuteBlastDamageProgram(actorIndex, program, programParams, DamageType);
}

inline FVector3f ClosestPointOnLine3f(const FVector3f& LineStart, const FVector3f& LineEnd, const FVector3f& Point)
{
	// Solve to find alpha along line that is closest point
	// Weisstein, Eric W. "Point-Line Distance--3-Dimensional." From MathWorld--A Switchram Web Resource. http://mathworld.wolfram.com/Point-LineDistance3-Dimensional.html 
	const float A = (LineStart - Point) | (LineEnd - LineStart);
	const float B = (LineEnd - LineStart).SizeSquared();
	// This should be robust to B == 0 (resulting in NaN) because clamp should return 1.
	const float T = FMath::Clamp<float>(-A / B, 0.f, 1.f);

	return LineStart + (T * (LineEnd - LineStart));
}

void BlastCapsuleDamageProgram::ExecutePostActorCreated(uint32 actorIndex, FBodyInstance* actorBody, const FInput& input, UBlastMeshComponent& owner) const
{
	if (ImpulseStrength > 0.f && actorBody->IsInstanceSimulatingPhysics())
	{
		FVector3f CapsuleDir = input.worldRot.RotateVector(FVector3f::UpVector);

		FVector3f pointA = input.worldOrigin + CapsuleDir * HalfHeight;
		FVector3f pointB = input.worldOrigin - CapsuleDir * HalfHeight;

		FVector3f ActorCom = FVector3f(actorBody->GetCOMPosition());
		FVector3f CapsulePoint = ClosestPointOnLine3f(pointA, pointB, ActorCom);

		actorBody->AddRadialImpulseToBody(FVector(CapsulePoint), (ActorCom - CapsulePoint).SizeSquared(), ImpulseStrength, 0, bImpulseVelChange);
	}
}

BlastShearDamageProgram::BlastShearDamageProgram(float damage, float minRadius, float maxRadius, float impulseStrength, bool bVelChange)
	: Damage(damage)
	, MinRadius(minRadius)
	, MaxRadius(maxRadius)
	, ImpulseStrength(impulseStrength)
	, bImpulseVelChange(bVelChange)
{
}

bool BlastShearDamageProgram::Execute(uint32 actorIndex, FBodyInstance* actorBody, const FInput& input, UBlastMeshComponent& owner) const
{
	const float normalizedDamage = input.material->GetNormalizedDamage(Damage);
	if (normalizedDamage == 0.f)
	{
		return false;
	}

	FVector3f LocalNormal = input.localRot.GetForwardVector();

	NvBlastExtShearDamageDesc damage[] = {{normalizedDamage, {LocalNormal.X, LocalNormal.Y, LocalNormal.Z}, {input.localOrigin.X, input.localOrigin.Y, input.localOrigin.Z}, MinRadius, MaxRadius}};

	NvBlastExtProgramParams programParams(damage, input.material);

	programParams.accelerator = owner.GetAccelerator();

	NvBlastDamageProgram program = {NvBlastExtShearGraphShader, NvBlastExtShearSubgraphShader};

	return owner.ExecuteBlastDamageProgram(actorIndex, program, programParams, DamageType);
}

void BlastShearDamageProgram::ExecutePostActorCreated(uint32 actorIndex, FBodyInstance* actorBody, const FInput& input, UBlastMeshComponent& owner) const
{
	if (ImpulseStrength > 0.f && actorBody->IsInstanceSimulatingPhysics())
	{
		actorBody->AddRadialImpulseToBody(FVector(input.worldOrigin), MaxRadius, ImpulseStrength, 0, bImpulseVelChange);
	}
}
