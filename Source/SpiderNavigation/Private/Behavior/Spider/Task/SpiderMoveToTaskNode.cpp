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

#include "Behavior/Spider/Task/SpiderMoveToTaskNode.h"

#include "Behavior/BehaviorTreeHelpers.h"
#include "BehaviorTree/BlackboardComponent.h"

#include "Interfaces/SpiderAIControllerInterface.h"

#include "AIController.h"

USpiderMoveToTaskNode::USpiderMoveToTaskNode()
{
    
}

void USpiderMoveToTaskNode::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);
    TargetLocation.ResolveSelectedKey(*GetBlackboardAsset());
}

EBTNodeResult::Type USpiderMoveToTaskNode::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::ExecuteTask(OwnerComp, NodeMemory);

	UE_LOG(LogTemp, Warning, TEXT("USpiderMoveToTaskNode::ExecuteTask - Start"));

	CachedOwnerComp = &OwnerComp;

    if(AIController == nullptr)
    {
        AIController = Cast<AActor>(OwnerComp.GetAIOwner());
		UE_LOG(LogTemp, Warning, TEXT("USpiderMoveToTaskNode::ExecuteTask - AIController gesetzt"));
	}

    if (!AIController)
    {
		UE_LOG(LogTemp, Warning, TEXT("USpiderMoveToTaskNode::ExecuteTask - Kein AIController gefunden"));
        return EBTNodeResult::Failed;
    }
  
    const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB)
    {
		UE_LOG(LogTemp, Warning, TEXT("USpiderMoveToTaskNode::ExecuteTask - Kein BlackboardComponent gefunden"));
        return EBTNodeResult::Failed;        
    }

    const FVector VTargetLocation = BB->GetValueAsVector(TargetLocation.SelectedKeyName);

    if(!AIController->GetClass()->ImplementsInterface(USpiderAIControllerInterface::StaticClass()))
    {
		UE_LOG(LogTemp, Warning, TEXT("USpiderMoveToTaskNode::ExecuteTask - AIController implementiert nicht das Interface"));
        return EBTNodeResult::Failed;
	}
    
    ISpiderAIControllerInterface* SpiderCtrl = Cast<ISpiderAIControllerInterface>(AIController);
    if (!SpiderCtrl)
    {
		UE_LOG(LogTemp, Warning, TEXT("USpiderMoveToTaskNode::ExecuteTask - Konnte SpiderCtrl nicht casten"));
        return EBTNodeResult::Failed;
    }
	UE_LOG(LogTemp, Warning, TEXT("USpiderMoveToTaskNode::ExecuteTask - SpiderCtrl gefunden"));
    // Delegate binden
    SpiderCtrl->GetHandleMoveCompleted().AddUObject(this, &USpiderMoveToTaskNode::HandleMoveCompleted);
    
	UE_LOG(LogTemp, Warning, TEXT("USpiderMoveToTaskNode::ExecuteTask - MoveTo wird gestartet"));
    ISpiderAIControllerInterface::Execute_MoveTo(AIController, VTargetLocation);
	UE_LOG(LogTemp, Warning, TEXT("USpiderMoveToTaskNode::ExecuteTask - MoveTo gestartet"));
    return EBTNodeResult::InProgress;
}

void USpiderMoveToTaskNode::HandleMoveCompleted(bool Success)
{
    if (!AIController || !CachedOwnerComp)
        return;

    if (Success)
    {
        ISpiderAIControllerInterface* SpiderCtrl = Cast<ISpiderAIControllerInterface>(AIController);
        if (SpiderCtrl)
            SpiderCtrl->GetHandleMoveCompleted().RemoveAll(this);
            FinishLatentTask(*CachedOwnerComp, Success ? EBTNodeResult::Succeeded : EBTNodeResult::Failed);
    }
}

FString USpiderMoveToTaskNode::GetStaticDescription() const
{
    const FString KeyName = TargetLocation.SelectedKeyName.IsNone()
        ? TEXT("<none>")
        : TargetLocation.SelectedKeyName.ToString();

    return FString::Printf(TEXT("Target: %s"), *KeyName);
}
