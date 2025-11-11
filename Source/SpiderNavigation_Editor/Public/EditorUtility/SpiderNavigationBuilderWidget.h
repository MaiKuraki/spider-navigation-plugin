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

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "Structs/SpiderNavNodeBuilder.h"
#include "EditorUtility/SpiderNavWorker.h"
#include "Templates/SharedPointer.h"          
#include "Templates/SharedPointerFwd.h"          
#include "Templates/SharedPointerInternals.h"    
#include "SpiderNavigationBuilderWidget.generated.h"
// Forward-declare batch constants from SpiderNavigationBuilderWidget.cpp
namespace SpiderBuilderConfig
{
	static constexpr int32 SAFE_SPAWN_MAX_RETRY = 120; // ~6 Min bei 3s Delay
}

/**
 * 
 */
UCLASS()
class SPIDERNAVIGATION_EDITOR_API USpiderNavigationBuilderWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()

	USpiderNavigationBuilderWidget();

protected:

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	class UButton* CancelGeneration;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	class UButton* Clear;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	class UButton* Generate;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	class UButton* Save;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	class UButton* Load;
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	class UButton* Debug;
protected:
	UPROPERTY(EditAnywhere, Category = "Spider|Performance")
	int32 TracesPerTickHint = 2000; // Wie viele Traces pro Tick (je 6 pro Knoten)
	UPROPERTY(EditAnywhere, Category = "Spider|Performance")
	float BuildThrottleDelay = 0.01f; // Sekunden zwischen Batches

	UPROPERTY(EditAnywhere, Category = "Spider|Performance")
	int32 BatchesPerTick = 500; // Wie viele Batches pro Tick abgearbeitet werden
	// Tuning-Werte im Header
	UPROPERTY(EditAnywhere, Category = "Spider|Performance")
	int32 ParallelBatchSize = 10000;

	UPROPERTY(EditAnywhere, Category = "Spider|Performance")
	float RelationCellMultiplier = 1.25f;

	/** The minimum distance between tracers to fill up scene */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpiderNavGridBuilder")
	float GridStepSize;

	/** The list of actors which could have navigation points on them */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpiderNavGridBuilder")
	TArray<AActor*> ActorsWhiteList;

	/** Whether to use ActorsWhiteList */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpiderNavGridBuilder")
	bool bUseActorWhiteList;

	/** The list of actors which COULD NOT have navigation points on them */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpiderNavGridBuilder")
	TArray<AActor*> ActorsBlackList;

	/** Whether to use ActorsBlackList */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpiderNavGridBuilder")
	bool bUseActorBlackList;

	/** For debug. If false then all tracers remain on the scene after grid rebuild */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpiderNavGridBuilder")
	bool bAutoRemoveTracers;

	/** Whether to save the navigation grid after rebuild */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpiderNavGridBuilder")
	bool bAutoSaveGrid;

	/** How far put navigation point from a WorldStatic face */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpiderNavGridBuilder")
	float BounceNavDistance;

	/** How far to trace from tracers. Multiplier of GridStepSize */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpiderNavGridBuilder")
	float TraceDistanceModificator;

	/** How close navigation points can be to each other. Multiplier of GridStepSize */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpiderNavGridBuilder")
	float ClosePointsFilterModificator;

	/** The radius of a sphere to find neighbors of each NavPoint. Multiplier of GridStepSize */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpiderNavGridBuilder")
	float ConnectionSphereRadiusModificator;

	/** How far to trace from each NavPoint to find intersection through egdes of possible neightbors. Multiplier of GridStepSize */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpiderNavGridBuilder")
	float TraceDistanceForEdgesModificator;

	/** How far can be one trace line from other trace line near the point of intersection when checking possible neightbors. Multiplier of GridStepSize */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpiderNavGridBuilder")
	float EgdeDeviationModificator;

	/** Whether should try to remove tracers enclosed in volumes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpiderNavGridBuilder")
	bool bShouldTryToRemoveTracersEnclosedInVolumes;

	/** Distance threshold to remove tracers enclosed in volumes  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpiderNavGridBuilder")
	float TracersInVolumesCheckDistance;
	// Debug-Option
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bDebugDraw = true;

	// Visualisierungsparameter
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	float DebugSphereRadius = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	float DebugDrawTime = 3.0f;
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:

	TArray<FVector> NavPointsLocations;
	TMap<int32, FVector> NavPointsNormals;
	class ASpiderNavGridBuilderVolume* Volume = nullptr;
	// Neues Datenfeld
	TSharedPtr<FSpiderNavWorker> Worker;
	UPROPERTY()
	TArray<FSpiderNavNodeBuilder> GeneratedNodes;

	FCriticalSection RelationCritical;

private:
	void SpawnTracersAsync();
	void TraceFromAllTracersAsync();
	void BuildRelationsDataAsync();

	void AddNavPointByHitResult(FHitResult RV_Hit);
	bool CheckNavPointsVisibility(class ASpiderNavPoint* NavPoint1, class ASpiderNavPoint* NavPoint2);
	bool GetLineLineIntersection(FVector Start0, FVector End0, FVector Start1, FVector End1, FVector& Intersection);
	void CheckAndAddIntersectionNavPointEdge(FVector Intersection, class ASpiderNavPoint* NavPoint1, class ASpiderNavPoint* NavPoint2);
	bool CheckNavPointCanSeeLocation(ASpiderNavPoint* NavPoint, FVector Location);

	float Dmnop(const TMap<int32, FVector>* v, int32 m, int32 n, int32 o, int32 p);
	void SaveGridFromData();
	//bool IsTracerInsideGeometry(UWorld* W, const FVector& Origin, const FCollisionQueryParams& Params) const;
	int32 GetNavPointIndex(ASpiderNavPoint* NavPoint);

	void RemoveAllTracers();
	void RemoveAllNavPoints();

	bool EnsureVolume();

	UFUNCTION()
	void OnCancelGenerationClicked();
	UFUNCTION()
	void OnClearClicked();
	UFUNCTION()
	void OnGenerateClicked();
	UFUNCTION()
	void OnSaveClicked();
	UFUNCTION()
	void OnLoadClicked();
	UFUNCTION()
	void OnDebugClicked();
};
