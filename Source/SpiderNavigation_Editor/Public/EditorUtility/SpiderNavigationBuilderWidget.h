// Copyright Yves Tanas 2025

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "SpiderNavigationBuilderWidget.generated.h"

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
	/** For debug. Blueprint class which will be used to spawn actors on scene in specified volume */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpiderNavGridBuilder")
	TSubclassOf<class ASpiderNavGridTracer> TracerActorBP;

	/** For debug. Blueprint class which will be used to spawn Navigation Points */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpiderNavGridBuilder")
	TSubclassOf<class ASpiderNavPoint> NavPointActorBP;

	/** For debug. Blueprint class which will be used to spawn Navigation Points on egdes when checking possible neightbors */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpiderNavGridBuilder")
	TSubclassOf<class ASpiderNavPointEdge> NavPointEdgeActorBP;

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

protected:
	virtual void NativeConstruct() override;

private:
	TArray<TWeakObjectPtr<ASpiderNavGridTracer>> Tracers;
	TArray<TWeakObjectPtr<class ASpiderNavPoint>> NavPoints;

	TArray<FVector> NavPointsLocations;
	TMap<int32, FVector> NavPointsNormals;
	class ASpiderNavGridBuilder* Volume = nullptr;

private:
	void SpawnTracersAsync();
	void RemoveTracersClosedInVolumesAsync();
	void TraceFromAllTracersAsync();
	void SpawnNavPointsAsync();
	void BuildRelationsAsync();
	void BuildPossibleEdgeRelationsAsync();
	void RemoveNoConnectedAsync();

	void AddNavPointByHitResult(FHitResult RV_Hit);
	bool CheckNavPointsVisibility(class ASpiderNavPoint* NavPoint1, class ASpiderNavPoint* NavPoint2);
	bool GetLineLineIntersection(FVector Start0, FVector End0, FVector Start1, FVector End1, FVector& Intersection);
	void CheckAndAddIntersectionNavPointEdge(FVector Intersection, class ASpiderNavPoint* NavPoint1, class ASpiderNavPoint* NavPoint2);
	bool CheckNavPointCanSeeLocation(ASpiderNavPoint* NavPoint, FVector Location);

	float Dmnop(const TMap<int32, FVector>* v, int32 m, int32 n, int32 o, int32 p);

	void SaveGridAsync();
	int32 GetNavPointIndex(ASpiderNavPoint* NavPoint);

	void RemoveAllTracers();
	void RemoveAllNavPoints();

	void EmptyAllAsync();

	bool VolumeNotNull();


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
