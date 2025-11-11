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

#include "Behavior/Spider/Decorator/SpiderIsAtLocationDecorator.h"

#include "Behavior/BehaviorTreeHelpers.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BTFunctionLibrary.h"

#include "AIController.h"

void USpiderIsAtLocationDecorator::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetLocation.ResolveSelectedKey(*BBAsset);
	}
}

bool USpiderIsAtLocationDecorator::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	bool OK = Super::CalculateRawConditionValue(OwnerComp, NodeMemory);

	AAIController* Controller = OwnerComp.GetAIOwner();
	if (Controller)
	{
		if (APawn* Pawn = Cast<APawn>(Controller->GetPawn()))
		{
			const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
			if (BB)
			{
				FVector TargetLoc = BB->GetValueAsVector(TargetLocation.SelectedKeyName);
				float Distance = FVector::Dist(Pawn->GetActorLocation(), TargetLoc);
				return Distance <= AcceptableDistance;
			}
			else
			{
				UE_LOG(LogBehaviorTree, Warning, TEXT("USpiderIsAtLocationDecorator: No BlackboardComponent found for BehaviorTreeComponent %s"), *OwnerComp.GetName());
			}
		}
		else
		{
			UE_LOG(LogBehaviorTree, Warning, TEXT("USpiderIsAtLocationDecorator: No Pawn found for AIController %s"), *Controller->GetName());
		}
	}
	else
	{
		UE_LOG(LogBehaviorTree, Warning, TEXT("USpiderIsAtLocationDecorator: No AIController found"));
	}

	return false;
}
FString USpiderIsAtLocationDecorator::GetStaticDescription() const
{
	const FString AbortText = UBehaviorTreeHelpers::AbortModeToText(FlowAbortMode);

	// Zeile nur anh�ngen, wenn sinnvoll
	const FString AbortLine = AbortText.IsEmpty() ? TEXT("") : (AbortText + TEXT("\n"));

	return FString::Printf(
		TEXT("%sTargetLocation: %s\nAcceptable Distance: %.1f"),
		*AbortLine,
		*TargetLocation.SelectedKeyName.ToString(),
		AcceptableDistance
	);
}