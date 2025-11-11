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
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Interfaces/SpiderAIControllerInterface.h"
#include "SpiderAIController.generated.h"

class ISpiderNavigationInterface;

/**
 * Spider AI Controller
 * - Steuert Knotenbasierte Navigation
 * - Nutzt ISpiderNavigationInterface
 * - Implementiert ISpiderAIControllerInterface mit MoveCompleted-Event
 */
UCLASS()
class SPIDERNAVIGATION_API ASpiderAIController
	: public AAIController
	, public ISpiderAIControllerInterface
{
	GENERATED_BODY()

public:
	ASpiderAIController();

	// ===== Interface =====
	virtual FHandleMoveCompleted& GetHandleMoveCompleted() override { return HandleMoveCompleted; }

	// BlueprintNativeEvents aus dem Interface
	virtual void MoveTo_Implementation(const FVector& Destination) override;
	virtual AActor* GetTargetActor_Implementation() const override;
	virtual FVector GetLocation_Implementation() const override;
	virtual bool MustCheckTargetVisibility_Implementation() const override;

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaSeconds) override;

public:
	/** Behavior Tree asset to run */
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset = nullptr;

	/** Blackboard component instance */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBlackboardComponent> BlackboardComp = nullptr;

	/** Behavior tree component instance */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBehaviorTreeComponent> BehaviorComp = nullptr;

	// ===== AI Config =====
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spider|Target")
	bool bMustCheckTargetVisibility = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spider|Target")
	bool bFollowPlayer = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spider|Debug")
	bool bDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spider|Speed", meta = (ClampMin = "1.0"))
	float MoveSpeed = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spider|Speed", meta = (ClampMin = "1.0"))
	float RotationSpeed = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spider|Navigation", meta = (ClampMin = "0.01"))
	float UpdateLocalDestinationInterval = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spider|Navigation", meta = (ClampMin = "1.0"))
	float AcceptanceRadius = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spider|Navigation", meta = (ClampMin = "1.0"))
	float NodeAcceptanceRadius = 30.f;

protected:
	// ===== Internal Data =====
	UPROPERTY()
	AActor* TargetActor = nullptr;

	UPROPERTY()
	UObject* NavigationComponent = nullptr;

	UPROPERTY()
	TArray<FVector> CurrentPathPoints;

	UPROPERTY()
	FVector MoveDestination = FVector::ZeroVector;

	UPROPERTY()
	FVector LocalMoveDestination = FVector::ZeroVector;

	UPROPERTY()
	FVector MoveNormal = FVector::UpVector;

	UPROPERTY()
	FVector NormalToDestinationBeforeMove = FVector::ZeroVector;

	UPROPERTY()
	FRotator RotationBeforeMove = FRotator::ZeroRotator;

	UPROPERTY()
	FRotator RotationDestination = FRotator::ZeroRotator;

	UPROPERTY()
	bool bMustMove = false;

	UPROPERTY()
	bool bMustMoveLocal = false;

	UPROPERTY()
	bool bMustRotate = false;

	UPROPERTY()
	bool bLocalPathFound = false;

	FTimerHandle TimerHandle_FindPathTick;
	FHandleMoveCompleted HandleMoveCompleted;

private:
	// ===== Core Logic =====
	void FindPathTick();
	void UpdateLocalMoveDestination();
	void UpdateRotationParams();

	// ===== Movement =====
	void Tick_CheckFinish();
	void Tick_Rotate(float DeltaSeconds);
	void Tick_Move(float DeltaSeconds);

	// ===== Blackboard helpers =====
	void SetTargetLocation(const FVector& NewLocation);
	void SetTargetActor(AActor* NewTarget);

	// ===== Navigation helpers =====
	UObject* ResolveNavigationInterface();
	FVector GetNormalToLocalDestination() const;
	void DrawDebugPath() const;
};
