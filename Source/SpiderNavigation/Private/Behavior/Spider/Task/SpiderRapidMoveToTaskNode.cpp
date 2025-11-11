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

#include "Behavior/Spider/Task/SpiderRapidMoveToTaskNode.h"


#include "Behavior/BehaviorTreeHelpers.h"
#include "BehaviorTree/BlackboardComponent.h"

#include "Interfaces/SpiderAIControllerInterface.h"

#include "AIController.h"

USpiderRapidMoveToTaskNode::USpiderRapidMoveToTaskNode()
{

}

void USpiderRapidMoveToTaskNode::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);
    TargetToFollow.ResolveSelectedKey(*GetBlackboardAsset());
}

EBTNodeResult::Type USpiderRapidMoveToTaskNode::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    Super::ExecuteTask(OwnerComp, NodeMemory);

    if (AIController == nullptr)
    {
        AIController = Cast<AActor>(OwnerComp.GetAIOwner());
    }

    if (!AIController)
    {
        return EBTNodeResult::Failed;
    }
    const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB)
    {
        return EBTNodeResult::Failed;
    }

    AActor* Target = Cast<AActor>(BB->GetValueAsObject(TargetToFollow.SelectedKeyName));
    if (!Target)
    {
        return EBTNodeResult::Failed;
    }

    if (!AIController->GetClass()->ImplementsInterface(USpiderAIControllerInterface::StaticClass()))
    {
        return EBTNodeResult::Failed;
    }

    ISpiderAIControllerInterface::Execute_MoveTo(AIController, Target->GetActorLocation());
    return EBTNodeResult::Succeeded;
}

FString USpiderRapidMoveToTaskNode::GetStaticDescription() const
{
    const FString KeyName = TargetToFollow.SelectedKeyName.IsNone()
        ? TEXT("<none>")
        : TargetToFollow.SelectedKeyName.ToString();

    return FString::Printf(TEXT("TargetToFollow: %s"), *KeyName);
}
