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
#include "BehaviorTree/BTTaskNode.h"
#include "Navigation/PathFollowingComponent.h"
#include "SpiderMoveToTaskNode.generated.h"

/**
 * 
 */
UCLASS()
class SPIDERNAVIGATION_API USpiderMoveToTaskNode : public UBTTaskNode
{
	GENERATED_BODY()
public:
	USpiderMoveToTaskNode();
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Value Check", meta = (ExposeOnSpawn = true))
	FBlackboardKeySelector TargetLocation;
protected:
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;
protected:
	class AActor* AIController;
	class UBehaviorTreeComponent* CachedOwnerComp;

private:
	void HandleMoveCompleted(bool Success);
};
