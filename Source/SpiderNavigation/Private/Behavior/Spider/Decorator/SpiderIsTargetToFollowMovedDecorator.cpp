//The MIT License
//
//Copyright(C) 2017 Roman Nix
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

#include "Behavior/Spider/Decorator/SpiderIsTargetToFollowMovedDecorator.h"

#include "Behavior/BehaviorTreeHelpers.h"
#include "BehaviorTree/BlackboardComponent.h"

USpiderIsTargetToFollowMovedDecorator::USpiderIsTargetToFollowMovedDecorator()
{
	NodeName = TEXT("Spider Is TargetToFollow Moved");
	FlowAbortMode = EBTFlowAbortMode::Both;

	TargetToFollow.AddObjectFilter(this,
		GET_MEMBER_NAME_CHECKED(USpiderIsTargetToFollowMovedDecorator, TargetToFollow),
		UObject::StaticClass());

	TargetLocation.AddVectorFilter(this,
		GET_MEMBER_NAME_CHECKED(USpiderIsTargetToFollowMovedDecorator, TargetLocation));
}
void USpiderIsTargetToFollowMovedDecorator::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	TargetToFollow.ResolveSelectedKey(*GetBlackboardAsset());
	TargetLocation.ResolveSelectedKey(*GetBlackboardAsset());
}

bool USpiderIsTargetToFollowMovedDecorator::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB)
		return false;

	AActor* Target = Cast<AActor>(BB->GetValueAsObject(TargetToFollow.SelectedKeyName));
	if (!Target)
		return false;

	const FVector LastKnownLocation = BB->GetValueAsVector(TargetLocation.SelectedKeyName);
	const FVector CurrentLocation = Target->GetActorLocation();

	const bool bHasMoved = !CurrentLocation.Equals(LastKnownLocation, 5.0f); // 5 cm Toleranz
	return bHasMoved;
}

FString USpiderIsTargetToFollowMovedDecorator::GetStaticDescription() const
{
	const FString AbortText = UBehaviorTreeHelpers::AbortModeToText(FlowAbortMode);
	const FString AbortLine = AbortText.IsEmpty() ? TEXT("") : (AbortText + TEXT("\n"));

	return FString::Printf(TEXT("%sTargetToFollow: %s\nTargetLocation: %s"),
		*AbortLine,
		*TargetToFollow.SelectedKeyName.ToString(),
		*TargetLocation.SelectedKeyName.ToString());
}

//if (bHasMoved)
//{
//	OwnerComp.GetBlackboardComponent()->SetValueAsVector(TargetLocation.SelectedKeyName, CurrentLocation);
//}