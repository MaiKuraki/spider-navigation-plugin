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

#include "Behavior/Spider/Decorator/SpiderCloseEnoughDecorator.h"

#include "Behavior/BehaviorTreeHelpers.h" 

#include "BehaviorTree/BTFunctionLibrary.h"
#include "BehaviorTree/BlackboardComponent.h"

#include "GameFramework/GameStateBase.h"

#include "Interfaces/SpiderNavigationInterface.h"
#include "Interfaces/SpiderAIControllerInterface.h"

#include "Kismet/GameplayStatics.h"

#include "AIController.h"


USpiderCloseEnoughDecorator::USpiderCloseEnoughDecorator()
{
	NodeName = "Spider Close Enough";

	TargetToFollow.AddObjectFilter(this,
		GET_MEMBER_NAME_CHECKED(USpiderCloseEnoughDecorator, TargetToFollow),
		UObject::StaticClass());

	FlowAbortMode = EBTFlowAbortMode::Both;
}
void USpiderCloseEnoughDecorator::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetToFollow.ResolveSelectedKey(*BBAsset);
	}
}
bool USpiderCloseEnoughDecorator::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	bool OK = Super::CalculateRawConditionValue(OwnerComp, NodeMemory);

	if (!CheckReferences(OwnerComp))
	{
		UE_LOG(LogTemp, Warning, TEXT("SpiderCloseEnoughDecorator: Missing references."));
		return false;
	}
	else
	{
		return CheckSpiderCloseEnough();
	}
}

bool USpiderCloseEnoughDecorator::CheckReferences(UBehaviorTreeComponent& OwnerComp) const
{
	if (!Pawn)
	{
		AAIController* Controller = OwnerComp.GetAIOwner();
		if (Controller && Controller->GetClass()->ImplementsInterface(USpiderAIControllerInterface::StaticClass()))
		{
			Pawn = Cast<AActor>(Controller->GetPawn());
		}
	}

	if (!Navigation)
	{
		if (AGameStateBase* GameState = GetWorld()->GetGameState())
		{
			TArray<UActorComponent*> NavComponents = GameState->GetComponentsByInterface(USpiderNavigationInterface::StaticClass());
			if (NavComponents.Num() > 0)
			{
				Navigation = NavComponents[0];
			}
		}
	}

	if(!BB)
	{
		BB = OwnerComp.GetBlackboardComponent();
	}
		
	if (BB)
	{
		Target = Cast<AActor>(BB->GetValueAsObject(TargetToFollow.SelectedKeyName));
	}

	return Pawn != nullptr && Navigation != nullptr && Target != nullptr;
}

bool USpiderCloseEnoughDecorator::CheckSpiderCloseEnough() const
{
	if(Navigation && Pawn && Target)
	{
		FVector ClosestNodeLocation = ISpiderNavigationInterface::Execute_FindClosestNodeLocation(Navigation, Target->GetActorLocation());

		if ((ClosestNodeLocation - Pawn->GetActorLocation()).Length() >= AcceptableDistance)
			return true;
	}
	else
	{
		return false;
	}

	return false;
}


FString USpiderCloseEnoughDecorator::GetStaticDescription() const
{
	const FString AbortText = UBehaviorTreeHelpers::AbortModeToText(FlowAbortMode);

	// Zeile nur anhängen, wenn sinnvoll
	const FString AbortLine = AbortText.IsEmpty() ? TEXT("") : (AbortText + TEXT("\n"));

	return FString::Printf(
		TEXT("%sTarget: %s\nAcceptable Distance: %.1f"),
		*AbortLine,
		*TargetToFollow.SelectedKeyName.ToString(),
		AcceptableDistance
	);
}
