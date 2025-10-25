// Copyright Yves Tanas 2025
#include "EditorUtility/SpiderNavigationBuilderWidget.h"
#include "SpiderNavGridTracer.h"
#include "SpiderNavPoint.h"
#include "SpiderNavPointEdge.h"
#include "SpiderNavGridBuilder.h"
#include "SpiderNavGridSaveGame.h"

#include "Async/Async.h"
#include "HAL/PlatformProcess.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"
#include "EditorUtilityWidgetComponents.h"
#include "GameFramework/Actor.h"
#include "Editor.h"

#define SPIDER_LOG(Category, Verbosity, Format, ...) \
    UE_LOG(Category, Verbosity, TEXT("[SpiderBuilder] %s() | Thread:%s | ") Format, \
        ANSI_TO_TCHAR(__FUNCTION__), IsInGameThread() ? TEXT("GameThread") : TEXT("WorkerThread"), ##__VA_ARGS__)

// ------------------------------------------------------------------------------------------------------------
// Constructor & Initialization
// ------------------------------------------------------------------------------------------------------------

USpiderNavigationBuilderWidget::USpiderNavigationBuilderWidget()
{
    SPIDER_LOG(LogTemp, Log, TEXT("Constructor started."));

    GridStepSize = 100.0f;
    bUseActorWhiteList = false;
    bUseActorBlackList = false;
    bAutoRemoveTracers = true;
    bAutoSaveGrid = true;

    TracerActorBP = ASpiderNavGridTracer::StaticClass();
    NavPointActorBP = ASpiderNavPoint::StaticClass();
    NavPointEdgeActorBP = ASpiderNavPointEdge::StaticClass();

    BounceNavDistance = 3.0f;
    TraceDistanceModificator = 1.5f;
    ClosePointsFilterModificator = 0.1f;
    ConnectionSphereRadiusModificator = 1.5f;
    TraceDistanceForEdgesModificator = 1.9f;
    EgdeDeviationModificator = 0.15f;
    TracersInVolumesCheckDistance = 100000.0f;
    bShouldTryToRemoveTracersEnclosedInVolumes = false;

    SPIDER_LOG(LogTemp, Log, TEXT("Constructor finished. Default params set."));
}

// ------------------------------------------------------------------------------------------------------------
// Native Construct (Editor Widget Init)
// ------------------------------------------------------------------------------------------------------------

void USpiderNavigationBuilderWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SPIDER_LOG(LogTemp, Log, TEXT("NativeConstruct() started."));

    UE_LOG(LogTemp, Warning, TEXT("[SpiderBuilder] Buttons - Generate:%s Clear:%s Save:%s Load:%s Debug:%s"),
        Generate ? TEXT("OK") : TEXT("NULL"),
        Clear ? TEXT("OK") : TEXT("NULL"),
        Save ? TEXT("OK") : TEXT("NULL"),
        Load ? TEXT("OK") : TEXT("NULL"),
        Debug ? TEXT("OK") : TEXT("NULL"));

    if (Generate)
    {
        Generate->OnClicked.AddDynamic(this, &USpiderNavigationBuilderWidget::OnGenerateClicked);
        SPIDER_LOG(LogTemp, Log, TEXT("Generate button bound."));
    }
    if (Clear)
    {
        Clear->OnClicked.AddDynamic(this, &USpiderNavigationBuilderWidget::OnClearClicked);
        SPIDER_LOG(LogTemp, Log, TEXT("Clear button bound."));
    }
    if (Save)
    {
        Save->OnClicked.AddDynamic(this, &USpiderNavigationBuilderWidget::OnSaveClicked);
        SPIDER_LOG(LogTemp, Log, TEXT("Save button bound."));
    }
    if (Load)
    {
        Load->OnClicked.AddDynamic(this, &USpiderNavigationBuilderWidget::OnLoadClicked);
        SPIDER_LOG(LogTemp, Log, TEXT("Load button bound."));
    }
    if (Debug)
    {
        Debug->OnClicked.AddDynamic(this, &USpiderNavigationBuilderWidget::OnDebugClicked);
        SPIDER_LOG(LogTemp, Log, TEXT("Debug button bound."));
    }

    SPIDER_LOG(LogTemp, Log, TEXT("NativeConstruct() completed."));
}

// ------------------------------------------------------------------------------------------------------------
// Button Events
// ------------------------------------------------------------------------------------------------------------

void USpiderNavigationBuilderWidget::OnClearClicked()
{
    SPIDER_LOG(LogTemp, Log, TEXT("Button clicked. World:%s"),
        GetWorld() ? *GetWorld()->GetName() : TEXT("NULL"));
    EmptyAllAsync();
}

void USpiderNavigationBuilderWidget::OnGenerateClicked()
{
    SPIDER_LOG(LogTemp, Log, TEXT("Generate clicked."));
    EmptyAllAsync();

    if (VolumeNotNull())
    {
        SPIDER_LOG(LogTemp, Log, TEXT("Volume is valid -> SpawnTracersAsync()"));
        SpawnTracersAsync();
    }
    else
    {
        SPIDER_LOG(LogTemp, Error, TEXT("Volume is NULL. Please assign valid volume actor."));
    }
}

void USpiderNavigationBuilderWidget::OnSaveClicked()
{
    SPIDER_LOG(LogTemp, Log, TEXT("Save clicked."));
    SaveGridAsync();
}

void USpiderNavigationBuilderWidget::OnLoadClicked()
{
    SPIDER_LOG(LogTemp, Log, TEXT("Load clicked."));
}
#include "Components/BoxComponent.h"
void USpiderNavigationBuilderWidget::OnDebugClicked()
{
    SPIDER_LOG(LogTemp, Log, TEXT("Debug clicked."));
}
 
#if WITH_EDITOR
// ------------------------------------------------------------------------------------------------------------
// SpawnTracersAsync
// ------------------------------------------------------------------------------------------------------------
void USpiderNavigationBuilderWidget::SpawnTracersAsync()
{
    SPIDER_LOG(LogTemp, Log, TEXT("SpawnTracersAsync() called."));

    if (!GEditor)
    {
        SPIDER_LOG(LogTemp, Error, TEXT("GEditor invalid!"));
        return;
    }

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        SPIDER_LOG(LogTemp, Error, TEXT("No valid EditorWorld."));
        return;
    }

    if (!TracerActorBP)
    {
        SPIDER_LOG(LogTemp, Error, TEXT("TracerActorBP not assigned."));
        return;
    }

    FVector Origin, BoxExtent;
    Volume->GetActorBounds(false, Origin, BoxExtent);

    UE_LOG(LogTemp, Log,
        TEXT("[SpiderBuilder] Using Volume Bounds: Origin=%s Extent=%s"),
        *Origin.ToString(), *BoxExtent.ToString());

    FVector GridStart = Origin - BoxExtent;
    FVector GridEnd = Origin + BoxExtent;
    FRotator DefaultRotator = FRotator::ZeroRotator;

    // Alle Spawn-Positionen vorbereiten
    TArray<FVector> SpawnPositions;
    for (float x = GridStart.X; x <= GridEnd.X; x += GridStepSize)
    {
        for (float y = GridStart.Y; y <= GridEnd.Y; y += GridStepSize)
        {
            for (float z = GridStart.Z; z <= GridEnd.Z; z += GridStepSize)
            {
                FVector Point(x, y, z);

                if (!Volume || !Volume->VolumeBox)
                    continue;

                // Box-Transformation
                const FTransform BoxTransform = Volume->VolumeBox->GetComponentTransform();
                const FVector LocalPoint = BoxTransform.InverseTransformPosition(Point);
                const FVector Extent = Volume->VolumeBox->GetUnscaledBoxExtent();

                // Punkt liegt innerhalb, wenn alle Achsen innerhalb des Extents liegen
                if (FMath::Abs(LocalPoint.X) <= Extent.X &&
                    FMath::Abs(LocalPoint.Y) <= Extent.Y &&
                    FMath::Abs(LocalPoint.Z) <= Extent.Z)
                {
                    SpawnPositions.Add(Point);
                }
            }
        }
    }

    SPIDER_LOG(LogTemp, Log, TEXT("Prepared %d spawn positions."), SpawnPositions.Num());

    Async(EAsyncExecution::Thread, [this, World, SpawnPositions, DefaultRotator]()
        {
            SPIDER_LOG(LogTemp, Log, TEXT("SpawnTracersAsync worker thread started."));

            int32 Total = SpawnPositions.Num();
            int32 Count = 0;

            for (const FVector& Loc : SpawnPositions)
            {
                AsyncTask(ENamedThreads::GameThread, [this, World, Loc, DefaultRotator]()
                    {
                        if (!TracerActorBP || !World) return;
                        FActorSpawnParameters Params;
                        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
                        Params.ObjectFlags |= RF_Transactional;

                        ASpiderNavGridTracer* Tracer =
                            World->SpawnActor<ASpiderNavGridTracer>(TracerActorBP, Loc, DefaultRotator, Params);

                        if (Tracer)
                        {
                            Tracers.Add(Tracer);
                            SPIDER_LOG(LogTemp, Verbose, TEXT("Spawned tracer at %s"), *Loc.ToString());
                        }
                    });

                Count++;
                if (Count % 100 == 0)
                    SPIDER_LOG(LogTemp, Log, TEXT("Spawn progress: %d/%d"), Count, Total);

                FPlatformProcess::Sleep(0.002f);
            }

            AsyncTask(ENamedThreads::GameThread, [this, Total]()
                {
                    SPIDER_LOG(LogTemp, Log, TEXT("Finished spawning %d tracers."), Total);
                    if (bShouldTryToRemoveTracersEnclosedInVolumes)
                    {
                            SPIDER_LOG(LogTemp, Log, TEXT("Launching RemoveTracersClosedInVolumesAsync()"));
                            RemoveTracersClosedInVolumesAsync();
                    }
                    else
                    {
                        SPIDER_LOG(LogTemp, Log, TEXT("Launching TraceFromAllTracersAsync()"));
                        TraceFromAllTracersAsync();
                    }
                });
        });
}
#endif

#if WITH_EDITOR
// ------------------------------------------------------------------------------------------------------------
// RemoveTracersClosedInVolumesAsync
// ------------------------------------------------------------------------------------------------------------
void USpiderNavigationBuilderWidget::RemoveTracersClosedInVolumesAsync()
{
    SPIDER_LOG(LogTemp, Log, TEXT("RemoveTracersClosedInVolumesAsync() called."));

    if (!GEditor)
    {
        SPIDER_LOG(LogTemp, Error, TEXT("GEditor not valid."));
        return;
    }

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        SPIDER_LOG(LogTemp, Error, TEXT("No valid EditorWorld."));
        return;
    }

    if (Tracers.Num() == 0)
    {
        SPIDER_LOG(LogTemp, Log, TEXT("No tracers to check."));
        return;
    }

    FCollisionQueryParams BaseTraceParams(FName(TEXT("RV_Trace")), false);
    BaseTraceParams.bTraceComplex = false;
    BaseTraceParams.bReturnPhysicalMaterial = false;
    BaseTraceParams.AddIgnoredActor(Volume);

    TArray<AActor*> ActorsToIgnore;
    for (auto& Ptr : Tracers)
        if (Ptr.IsValid()) ActorsToIgnore.Add(Ptr.Get());
    BaseTraceParams.AddIgnoredActors(ActorsToIgnore);

    const float TraceDistance = TracersInVolumesCheckDistance;
    const int32 Total = Tracers.Num();

    SPIDER_LOG(LogTemp, Log, TEXT("Checking %d tracers with TraceDistance=%.1f"), Total, TraceDistance);

    Async(EAsyncExecution::Thread, [this, BaseTraceParams, TraceDistance, Total]()
        {
            int32 Removed = 0;
            int32 Processed = 0;

            for (auto& Ptr : Tracers)
            {
                if (!Ptr.IsValid()) continue;
                ASpiderNavGridTracer* Tracer = Ptr.Get();
                FVector Start = Tracer->GetActorLocation();
                TArray<AActor*> ActorsBumpedIn;

                static const FVector Dirs[6] = {
                    FVector(1,0,0), FVector(-1,0,0),
                    FVector(0,1,0), FVector(0,-1,0),
                    FVector(0,0,1), FVector(0,0,-1)
                };

                for (const FVector& Dir : Dirs)
                {
                    FVector End = Start + Dir * TraceDistance;
                    AsyncTask(ENamedThreads::GameThread, [this, Start, End, BaseTraceParams, &ActorsBumpedIn]()
                        {
                            UWorld* World = GEditor->GetEditorWorldContext().World();
                            if (!World) return;
                            FHitResult Hit;
                            FCollisionQueryParams Params = BaseTraceParams;
                            if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
                                if (AActor* HitActor = Hit.GetActor())
                                    ActorsBumpedIn.Add(HitActor);
                        });
                    FPlatformProcess::Sleep(0.001f);
                }

                FPlatformProcess::Sleep(0.02f);

                if (ActorsBumpedIn.Num() == 6)
                {
                    AActor* First = ActorsBumpedIn[0];
                    bool Different = false;
                    for (AActor* A : ActorsBumpedIn)
                        if (A != First) { Different = true; break; }

                    if (!Different)
                    {
                        AsyncTask(ENamedThreads::GameThread, [this, Tracer]()
                            {
                                if (Tracer && Tracer->IsValidLowLevel())
                                    Tracer->Destroy();
                            });
                        Removed++;
                    }
                }

                Processed++;
                if (Processed % 100 == 0)
                    SPIDER_LOG(LogTemp, Log, TEXT("Checked %d/%d tracers (removed %d)"), Processed, Total, Removed);

                FPlatformProcess::Sleep(0.002f);
            }

            AsyncTask(ENamedThreads::GameThread, [this, Removed, Total]()
                {
                    SPIDER_LOG(LogTemp, Log, TEXT("Volume check done: %d/%d removed."), Removed, Total);
                    TraceFromAllTracersAsync();
                });
        });
}
#endif

#if WITH_EDITOR
// ------------------------------------------------------------------------------------------------------------
// TraceFromAllTracersAsync
// ------------------------------------------------------------------------------------------------------------
void USpiderNavigationBuilderWidget::TraceFromAllTracersAsync()
{
    SPIDER_LOG(LogTemp, Log, TEXT("TraceFromAllTracersAsync() called."));

    if (!GEditor)
    {
        SPIDER_LOG(LogTemp, Error, TEXT("GEditor not valid."));
        return;
    }

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        SPIDER_LOG(LogTemp, Error, TEXT("No valid EditorWorld."));
        return;
    }

    if (Tracers.Num() == 0)
    {
        SPIDER_LOG(LogTemp, Log, TEXT("No tracers to trace from."));
        return;
    }

    FCollisionQueryParams BaseTraceParams(FName(TEXT("SpiderTrace")), false);
    BaseTraceParams.bTraceComplex = false;
    BaseTraceParams.bReturnPhysicalMaterial = false;
    BaseTraceParams.AddIgnoredActor(Volume);

    TArray<AActor*> Ignored;
    for (auto& Ptr : Tracers)
        if (Ptr.IsValid()) Ignored.Add(Ptr.Get());
    BaseTraceParams.AddIgnoredActors(Ignored);

    const float TraceDist = GridStepSize * TraceDistanceModificator;
    const int32 Total = Tracers.Num();
    SPIDER_LOG(LogTemp, Log, TEXT("Preparing %d tracers, TraceDist=%.1f"), Total, TraceDist);

    Async(EAsyncExecution::Thread, [this, BaseTraceParams, TraceDist, Total]()
        {
            int32 Processed = 0;
            static const FVector Dirs[6] = {
                FVector(1,0,0), FVector(-1,0,0),
                FVector(0,1,0), FVector(0,-1,0),
                FVector(0,0,1), FVector(0,0,-1)
            };

            for (auto& Ptr : Tracers)
            {
                if (!Ptr.IsValid()) continue;
                ASpiderNavGridTracer* Tracer = Ptr.Get();
                FVector Start = Tracer->GetActorLocation();

                for (const FVector& Dir : Dirs)
                {
                    FVector End = Start + Dir * TraceDist;
                    AsyncTask(ENamedThreads::GameThread, [this, Start, End, BaseTraceParams]()
                        {
                            UWorld* World = GEditor->GetEditorWorldContext().World();
                            if (!World) return;
                            FHitResult Hit;
                            if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, BaseTraceParams))
                            {
                                AddNavPointByHitResult(Hit);
                                SPIDER_LOG(LogTemp, Verbose, TEXT("Trace hit at %s"), *Hit.ImpactPoint.ToString());
                            }
#if WITH_EDITOR
                            if (Hit.bBlockingHit)
                                DrawDebugLine(World, Start, Hit.ImpactPoint, FColor::Green, false, 2.0f, 0, 1.0f);
                            else
                                DrawDebugLine(World, Start, End, FColor::Red, false, 2.0f, 0, 1.0f);
#endif
                        });
                    FPlatformProcess::Sleep(0.0005f);
                }

                Processed++;
                if (Processed % 100 == 0)
                    SPIDER_LOG(LogTemp, Log, TEXT("Processed %d/%d tracers"), Processed, Total);

                FPlatformProcess::Sleep(0.002f);
            }

            AsyncTask(ENamedThreads::GameThread, [this, Total]()
                {
                    SPIDER_LOG(LogTemp, Log, TEXT("Finished tracing %d tracers."), Total);
                });

            if (bAutoRemoveTracers)
            {
                AsyncTask(ENamedThreads::GameThread, [this]()
                    {
                        SPIDER_LOG(LogTemp, Log, TEXT("Auto-removing tracers."));
                        RemoveAllTracers();
                    });
            }

            SPIDER_LOG(LogTemp, Log, TEXT("Starting SpawnNavPointsAsync()"));
            SpawnNavPointsAsync();
        });
}
#endif

// ------------------------------------------------------------------------------------------------------------
// AddNavPointByHitResult
// ------------------------------------------------------------------------------------------------------------
void USpiderNavigationBuilderWidget::AddNavPointByHitResult(FHitResult RV_Hit)
{
    SPIDER_LOG(LogTemp, Verbose, TEXT("AddNavPointByHitResult called. BlockingHit=%s"), RV_Hit.bBlockingHit ? TEXT("true") : TEXT("false"));

    if (!RV_Hit.bBlockingHit)
        return;

    AActor* BlockingActor = RV_Hit.GetActor();
    if (!BlockingActor)
    {
        SPIDER_LOG(LogTemp, Warning, TEXT("Hit actor invalid."));
        return;
    }

    // --- WhiteList ---
    if (bUseActorWhiteList)
    {
        bool bFound = false;
        for (AActor* A : ActorsWhiteList)
            if (A == BlockingActor)
            {
                bFound = true;
                break;
            }
        if (!bFound)
        {
            SPIDER_LOG(LogTemp, Verbose, TEXT("Actor %s not in whitelist."), *BlockingActor->GetName());
            return;
        }
    }

    // --- BlackList ---
    if (bUseActorBlackList)
    {
        for (AActor* B : ActorsBlackList)
            if (B == BlockingActor)
            {
                SPIDER_LOG(LogTemp, Verbose, TEXT("Actor %s in blacklist."), *BlockingActor->GetName());
                return;
            }
    }

    FVector NewLoc = RV_Hit.Location + RV_Hit.Normal * BounceNavDistance;
    bool bTooClose = false;

    for (const FVector& L : NavPointsLocations)
        if ((L - NewLoc).Size() < GridStepSize * ClosePointsFilterModificator)
        {
            bTooClose = true;
            break;
        }

    if (bTooClose)
    {
        SPIDER_LOG(LogTemp, Verbose, TEXT("NavPoint too close, skipped."));
        return;
    }

    int32 Idx = NavPointsLocations.Add(NewLoc);
    NavPointsNormals.Add(Idx, RV_Hit.Normal);
    SPIDER_LOG(LogTemp, Verbose, TEXT("Added NavPoint[%d] at %s"), Idx, *NewLoc.ToString());
}

// ------------------------------------------------------------------------------------------------------------
// SpawnNavPointsAsync
// ------------------------------------------------------------------------------------------------------------
#if WITH_EDITOR
void USpiderNavigationBuilderWidget::SpawnNavPointsAsync()
{
    SPIDER_LOG(LogTemp, Log, TEXT("SpawnNavPointsAsync() called. Locations=%d"), NavPointsLocations.Num());

    if (!GEditor) return;
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World || !NavPointActorBP) return;

    const int32 Total = NavPointsLocations.Num();
    if (Total == 0)
    {
        SPIDER_LOG(LogTemp, Log, TEXT("No NavPointsLocations defined."));
        return;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Params.ObjectFlags |= RF_Transactional;
    FRotator R = FRotator::ZeroRotator;

    Async(EAsyncExecution::Thread, [this, Params, R, Total]()
        {
            SPIDER_LOG(LogTemp, Log, TEXT("SpawnNavPointsAsync worker thread started. Total=%d"), Total);
            int32 Spawned = 0;

            for (int32 i = 0; i < NavPointsLocations.Num(); ++i)
            {
                FVector Loc = NavPointsLocations[i];
                AsyncTask(ENamedThreads::GameThread, [this, Loc, Params, R, i]()
                    {
                        UWorld* W = GEditor->GetEditorWorldContext().World();
                        if (!W || !NavPointActorBP) return;
                        ASpiderNavPoint* P = W->SpawnActor<ASpiderNavPoint>(NavPointActorBP, Loc, R, Params);
                        if (P)
                        {
                            NavPoints.Add(P);
                            if (FVector* N = NavPointsNormals.Find(i))
                                P->Normal = *N;
                            SPIDER_LOG(LogTemp, Verbose, TEXT("Spawned NavPoint[%d] at %s"), i, *Loc.ToString());
                        }
                    });

                Spawned++;
                if (Spawned % 100 == 0)
                    SPIDER_LOG(LogTemp, Log, TEXT("Spawned %d/%d NavPoints"), Spawned, Total);

                FPlatformProcess::Sleep(0.0015f);
            }

            AsyncTask(ENamedThreads::GameThread, [this, Total]()
                {
                    SPIDER_LOG(LogTemp, Log, TEXT("Finished spawning %d NavPoints. Starting BuildRelationsAsync()"), Total);
                    BuildRelationsAsync();
                });
        });
}
#endif

// ------------------------------------------------------------------------------------------------------------
// BuildRelationsAsync
// ------------------------------------------------------------------------------------------------------------
#if WITH_EDITOR
void USpiderNavigationBuilderWidget::BuildRelationsAsync()
{
    SPIDER_LOG(LogTemp, Log, TEXT("BuildRelationsAsync() called. NavPoints=%d"), NavPoints.Num());
    if (!GEditor) return;
    UWorld* W = GEditor->GetEditorWorldContext().World();
    if (!W || NavPoints.Num() == 0) return;

    const float Radius = GridStepSize * ConnectionSphereRadiusModificator;
    const int32 Total = NavPoints.Num();

    Async(EAsyncExecution::Thread, [this, Radius, Total]()
        {
            SPIDER_LOG(LogTemp, Log, TEXT("Worker started for BuildRelationsAsync (Total=%d)."), Total);
            int32 Processed = 0;

            for (auto& PPtr : NavPoints)
            {
                if (!PPtr.IsValid()) continue;
                ASpiderNavPoint* NP = PPtr.Get();

                AsyncTask(ENamedThreads::GameThread, [this, NP, Radius]()
                    {
                        UWorld* W = GEditor->GetEditorWorldContext().World();
                        if (!W || !NP) return;
                        FCollisionObjectQueryParams ObjQ(ECC_TO_BITFIELD(ECC_WorldDynamic));
                        FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);
                        FCollisionQueryParams Q(FName(TEXT("SpiderRelationsTrace")), false);
                        Q.AddIgnoredActor(NP);
                        Q.AddIgnoredActor(Volume);

                        TArray<FOverlapResult> Hits;
                        if (!W->OverlapMultiByObjectType(Hits, NP->GetActorLocation(), FQuat::Identity, ObjQ, Sphere, Q))
                            return;

                        for (const auto& O : Hits)
                        {
                            AActor* A = O.GetActor();
                            if (!A || !A->IsA(ASpiderNavPoint::StaticClass()) || A == NP) continue;

                            ASpiderNavPoint* Other = Cast<ASpiderNavPoint>(A);
                            bool Visible = CheckNavPointsVisibility(NP, Other);
                            if (Visible)
                            {
                                NP->Neighbors.AddUnique(Other);
                                Other->Neighbors.AddUnique(NP);
                            }
                            else
                                NP->PossibleEdgeNeighbors.AddUnique(Other);
                        }

                        SPIDER_LOG(LogTemp, Verbose, TEXT("Relations built for %s: Neigh=%d Possible=%d"),
                            *NP->GetName(), NP->Neighbors.Num(), NP->PossibleEdgeNeighbors.Num());
                    });

                Processed++;
                if (Processed % 50 == 0)
                    SPIDER_LOG(LogTemp, Log, TEXT("Processed %d/%d for relations"), Processed, Total);

                FPlatformProcess::Sleep(0.002f);
            }

            AsyncTask(ENamedThreads::GameThread, [this, Total]()
                {
                    SPIDER_LOG(LogTemp, Log, TEXT("Finished building relations for %d NavPoints. Starting BuildPossibleEdgeRelationsAsync()."), Total);
                    BuildPossibleEdgeRelationsAsync();
                });
        });
}
#endif

// ------------------------------------------------------------------------------------------------------------
// CheckNavPointsVisibility
// ------------------------------------------------------------------------------------------------------------
#if WITH_EDITOR
bool USpiderNavigationBuilderWidget::CheckNavPointsVisibility(ASpiderNavPoint* N1, ASpiderNavPoint* N2)
{
    if (!N1 || !N2) return false;
    UWorld* W = GEditor->GetEditorWorldContext().World();
    if (!W) return false;

    FHitResult Hit;
    FCollisionQueryParams Params(FName(TEXT("SpiderNavPointVisibility")), false);
    Params.bTraceComplex = false;
    Params.AddIgnoredActor(N1);
    Params.AddIgnoredActor(Volume);

    FVector Start = N1->GetActorLocation();
    FVector End = N2->GetActorLocation();

    bool Block = W->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

#if WITH_EDITOR
    DrawDebugLine(W, Start, Block ? Hit.ImpactPoint : End, Block ? FColor::Red : FColor::Green, false, 1.5f, 0, 0.5f);
#endif

    bool Result = (Block && Hit.GetActor() == N2);
    SPIDER_LOG(LogTemp, Verbose, TEXT("Visibility %s -> %s = %s"), *N1->GetName(), *N2->GetName(), Result ? TEXT("Visible") : TEXT("Blocked"));
    return Result;
}
#endif

// ------------------------------------------------------------------------------------------------------------
// BuildPossibleEdgeRelationsAsync
// ------------------------------------------------------------------------------------------------------------
#if WITH_EDITOR
void USpiderNavigationBuilderWidget::BuildPossibleEdgeRelationsAsync()
{
    SPIDER_LOG(LogTemp, Log, TEXT("BuildPossibleEdgeRelationsAsync() started. NavPoints=%d"), NavPoints.Num());
    if (!GEditor) return;
    UWorld* W = GEditor->GetEditorWorldContext().World();
    if (!W || NavPoints.Num() == 0) return;

    const float Step = GridStepSize * TraceDistanceForEdgesModificator;
    const int32 Total = NavPoints.Num();

    Async(EAsyncExecution::Thread, [this, Step, Total]()
        {
            SPIDER_LOG(LogTemp, Log, TEXT("Worker thread launched for BuildPossibleEdgeRelationsAsync."));
            int32 Processed = 0;

            for (auto& Ptr : NavPoints)
            {
                if (!Ptr.IsValid()) continue;
                ASpiderNavPoint* NP = Ptr.Get();

                for (ASpiderNavPoint* PN : NP->PossibleEdgeNeighbors)
                {
                    if (!PN) continue;
                    for (int32 x1 = -1; x1 <= 1; ++x1)
                        for (int32 y1 = -1; y1 <= 1; ++y1)
                            for (int32 z1 = -1; z1 <= 1; ++z1)
                                for (int32 x2 = -1; x2 <= 1; ++x2)
                                    for (int32 y2 = -1; y2 <= 1; ++y2)
                                        for (int32 z2 = -1; z2 <= 1; ++z2)
                                        {
                                            FVector D1(x1, y1, z1), D2(x2, y2, z2);
                                            if (D1.Size() != 1 || D2.Size() != 1) continue;
                                            if ((D1.GetAbs() - D2.GetAbs()).Size() == 0) continue;

                                            FVector S0 = NP->GetActorLocation(), E0 = S0 + D1 * Step;
                                            FVector S1 = PN->GetActorLocation(), E1 = S1 + D2 * Step;
                                            FVector I;
                                            if (GetLineLineIntersection(S0, E0, S1, E1, I))
                                            {
                                                AsyncTask(ENamedThreads::GameThread, [this, I, NP, PN]()
                                                    {
                                                        CheckAndAddIntersectionNavPointEdge(I, NP, PN);
                                                    });
                                            }
                                        }
                }

                AsyncTask(ENamedThreads::GameThread, [NP]() { if (NP) NP->PossibleEdgeNeighbors.Empty(); });
                Processed++;
                if (Processed % 50 == 0)
                    SPIDER_LOG(LogTemp, Log, TEXT("Processed %d/%d for edges"), Processed, Total);

                FPlatformProcess::Sleep(0.002f);
            }

            AsyncTask(ENamedThreads::GameThread, [this, Total]()
                {
                    SPIDER_LOG(LogTemp, Log, TEXT("Finished BuildPossibleEdgeRelationsAsync. Total=%d -> Starting RemoveNoConnectedAsync()."), Total);
                    RemoveNoConnectedAsync();
                });
        });
}
#endif

// ------------------------------------------------------------
// 🔹 Hilfsfunktionen – Thread-sicher & Editor-kompatibel
// ------------------------------------------------------------

// ------------------------------------------------------------------------------------------------------------
// GetLineLineIntersection
// ------------------------------------------------------------------------------------------------------------
bool USpiderNavigationBuilderWidget::GetLineLineIntersection(FVector S0, FVector E0, FVector S1, FVector E1, FVector& I)
{
    SPIDER_LOG(LogTemp, Verbose, TEXT("GetLineLineIntersection called."));
    TMap<int32, FVector> V{ {0,S0},{1,E0},{2,S1},{3,E1} };

    float d0232 = Dmnop(&V, 0, 2, 3, 2);
    float d3210 = Dmnop(&V, 3, 2, 1, 0);
    float d3232 = Dmnop(&V, 3, 2, 3, 2);
    float denom = (Dmnop(&V, 1, 0, 1, 0) * d3232 - Dmnop(&V, 3, 2, 1, 0) * Dmnop(&V, 3, 2, 1, 0));
    if (FMath::IsNearlyZero(denom))return false;

    float mu = (d0232 * d3210 - Dmnop(&V, 0, 2, 1, 0) * d3232) / denom;
    float t = (d0232 + mu * d3210) / d3232;

    FVector CA = S0 + mu * (E0 - S0);
    FVector CB = S1 + t * (E1 - S1);
    float Dist = (CA - CB).Size();
    bool OnSeg = (t > 0);

    if (Dist < GridStepSize * EgdeDeviationModificator && OnSeg) { I = CA; return true; }
    return false;
}

// ------------------------------------------------------------------------------------------------------------
// Dmnop helper
// ------------------------------------------------------------------------------------------------------------
float USpiderNavigationBuilderWidget::Dmnop(const TMap<int32, FVector>* V, int32 M, int32 N, int32 O, int32 P)
{
    if (!V)return 0.f;
    const FVector* Vm = V->Find(M), * Vn = V->Find(N), * Vo = V->Find(O), * Vp = V->Find(P);
    if (!Vm || !Vn || !Vo || !Vp)return 0.f;
    return FVector::DotProduct(*Vm - *Vn, *Vo - *Vp);
}

// ------------------------------------------------------------------------------------------------------------
// CheckNavPointCanSeeLocation
// ------------------------------------------------------------------------------------------------------------
bool USpiderNavigationBuilderWidget::CheckNavPointCanSeeLocation(ASpiderNavPoint* N, FVector L)
{
    if (!N || !GEditor)return false;
    UWorld* W = GEditor->GetEditorWorldContext().World();
    if (!W)return false;

    FHitResult Hit;
    FCollisionQueryParams Q(FName(TEXT("SpiderNav_Visibility")), false);
    Q.bTraceComplex = false; Q.AddIgnoredActor(N); Q.AddIgnoredActor(Volume);
    bool Block = W->LineTraceSingleByChannel(Hit, N->GetActorLocation(), L, ECC_Visibility, Q);

#if WITH_EDITOR
    DrawDebugLine(W, N->GetActorLocation(), L, Block ? FColor::Red : FColor::Green, false, 2.f, 0, 1.f);
#endif
    SPIDER_LOG(LogTemp, Verbose, TEXT("Visibility %s -> %s = %s"), *N->GetName(), *L.ToString(), Block ? TEXT("Blocked") : TEXT("Clear"));
    return !Block;
}

// ------------------------------------------------------------------------------------------------------------
// CheckAndAddIntersectionNavPointEdge
// ------------------------------------------------------------------------------------------------------------
void USpiderNavigationBuilderWidget::CheckAndAddIntersectionNavPointEdge(FVector I, ASpiderNavPoint* N1, ASpiderNavPoint* N2)
{
    SPIDER_LOG(LogTemp, Verbose, TEXT("CheckAndAddIntersectionNavPointEdge called at %s"), *I.ToString());
    if (!GEditor)return;
    UWorld* W = GEditor->GetEditorWorldContext().World();
    if (!W || !N1 || !N2)return;

    if (!CheckNavPointCanSeeLocation(N1, I) || !CheckNavPointCanSeeLocation(N2, I))return;

    FActorSpawnParameters P; P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ASpiderNavPointEdge* E = W->SpawnActor<ASpiderNavPointEdge>(NavPointEdgeActorBP, I, FRotator::ZeroRotator, P);
    if (E)
    {
        NavPoints.Add(E);
        N1->Neighbors.AddUnique(E); N2->Neighbors.AddUnique(E);
        E->Neighbors.AddUnique(N1); E->Neighbors.AddUnique(N2);
        FVector N = N1->Normal + N2->Normal; N.Normalize(); E->Normal = N;
#if WITH_EDITOR
        DrawDebugPoint(W, I, 8, FColor::Cyan, false, 3.f);
#endif
        SPIDER_LOG(LogTemp, Verbose, TEXT("Edge created at %s linking %s and %s"), *I.ToString(), *N1->GetName(), *N2->GetName());
    }
}

// ------------------------------------------------------------------------------------------------------------
// RemoveNoConnectedAsync
// ------------------------------------------------------------------------------------------------------------
#if WITH_EDITOR
void USpiderNavigationBuilderWidget::RemoveNoConnectedAsync()
{
    SPIDER_LOG(LogTemp, Log, TEXT("RemoveNoConnectedAsync() called. NavPoints=%d"), NavPoints.Num());
    if (!GEditor)return;
    UWorld* W = GEditor->GetEditorWorldContext().World();
    if (!W || NavPoints.Num() == 0)return;

    const int32 Total = NavPoints.Num();
    Async(EAsyncExecution::Thread, [this, Total]()
        {
            int32 Rem = 0, Proc = 0;
            TArray<TWeakObjectPtr<ASpiderNavPoint>>Keep;
            for (auto& P : NavPoints)
            {
                if (!P.IsValid())continue;
                ASpiderNavPoint* N = P.Get();
                if (!N)continue;
                if (N->Neighbors.Num() > 1)Keep.Add(N);
                else
                {
                    Rem++;
                    AsyncTask(ENamedThreads::GameThread, [N]()
                        {if (N && N->IsValidLowLevelFast())N->Destroy(); });
                }
                Proc++; if (Proc % 100 == 0)SPIDER_LOG(LogTemp, Log, TEXT("Checked %d/%d (Removed %d)"), Proc, Total, Rem);
                FPlatformProcess::Sleep(0.001f);
            }
            AsyncTask(ENamedThreads::GameThread, [this, Keep, Rem, Total]()
                {
                    NavPoints = Keep;
                    SPIDER_LOG(LogTemp, Log, TEXT("Finished RemoveNoConnectedAsync. %d removed, %d remain."), Rem, Keep.Num());
                    if (bAutoSaveGrid) { SPIDER_LOG(LogTemp, Log, TEXT("Auto-saving grid...")); SaveGridAsync(); }
                    SPIDER_LOG(LogTemp, Warning, TEXT("Grid built. NavPoints=%d"), NavPoints.Num());
                });
        });
}
#endif

// ------------------------------------------------------------------------------------------------------------
// SaveGridAsync
// ------------------------------------------------------------------------------------------------------------
#if WITH_EDITOR
void USpiderNavigationBuilderWidget::SaveGridAsync()
{
    SPIDER_LOG(LogTemp, Log, TEXT("SaveGridAsync() called. NavPoints=%d"), NavPoints.Num());
    if (NavPoints.Num() == 0)return;

    Async(EAsyncExecution::TaskGraphMainThread, [this]()
        {
            TMap<int32, FVector>Locs, Norms; TMap<int32, FSpiderNavRelations>Rel;
            for (int32 i = 0; i < NavPoints.Num(); ++i)
            {
                if (!NavPoints[i].IsValid())continue;
                ASpiderNavPoint* N = NavPoints[i].Get();
                if (!N)continue;
                Locs.Add(i, N->GetActorLocation());
                Norms.Add(i, N->Normal);
                FSpiderNavRelations R;
                for (ASpiderNavPoint* Nei : N->Neighbors)
                {
                    int32 Id = GetNavPointIndex(Nei);
                    if (Id != INDEX_NONE)R.Neighbors.Add(Id);
                }
                Rel.Add(i, R);
            }
            Async(EAsyncExecution::Thread, [Locs, Norms, Rel]()
                {
                    USpiderNavGridSaveGame* S = Cast<USpiderNavGridSaveGame>(
                        UGameplayStatics::CreateSaveGameObject(USpiderNavGridSaveGame::StaticClass()));
                    if (!S) { SPIDER_LOG(LogTemp, Error, TEXT("Failed to create SaveGame instance.")); return; }
                    S->NavLocations = Locs; S->NavNormals = Norms; S->NavRelations = Rel;
                    bool OK = UGameplayStatics::SaveGameToSlot(S, S->SaveSlotName, S->UserIndex);
                    if (OK)
                    {
                        SPIDER_LOG(LogTemp, Log, TEXT("Success SpiderNavGrid save (%d nodes)."), Locs.Num());
                    }
                    else
                    {
                        SPIDER_LOG(LogTemp, Error, TEXT("Failure SpiderNavGrid save (%d nodes)."), Locs.Num());
                    }
                });
        });
}
#endif

// ------------------------------------------------------------------------------------------------------------
// GetNavPointIndex
// ------------------------------------------------------------------------------------------------------------
int32 USpiderNavigationBuilderWidget::GetNavPointIndex(ASpiderNavPoint* NavPoint)
{
    if (!NavPoint)
        return -1;

    for (int32 i = 0; i < NavPoints.Num(); ++i)
    {
        const TWeakObjectPtr<ASpiderNavPoint>& WeakNavPoint = NavPoints[i];

        // WeakPtr muss gültig sein, sonst überspringen
        if (!WeakNavPoint.IsValid())
            continue;

        if (WeakNavPoint.Get() == NavPoint)
        {
            return i;
        }
    }
    return -1;
}





// ------------------------------------------------------------------------------------------------------------
// Utility Checks
// ------------------------------------------------------------------------------------------------------------

bool USpiderNavigationBuilderWidget::VolumeNotNull()
{
    SPIDER_LOG(LogTemp, Log, TEXT("Checking for valid ASpiderNavGridBuilder in world..."));
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
    {
        SPIDER_LOG(LogTemp, Error, TEXT("Editor world not found."));
        return false;
    }

    TArray<AActor*> FoundVolumes;
    UGameplayStatics::GetAllActorsOfClass(World, ASpiderNavGridBuilder::StaticClass(), FoundVolumes);

    SPIDER_LOG(LogTemp, Log, TEXT("Found %d SpiderNavGridBuilder volumes."), FoundVolumes.Num());

    if (FoundVolumes.Num() > 0)
    {
        Volume = Cast<ASpiderNavGridBuilder>(FoundVolumes[0]);
        SPIDER_LOG(LogTemp, Log, TEXT("Assigned Volume: %s"), *Volume->GetName());
    }
    else
    {
        SPIDER_LOG(LogTemp, Warning, TEXT("No ASpiderNavGridBuilder found in world."));
    }

    return Volume != nullptr;
}

// ------------------------------------------------------------------------------------------------------------
// EmptyAllAsync & Cleanup
// ------------------------------------------------------------------------------------------------------------

void USpiderNavigationBuilderWidget::EmptyAllAsync()
{
    SPIDER_LOG(LogTemp, Log, TEXT("EmptyAllAsync() started."));

    Async(EAsyncExecution::Thread, [this]()
        {
            SPIDER_LOG(LogTemp, Log, TEXT("Background thread launched for EmptyAllAsync."));

            AsyncTask(ENamedThreads::GameThread, [this]()
                {
                    SPIDER_LOG(LogTemp, Log, TEXT("Removing all Tracers (GameThread)."));
                    RemoveAllTracers();
                });

            FPlatformProcess::Sleep(0.05f);

            AsyncTask(ENamedThreads::GameThread, [this]()
                {
                    SPIDER_LOG(LogTemp, Log, TEXT("Removing all NavPoints (GameThread)."));
                    RemoveAllNavPoints();
                });

            NavPointsNormals.Empty();
            NavPointsLocations.Empty();
            SPIDER_LOG(LogTemp, Log, TEXT("Nav data arrays cleared."));
        });
}

void USpiderNavigationBuilderWidget::RemoveAllTracers()
{
    SPIDER_LOG(LogTemp, Log, TEXT("Removing %d Tracers..."), Tracers.Num());
    int32 DestroyedCount = 0;

    for (int32 i = 0; i < Tracers.Num(); ++i)
    {
        if (Tracers[i].IsValid())
        {
            SPIDER_LOG(LogTemp, Verbose, TEXT("Destroying Tracer[%d] = %s"), i, *Tracers[i]->GetName());
            Tracers[i]->Destroy();
            DestroyedCount++;
        }
        else
        {
            SPIDER_LOG(LogTemp, Warning, TEXT("Tracer[%d] invalid or already destroyed."), i);
        }
    }

    Tracers.Empty();
    SPIDER_LOG(LogTemp, Log, TEXT("Removed %d tracers successfully."), DestroyedCount);
}

void USpiderNavigationBuilderWidget::RemoveAllNavPoints()
{
    SPIDER_LOG(LogTemp, Log, TEXT("Removing %d NavPoints..."), NavPoints.Num());
    int32 DestroyedCount = 0;

    for (int32 i = 0; i < NavPoints.Num(); ++i)
    {
        if (NavPoints[i].IsValid())
        {
            ASpiderNavPoint* NavPoint = NavPoints[i].Get();
            SPIDER_LOG(LogTemp, Verbose, TEXT("Destroying NavPoint[%d] = %s"), i, *NavPoint->GetName());
            NavPoint->Destroy();
            DestroyedCount++;
        }
        else
        {
            SPIDER_LOG(LogTemp, Warning, TEXT("NavPoint[%d] invalid or already destroyed."), i);
        }
    }

    NavPoints.Empty();
    SPIDER_LOG(LogTemp, Log, TEXT("Removed %d navpoints successfully."), DestroyedCount);
}


