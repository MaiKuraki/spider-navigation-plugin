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

#include "Controllers/SpiderAIController.h"
#include "Interfaces/SpiderNavigationInterface.h"

#include "GameFramework/GameStateBase.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"

#define SPIDER_LOG(Verbosity, Format, ...) UE_LOG(LogTemp, Verbosity, TEXT("[SpiderAI] %s(): ") Format, ANSI_TO_TCHAR(__FUNCTION__), ##__VA_ARGS__)

ASpiderAIController::ASpiderAIController()
{
	BlackboardComp = CreateDefaultSubobject<UBlackboardComponent>(TEXT("Blackboard"));
	BehaviorComp = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("Behavior"));
}

void ASpiderAIController::BeginPlay()
{
	Super::BeginPlay();

	if (bFollowPlayer)
	{
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			TargetActor = PC->GetPawn();
		}
	}

	if (!NavigationComponent)
	{
		NavigationComponent = ResolveNavigationInterface();
	}

	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle_FindPathTick,
		this,
		&ASpiderAIController::FindPathTick,
		UpdateLocalDestinationInterval,
		true
	);
}

void ASpiderAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (BehaviorTreeAsset)
	{
		if (BehaviorTreeAsset->BlackboardAsset)
		{
			BlackboardComp->InitializeBlackboard(*BehaviorTreeAsset->BlackboardAsset);
		}

		if (!NavigationComponent)
		{
			NavigationComponent = ResolveNavigationInterface();
		}

		if (NavigationComponent)
		{
			const FVector Start = ISpiderNavigationInterface::Execute_FindClosestNodeLocation(
				NavigationComponent, InPawn->GetActorLocation());
			BlackboardComp->SetValueAsVector(TEXT("HomeLocation"), Start);
			BlackboardComp->SetValueAsObject(TEXT("SelfActor"), GetPawn());

			ISpiderNavigationInterface::Execute_RegisterSpiderNavGridActor(
				NavigationComponent, GetPawn());
		}

		BehaviorComp->StartTree(*BehaviorTreeAsset);
	}
}

void ASpiderAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Tick_CheckFinish();
	Tick_Rotate(DeltaSeconds);
	Tick_Move(DeltaSeconds);
}

void ASpiderAIController::SetTargetLocation(const FVector& NewLocation)
{
	if (BlackboardComp)
	{
		BlackboardComp->SetValueAsVector(TEXT("TargetLocation"), NewLocation);
	}
}

void ASpiderAIController::SetTargetActor(AActor* NewTarget)
{
	if (BlackboardComp)
	{
		BlackboardComp->SetValueAsObject(TEXT("TargetToFollow"), NewTarget);
	}
	TargetActor = NewTarget;
}

AActor* ASpiderAIController::GetTargetActor_Implementation() const { return TargetActor; }

FVector ASpiderAIController::GetLocation_Implementation() const
{
	return GetPawn() ? GetPawn()->GetActorLocation() : FVector::ZeroVector;
}

bool ASpiderAIController::MustCheckTargetVisibility_Implementation() const { return bMustCheckTargetVisibility; }

void ASpiderAIController::MoveTo_Implementation(const FVector& Destination)
{
	// --- DEBUG START ---
	SPIDER_LOG(Log, TEXT("MoveTo_Implementation called with Destination: %s"), *Destination.ToString());

	// Check Interface validity
	if (!NavigationComponent)
	{
		NavigationComponent = ResolveNavigationInterface();
		SPIDER_LOG(Warning, TEXT("NavigationComponent was null. Attempted to resolve: %s"),
			NavigationComponent ? TEXT("SUCCESS") : TEXT("FAILED"));
	}

	if (!NavigationComponent)
	{
		SPIDER_LOG(Error, TEXT("NavigationComponent is still NULL after Resolve. Aborting MoveTo."));
		return;
	}

	if (!NavigationComponent->GetClass()->ImplementsInterface(USpiderNavigationInterface::StaticClass()))
	{
		SPIDER_LOG(Error, TEXT("NavigationComponent (%s) does NOT implement ISpiderNavigationInterface!"),
			*NavigationComponent->GetClass()->GetName());
		return;
	}

	// Compute adjusted destination
	const FVector AdjustedDestination =
		ISpiderNavigationInterface::Execute_FindClosestNodeLocation(NavigationComponent, Destination);

	SPIDER_LOG(Log, TEXT("AdjustedDestination (ClosestNode): %s"), *AdjustedDestination.ToString());

	if (AdjustedDestination.Equals(MoveDestination, 1.f))
	{
		SPIDER_LOG(Log, TEXT("AdjustedDestination == MoveDestination -> No new path required."));
		return;
	}

	MoveDestination = AdjustedDestination;

	const FVector PawnPos = GetPawn() ? GetPawn()->GetActorLocation() : FVector::ZeroVector;
	const FVector StartNode = ISpiderNavigationInterface::Execute_FindClosestNodeLocation(NavigationComponent, PawnPos);

	SPIDER_LOG(Log, TEXT("PawnPos: %s"), *PawnPos.ToString());
	SPIDER_LOG(Log, TEXT("StartNode (Closest to Pawn): %s"), *StartNode.ToString());
	SPIDER_LOG(Log, TEXT("Destination (ClosestNode): %s"), *MoveDestination.ToString());

	// Visual Debug
#if WITH_EDITOR
	DrawDebugSphere(GetWorld(), PawnPos, 15.f, 8, FColor::Yellow, false, 10.f);  // Pawn
	DrawDebugSphere(GetWorld(), StartNode, 25.f, 8, FColor::Green, false, 10.f); // Start Node
	DrawDebugSphere(GetWorld(), MoveDestination, 25.f, 8, FColor::Red, false, 10.f); // Destination
#endif

	// Pathfinding
	CurrentPathPoints = ISpiderNavigationInterface::Execute_FindPathBetweenPoints(
		NavigationComponent, StartNode, MoveDestination);

	SPIDER_LOG(Log, TEXT("FindPathBetweenPoints called: Start=%s  End=%s  -> Returned %d points"),
		*StartNode.ToString(), *MoveDestination.ToString(), CurrentPathPoints.Num());

	if (CurrentPathPoints.Num() > 0)
	{
		for (int32 i = 0; i < CurrentPathPoints.Num(); ++i)
		{
			SPIDER_LOG(Log, TEXT("  PathPoint[%d]: %s"), i, *CurrentPathPoints[i].ToString());
		}
	}
	else
	{
		SPIDER_LOG(Warning, TEXT("Empty path returned from navigation."));
		SPIDER_LOG(Warning, TEXT("Possible causes:"));
		SPIDER_LOG(Warning, TEXT("  - Start or End node not inside Navigation Volume"));
		SPIDER_LOG(Warning, TEXT("  - NavigationComponent world not matching Pawn world"));
		SPIDER_LOG(Warning, TEXT("  - NavGraph not yet built / async generation incomplete"));
		SPIDER_LOG(Warning, TEXT("  - FindClosestNodeLocation returned fallback (0,0,0)"));

		// Optionally: draw debug markers to visualize
		if (GetWorld())
		{
			DrawDebugSphere(GetWorld(), StartNode, 40.f, 12, FColor::Red, false, 5.f);
			DrawDebugSphere(GetWorld(), MoveDestination, 40.f, 12, FColor::Blue, false, 5.f);
		}

		bMustMove = false;
		bLocalPathFound = false;
		bMustMoveLocal = false;
		HandleMoveCompleted.Broadcast(false);
		return;
	}
	// --- DEBUG END ---

	// remove start if it's same as current position
	if (FVector::DistSquared(CurrentPathPoints[0], StartNode) < FMath::Square(NodeAcceptanceRadius))
	{
		CurrentPathPoints.RemoveAt(0);
	}

	UpdateLocalMoveDestination();

	if (!bLocalPathFound)
	{
		HandleMoveCompleted.Broadcast(false);
		return;
	}

	bMustMove = true;
	SPIDER_LOG(Log, TEXT("MoveTo started. PathLength=%d"), CurrentPathPoints.Num());
}

void ASpiderAIController::Tick_CheckFinish()
{
	if (!bMustMove)
		return;

	const FVector PawnLoc = GetPawn() ? GetPawn()->GetActorLocation() : FVector::ZeroVector;

	if (FVector::DistSquared(PawnLoc, MoveDestination) <= FMath::Square(AcceptanceRadius))
	{
		bMustMove = false;
		bMustMoveLocal = false;
		bMustRotate = false;
		HandleMoveCompleted.Broadcast(true);
		SPIDER_LOG(Log, TEXT("Movement completed successfully."));
	}
}

void ASpiderAIController::Tick_Rotate(float DeltaSeconds)
{
	if (!bMustRotate || !GetPawn())
		return;

	const FRotator Current = GetPawn()->GetActorRotation();

	if (!Current.Equals(RotationDestination, 0.5f))
	{
		const FRotator NewRot = FMath::RInterpTo(Current, RotationDestination, DeltaSeconds, RotationSpeed);
		GetPawn()->SetActorRotation(NewRot);
	}
	else
	{
		bMustRotate = false;
	}
}

void ASpiderAIController::Tick_Move(float DeltaSeconds)
{
	if (!bMustMove || !bMustMoveLocal || bMustRotate || !GetPawn())
		return;

	const FVector Dir = GetNormalToLocalDestination();
	if (Dir.IsNearlyZero())
	{
		bMustMoveLocal = false;
		return;
	}

	NormalToDestinationBeforeMove = Dir;
	GetPawn()->AddActorWorldOffset(Dir * MoveSpeed * DeltaSeconds, true);

	const float DistSq = FVector::DistSquared(GetPawn()->GetActorLocation(), LocalMoveDestination);
	if (DistSq <= FMath::Square(NodeAcceptanceRadius))
	{
		if (CurrentPathPoints.Num() > 0)
		{
			CurrentPathPoints.RemoveAt(0);
		}
		UpdateLocalMoveDestination();
	}
}

void ASpiderAIController::FindPathTick()
{
	if (!bMustMove)
		return;

	UpdateLocalMoveDestination();

	if (!bLocalPathFound)
	{
		bMustMove = false;
		HandleMoveCompleted.Broadcast(false);
	}
}

void ASpiderAIController::UpdateLocalMoveDestination()
{
	if (CurrentPathPoints.Num() == 0)
	{
		bLocalPathFound = false;
		bMustMoveLocal = false;
		return;
	}

	LocalMoveDestination = CurrentPathPoints[0];
	bLocalPathFound = true;
	bMustMoveLocal = true;

	UpdateRotationParams();

	if (bDebug)
	{
		DrawDebugPath();
	}
}

void ASpiderAIController::UpdateRotationParams()
{
	if (!GetPawn())
		return;

	RotationBeforeMove = GetPawn()->GetActorRotation();

	const FVector ToTarget = (LocalMoveDestination - GetPawn()->GetActorLocation()).GetSafeNormal();
	RotationDestination = ToTarget.Rotation();

	bMustRotate = !RotationBeforeMove.Equals(RotationDestination, 0.5f);
}

FVector ASpiderAIController::GetNormalToLocalDestination() const
{
	if (!GetPawn())
		return FVector::ZeroVector;

	return (LocalMoveDestination - GetPawn()->GetActorLocation()).GetSafeNormal();
}

UObject* ASpiderAIController::ResolveNavigationInterface()
{
	if (NavigationComponent && NavigationComponent->GetClass()->ImplementsInterface(USpiderNavigationInterface::StaticClass()))
		return NavigationComponent;

	if (AGameStateBase* GameState = GetWorld()->GetGameState())
	{
		TArray<UActorComponent*> Components =
			GameState->GetComponentsByInterface(USpiderNavigationInterface::StaticClass());
		if (Components.Num() > 0)
			return Components[0];
	}

	SPIDER_LOG(Warning, TEXT("No valid ISpiderNavigationInterface found in GameState."));
	return nullptr;
}

void ASpiderAIController::DrawDebugPath() const
{
	if (!bDebug || CurrentPathPoints.Num() == 0 || !GetWorld())
		return;

	const FVector PawnLoc = GetPawn() ? GetPawn()->GetActorLocation() : FVector::ZeroVector;

	for (int32 i = 0; i < CurrentPathPoints.Num(); ++i)
	{
		const FVector& P = CurrentPathPoints[i];
		DrawDebugSphere(GetWorld(), P, 6.f, 8, FColor::Green, false, UpdateLocalDestinationInterval);

		if (i > 0)
		{
			DrawDebugLine(GetWorld(), CurrentPathPoints[i - 1], P, FColor::Green, false, UpdateLocalDestinationInterval);
		}
	}

	DrawDebugLine(GetWorld(), PawnLoc, LocalMoveDestination, FColor::Red, false, UpdateLocalDestinationInterval);
}
