#include "BlastBlueprintFunctionLibrary.h"

#include "Components/PrimitiveComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BlastBlueprintFunctionLibrary)

bool UBlastBlueprintFunctionLibrary::IsValidToApplyForces(class UPrimitiveComponent* Component, FName BoneName)
{
	if (Component && Component->Mobility == EComponentMobility::Movable)
	{
		FBodyInstance* BI = Component->GetBodyInstance(BoneName);
		return BI && BI->bSimulatePhysics;
	}
	return false;
}
