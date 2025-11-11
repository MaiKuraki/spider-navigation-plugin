//The MIT License
//
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

#include "EditorUtility/SpiderNavigationBuilderWidget.h"
#include "Structs/SpiderDebugRenderer.h"
#include "Actors/SpiderNavGridBuilderVolume.h"
#include "SaveGame/SpiderNavGridSaveGame.h"
#include "Components/Button.h"
#include "Async/Async.h"
#include "HAL/PlatformProcess.h"
#include "Kismet/GameplayStatics.h"
#include "Editor.h"
#include "Containers/Ticker.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"

// ---------------------------------------------------
// Logging Helper
// ---------------------------------------------------
#define SPIDER_LOG(Category, Verbosity, Format, ...) \
    UE_LOG(Category, Verbosity, TEXT("[SpiderBuilder] %s() | Thread:%s | ") Format, \
        ANSI_TO_TCHAR(__FUNCTION__), IsInGameThread() ? TEXT("GT") : TEXT("Worker"), ##__VA_ARGS__)

// ---------------------------------------------------
// Tunables
// ---------------------------------------------------
namespace SpiderBuilderConfig
{
    static constexpr float WORKER_SLEEP_SEC = 0.0015f;   // sanftes Throttling im Worker
    static constexpr float DEBUG_FLUSH_TICK = 0.05f;     // 20 FPS Debug-Flush
}

// ---------------------------------------------------
// Small Utilities
// ---------------------------------------------------
static void EnqueueOnGameThreadDelayed(TFunction<void()>&& Func, float DelaySeconds)
{
    FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateLambda([F = MoveTemp(Func)](float) -> bool
            {
                AsyncTask(ENamedThreads::GameThread, F);
                return false;
            }),
        DelaySeconds
    );
}

// ===================================================
// Constructor & Init
// ===================================================
USpiderNavigationBuilderWidget::USpiderNavigationBuilderWidget()
{
    GridStepSize = 40.0f;
    BounceNavDistance = 5.0f;
    TraceDistanceModificator = 3.5f;
	ClosePointsFilterModificator = 0.25f;
    ConnectionSphereRadiusModificator = 2.2f;
	TraceDistanceForEdgesModificator = 4.f;
    EgdeDeviationModificator = .8f;
	TracersInVolumesCheckDistance = 10.0f;
	bShouldTryToRemoveTracersEnclosedInVolumes = true;
    bDebugDraw = true;
    DebugDrawTime = 1.0f;
}

void USpiderNavigationBuilderWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Buttons anbinden (falls im UMG gesetzt)
    if (Generate) { Generate->OnClicked.AddDynamic(this, &USpiderNavigationBuilderWidget::OnGenerateClicked); }
    if (Clear) { Clear->OnClicked.AddDynamic(this, &USpiderNavigationBuilderWidget::OnClearClicked); }
    if (Save) { Save->OnClicked.AddDynamic(this, &USpiderNavigationBuilderWidget::OnSaveClicked); }
    if (Load) { Load->OnClicked.AddDynamic(this, &USpiderNavigationBuilderWidget::OnLoadClicked); }
    if (Debug) { Debug->OnClicked.AddDynamic(this, &USpiderNavigationBuilderWidget::OnDebugClicked); }

    // Periodischer Flush des Debug-Renderers in die Editor-Welt
    static bool bRendererTickerRegistered = false;
    if (!bRendererTickerRegistered)
    {
        FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateLambda([](float)
                {
                    if (GEditor)
                    {
                        if (UWorld* W = GEditor->GetEditorWorldContext().World())
                        {
                            FSpiderDebugRenderer::Get().FlushToWorld(W);
                        }
                    }
                    return true;
                }),
            SpiderBuilderConfig::DEBUG_FLUSH_TICK
        );
        bRendererTickerRegistered = true;
    }

    SPIDER_LOG(LogTemp, Log, TEXT("NativeConstruct() ready."));
}

void USpiderNavigationBuilderWidget::NativeDestruct()
{
    // Nichts Spezielles nötig – alles non-blocking und ohne persistenten Ticker pro Widget
    Super::NativeDestruct();
    SPIDER_LOG(LogTemp, Log, TEXT("NativeDestruct()"));
}

// ===================================================
// Buttons
// ===================================================
void USpiderNavigationBuilderWidget::OnGenerateClicked()
{
    SPIDER_LOG(LogTemp, Log, TEXT("OnGenerateClicked"));
    // Optional: vorherige Daten löschen
    GeneratedNodes.Reset();
    SpawnTracersAsync();
}

void USpiderNavigationBuilderWidget::OnCancelGenerationClicked()
{

}

void USpiderNavigationBuilderWidget::OnClearClicked()
{
    SPIDER_LOG(LogTemp, Log, TEXT("OnClearClicked"));
    // Nur Debug-Layer leeren & Daten resetten
    GeneratedNodes.Reset();// DebugRenderer clear not implemented — just reset generated data
}

void USpiderNavigationBuilderWidget::OnSaveClicked()
{
    SPIDER_LOG(LogTemp, Log, TEXT("OnSaveClicked"));
    SaveGridFromData();
}

void USpiderNavigationBuilderWidget::OnLoadClicked()
{
    SPIDER_LOG(LogTemp, Log, TEXT("OnLoadClicked (not implemented)"));
}

void USpiderNavigationBuilderWidget::OnDebugClicked()
{
    bDebugDraw = !bDebugDraw;
    SPIDER_LOG(LogTemp, Log, TEXT("DebugDraw toggled: %s"), bDebugDraw ? TEXT("ON") : TEXT("OFF"));
}

// ===================================================
// Helpers
// ===================================================
bool USpiderNavigationBuilderWidget::EnsureVolume()
{
    if (Volume && Volume->IsValidLowLevelFast() && Volume->VolumeBox)
        return true;

    if (!GEditor) return false;
    UWorld* W = GEditor->GetEditorWorldContext().World();
    if (!W) return false;

    TArray<AActor*> Found;
    UGameplayStatics::GetAllActorsOfClass(W, ASpiderNavGridBuilderVolume::StaticClass(), Found);
    if (Found.Num() > 0)
    {
        Volume = Cast<ASpiderNavGridBuilderVolume>(Found[0]);
        if (Volume && Volume->VolumeBox)
        {
            SPIDER_LOG(LogTemp, Log, TEXT("Auto-assigned Volume: %s"), *Volume->GetName());
            return true;
        }
    }
    SPIDER_LOG(LogTemp, Warning, TEXT("No ASpiderNavGridBuilder with valid VolumeBox found in world."));
    return false;
}

// ===================================================
// Spawn "Tracers" → Struct-based grid points (NO Actors)
// ===================================================
void USpiderNavigationBuilderWidget::SpawnTracersAsync()
{
    if (!EnsureVolume())
        return;

    FVector Origin, BoxExtent;
    Volume->GetActorBounds(false, Origin, BoxExtent);

    const FVector   GridStart = Origin - BoxExtent;
    const FVector   GridEnd = Origin + BoxExtent;
    const FTransform BoxXf = Volume->VolumeBox->GetComponentTransform();
    const FVector   Extent = Volume->VolumeBox->GetUnscaledBoxExtent();

    // Reservierung (grobe Schätzung, vermeidet Reallocs)
    TArray<FSpiderNavNodeBuilder> LocalNodes;
    LocalNodes.Reserve(
        FMath::Max(1,
            int32(((GridEnd.X - GridStart.X) / GridStepSize) + 1) *
            int32(((GridEnd.Y - GridStart.Y) / GridStepSize) + 1) *
            int32(((GridEnd.Z - GridStart.Z) / GridStepSize) + 1)
        )
    );

    SPIDER_LOG(LogTemp, Log, TEXT("SpawnTracersAsync: Step=%.1f Bounds:%s -> %s"),
        GridStepSize, *GridStart.ToString(), *GridEnd.ToString());

    Async(EAsyncExecution::Thread, [this, GridStart, GridEnd, Extent, BoxXf, LocalNodes = MoveTemp(LocalNodes)]() mutable
        {
            int64 Pushed = 0;

            for (float x = GridStart.X; x <= GridEnd.X; x += GridStepSize)
            {
                for (float y = GridStart.Y; y <= GridEnd.Y; y += GridStepSize)
                {
                    for (float z = GridStart.Z; z <= GridEnd.Z; z += GridStepSize)
                    {
                        const FVector Point = FVector(x, y, z);
                        const FVector Local = BoxXf.InverseTransformPosition(Point);

                        if (FMath::Abs(Local.X) <= Extent.X &&
                            FMath::Abs(Local.Y) <= Extent.Y &&
                            FMath::Abs(Local.Z) <= Extent.Z)
                        {
                            LocalNodes.Emplace(FSpiderNavNodeBuilder{ Point, FVector::UpVector });
                            if (bDebugDraw)
                                FSpiderDebugRenderer::Get().EnqueuePoint(Point, 2, FColor::Yellow, 0.25f);

                            ++Pushed;
                            if ((Pushed % 50000) == 0)
                                SPIDER_LOG(LogTemp, Log, TEXT("Prepared %lld points..."), Pushed);
                        }
                    }
                }
                FPlatformProcess::Sleep(SpiderBuilderConfig::WORKER_SLEEP_SEC);
            }

            AsyncTask(ENamedThreads::GameThread, [this, LocalNodes = MoveTemp(LocalNodes)]() mutable
                {
                    GeneratedNodes = MoveTemp(LocalNodes);
                    SPIDER_LOG(LogTemp, Log, TEXT("Generated %d tracer nodes."), GeneratedNodes.Num());
                    TraceFromAllTracersAsync();
                });
        });
}

// ===================================================
// Tracing from nodes (NO Actors) → hit locations as nodes
// ===================================================
void USpiderNavigationBuilderWidget::TraceFromAllTracersAsync()
{
    if (!GEditor) return;
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)   return;

    const TArray<FSpiderNavNodeBuilder> LocalNodes = GeneratedNodes;
    if (LocalNodes.Num() == 0) return;

    // Tuning: wie viele Knoten pro Tick (jeder macht 6 Traces)
    const int32 NodesPerTick = FMath::Max(TracesPerTickHint, 200); // z.B. 2000–6000 auf 7950X3D
    const float TickDelay = FMath::Max(BuildThrottleDelay, 0.003f);

    TSharedRef<int32, ESPMode::ThreadSafe> Index = MakeShared<int32, ESPMode::ThreadSafe>(0);
    TSharedRef<TArray<FSpiderNavNodeBuilder>, ESPMode::ThreadSafe> HitNodes =
        MakeShared<TArray<FSpiderNavNodeBuilder>, ESPMode::ThreadSafe>();
    HitNodes->Reserve(LocalNodes.Num() * 3);

    static const FVector Dirs[6] = {
        FVector(1, 0, 0), FVector(-1, 0, 0),
        FVector(0, 1, 0), FVector(0,-1, 0),
        FVector(0, 0, 1), FVector(0, 0,-1)
    };

    const float TraceDist = GridStepSize * TraceDistanceModificator;

    FTickerDelegate Tick = FTickerDelegate::CreateWeakLambda(this,
        [this, World, LocalNodes, HitNodes, Index, NodesPerTick, TraceDist](float)->bool
        {
            if (!IsValid(World)) return false;

            const int32 Start = *Index;
            const int32 End = FMath::Min(Start + NodesPerTick, LocalNodes.Num());

            FCollisionQueryParams Q(FName(TEXT("SpiderTraceGT")), /*bTraceComplex*/false);
            if (Volume) Q.AddIgnoredActor(Volume);

            for (int32 i = Start; i < End; ++i)
            {
                const FVector Base = LocalNodes[i].Location;

                for (const FVector& Dir : Dirs)
                {
                    const FVector S = Base + Dir * 5.f;
                    const FVector E = S + Dir * TraceDist;

                    FHitResult Hit;
                    const bool bHit = World->LineTraceSingleByChannel(Hit, S, E, ECollisionChannel::ECC_Visibility, Q);
                    if (bHit && Hit.bBlockingHit)
                    {
                        const FVector L = Hit.Location + Hit.Normal * BounceNavDistance;
                        HitNodes->Emplace(FSpiderNavNodeBuilder{ L, Hit.Normal });

                        if (bDebugDraw)
                            FSpiderDebugRenderer::Get().EnqueueSphere(L, 6.f, FColor::Green, 0.75f);
                    }
                    else if (bDebugDraw)
                    {
                        FSpiderDebugRenderer::Get().EnqueueLine(S, E, FColor::Red, 0.25f);
                    }
                }
            }

            *Index = End;

            if (End >= LocalNodes.Num())
            {
                // fertig
                GeneratedNodes = MoveTemp(*HitNodes);
                SPIDER_LOG(LogTemp, Log, TEXT("TraceFromAllTracersAsync done. %d nodes."), GeneratedNodes.Num());
                BuildRelationsDataAsync();
                return false;
            }

            return true; // nächsten Tick weitermachen
        });

    FTSTicker::GetCoreTicker().AddTicker(Tick, TickDelay);
}



// ===================================================
// Build Relations (visibility edges between generated nodes)
// ===================================================
void USpiderNavigationBuilderWidget::BuildRelationsDataAsync()
{
    const int32 Total = GeneratedNodes.Num();
    if (Total == 0) return;
    if (!GEditor) return;

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World) return;

    // =====================================================
    // 🧠 Performance-Tuning Parameter
    // =====================================================
    const float Radius = GridStepSize * ConnectionSphereRadiusModificator;
    const float CellSize = Radius * RelationCellMultiplier; // typ. 1.25f
    const float RadiusSq = Radius * Radius;

    SPIDER_LOG(LogTemp, Log,
        TEXT("BuildRelationsDataAsync() started. Nodes=%d | CellSize=%.1f"),
        Total, CellSize);

    // =====================================================
    // 🧩 Step 1: Spatial Grid vorbereiten
    // =====================================================
    TMap<FIntVector, TArray<int32>> Grid;
    Grid.Reserve(Total);

    for (int32 i = 0; i < Total; ++i)
    {
        const FVector& P = GeneratedNodes[i].Location;
        const FIntVector Key(
            FMath::FloorToInt(P.X / CellSize),
            FMath::FloorToInt(P.Y / CellSize),
            FMath::FloorToInt(P.Z / CellSize));

        Grid.FindOrAdd(Key).Add(i);
    }

    SPIDER_LOG(LogTemp, Log, TEXT("Spatial grid built with %d cells."), Grid.Num());

    // =====================================================
    // 🧩 Step 2: Parallel Nachbarschaften berechnen
    // =====================================================
    TSharedRef<TArray<TSet<int32>>, ESPMode::ThreadSafe> Neighbors =
        MakeShared<TArray<TSet<int32>>, ESPMode::ThreadSafe>();
    Neighbors->SetNum(Total);

    // Multithreaded Relation Building
    ParallelFor(Total, [this, &Grid, Neighbors, CellSize, RadiusSq, World](int32 i)
        {
            // 🧩 Thread-lokales Set
            TSet<int32> LocalNeighbors;
            LocalNeighbors.Reserve(128);

            const FVector& A = GeneratedNodes[i].Location;
            const FIntVector Cell(
                FMath::FloorToInt(A.X / CellSize),
                FMath::FloorToInt(A.Y / CellSize),
                FMath::FloorToInt(A.Z / CellSize));

            FCollisionQueryParams Q(FName(TEXT("SpiderRelTrace")), false);
            if (Volume) Q.AddIgnoredActor(Volume);

            // Umgebung (3×3×3 Zellen prüfen)
            for (int32 dx = -1; dx <= 1; ++dx)
                for (int32 dy = -1; dy <= 1; ++dy)
                    for (int32 dz = -1; dz <= 1; ++dz)
                    {
                        const FIntVector NCell = Cell + FIntVector(dx, dy, dz);
                        const TArray<int32>* Bucket = Grid.Find(NCell);
                        if (!Bucket) continue;

                        for (int32 j : *Bucket)
                        {
                            if (j <= i) continue;
                            const FVector& B = GeneratedNodes[j].Location;
                            if (FVector::DistSquared(A, B) > RadiusSq)
                                continue;

                            FHitResult Hit;
                            if (!World->LineTraceSingleByChannel(Hit, A, B, ECC_Visibility, Q))
                            {
                                LocalNeighbors.Add(j);

                                if (bDebugDraw)
                                    FSpiderDebugRenderer::Get().EnqueueLine(A, B, FColor::Blue, 1.0f);
                            }
                        }
                    }

            // 🧵 Sicherer Merge (CriticalSection)
            {
                FScopeLock Lock(&RelationCritical);
                (*Neighbors)[i].Append(LocalNeighbors);
                for (int32 j : LocalNeighbors)
                    (*Neighbors)[j].Add(i);
            }

        }, EParallelForFlags::None);

    // =====================================================
    // 🧩 Step 3: Übergabe an GameThread
    // =====================================================
    AsyncTask(ENamedThreads::GameThread, [this, Neighbors]()
        {
            const int32 N = GeneratedNodes.Num();
            for (int32 i = 0; i < N; ++i)
            {
                GeneratedNodes[i].Neighbors.Reset();
                for (int32 j : (*Neighbors)[i])
                    GeneratedNodes[i].Neighbors.Add(j);
            }

            SPIDER_LOG(LogTemp, Log, TEXT("✅ BuildRelationsDataAsync completed safely. %d nodes processed."), N);
            SaveGridFromData();
        });
}

// ===================================================
// Save Grid (SaveGame object)
// ===================================================
#include "Subsystems/SpiderNavGridEditorSubsystem.h"

void USpiderNavigationBuilderWidget::SaveGridFromData()
{
    AsyncTask(ENamedThreads::GameThread, [Nodes = GeneratedNodes]()
        {
            USpiderNavGridSaveGame* Save =
                Cast<USpiderNavGridSaveGame>(UGameplayStatics::CreateSaveGameObject(USpiderNavGridSaveGame::StaticClass()));
            if (!Save)
            {
                UE_LOG(LogTemp, Error, TEXT("[SpiderBuilder] SaveGridFromData: Could not create SaveGame object"));
                return;
            }

            Save->NavLocations.Reset();
            Save->NavNormals.Reset();
            Save->NavRelations.Reset();

            for (int32 i = 0; i < Nodes.Num(); ++i)
            {
                Save->NavLocations.Add(i, Nodes[i].Location);
                Save->NavNormals.Add(i, Nodes[i].Normal);

                FSpiderNavRelations Rel;
                Rel.Neighbors = Nodes[i].Neighbors;
                Save->NavRelations.Add(i, Rel);
            }

            if(USpiderNavGridEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<USpiderNavGridEditorSubsystem>())
            {
                const bool OK = Subsystem->SaveGrid(Save->SaveSlotName, Save->UserIndex, Save);
                UE_LOG(LogTemp, Log, TEXT("[SpiderBuilder] Save %s (%d nodes)."), OK ? TEXT("SUCCESS") : TEXT("FAILED"), Nodes.Num());
            }
            else
            {
				UE_LOG(LogTemp, Warning, TEXT("[SpiderBuilder] SaveGridFromData: Could not get SpiderNavigationSubsystem, using fallback save."));
            }
        });
}
