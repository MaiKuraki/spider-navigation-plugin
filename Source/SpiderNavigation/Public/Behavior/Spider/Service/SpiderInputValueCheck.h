//The MIT License
//
//Copyright(C) 2017 Roman Nix
//Copyright(C) 2025 Yves Tanas
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files(the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions :
//
//The above copyright notice and this permission notice shall be included in
//all copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//THE SOFTWARE.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_BlueprintBase.h"
#include "SpiderInputValueCheck.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class SPIDERNAVIGATION_API USpiderInputValueCheck : public UBTService_BlueprintBase
{
	GENERATED_BODY()

public:
	USpiderInputValueCheck();

protected:
	virtual void TickNode(class UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** Notify called after GameplayTask changes state to Active (initial activation or resuming) */
	virtual void OnGameplayTaskActivated(UGameplayTask& Task) override;

	/** Notify called after GameplayTask changes state from Active (finishing or pausing) */
	virtual void OnGameplayTaskDeactivated(UGameplayTask& Task) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Value Check")
	FBlackboardKeySelector TargetToFollow;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Value Check")
	FBlackboardKeySelector TargetLocation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Value Check")
	float SenseRadius;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Value Check")
	TArray<TEnumAsByte<EObjectTypeQuery>> DesiredObjectTypes;

private:
	class UObject* Navigation;
	class AActor* AIController;
	class AActor* Target;
	FVector CurrentLocation;

private:
	bool CheckReferences(class UBehaviorTreeComponent& OwnerComp);
	void PerformValueCheck(class UBehaviorTreeComponent& OwnerComp);
};
