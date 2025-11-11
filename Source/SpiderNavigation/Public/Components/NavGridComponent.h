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
#include <algorithm>
#include <vector>         // std::vector
#include "Components/ActorComponent.h"
#include "Structs/SavedSpiderNavGrid.h"
#include "Interfaces/SpiderNavigationInterface.h"
#include "NavGridComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(NavGridComponent_LOG, Log, All);

UCLASS(BlueprintType, Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SPIDERNAVIGATION_API UNavGridComponent : public UActorComponent, public ISpiderNavigationInterface
{
	GENERATED_BODY()

public:	
	UNavGridComponent();

	/** Finds path in grid. Returns array of nodes */
	UFUNCTION(BlueprintCallable, Category = "SpiderNavigation")
	TArray<FVector> FindPath(FVector Start, FVector End, bool& bFoundCompletePath);

	/** Draws debug lines between connected nodes */
	UFUNCTION(BlueprintCallable, Category = "SpiderNavigation")
	void DrawDebugRelations();

	/** Finds closest node's location in grid to specified location */
	UFUNCTION(BlueprintCallable, Category = "SpiderNavigation")
	virtual FVector FindClosestNodeLocation_Implementation(FVector Location) override;

	/** Finds closest node's normal in grid to specified location  */
	UFUNCTION(BlueprintCallable, Category = "SpiderNavigation")
	FVector FindClosestNodeNormal(FVector Location);

	/** Finds path between current location and target location and returns location and normal of the next fisrt node in navigation grid */
	UFUNCTION(BlueprintCallable, Category = "SpiderNavigation")
	bool FindNextLocationAndNormal(FVector CurrentLocation, FVector TargetLocation, FVector& NextLocation, FVector& Normal);

protected:
	virtual void BeginPlay() override;

	void LoadGrid();


	void ResetGridMetrics();
	TArray<FVector> BuildPathFromEndNode(FSpiderNavNode* EndNode);

	FSpiderNavNode* FindClosestNode(FVector Location);

	TArray<FSpiderNavNode*> FindNodesPath(FSpiderNavNode* StartNode, FSpiderNavNode* EndNode, bool& bFoundCompletePath);
	TArray<FSpiderNavNode*> BuildNodesPathFromEndNode(FSpiderNavNode* EndNode);

	TArray<FSpiderNavNode*> OpenList;
	FSpiderNavNode* GetFromOpenList();
protected:
	/** Whether to load the navigation grid on BeginPlay */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpiderNavigation")
	bool bAutoLoadGrid;

	UPROPERTY(EditDefaultsOnly, Category = "Spider Navigation|Save")
	FString SaveGameName;
	UPROPERTY(EditDefaultsOnly, Category = "Spider Navigation|Save")
	int32 SaveSlotIndex;

	UPROPERTY(VisibleAnywhere, Category = "Spider Navigation|Save")
	FSavedSpiderNavGrid LoadedGrid;

	/** Thickness of debug lines */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpiderNavGridBuilder")
	float DebugLinesThickness;

private:
	TArray<class AActor*> SpiderNavGridsActors;

private:
	virtual TArray<class AActor*> GetAllSpiderNavGridsActors_Implementation() override;
	virtual TArray<FVector> FindPathBetweenPoints_Implementation(FVector StartLocation, FVector EndLocation) override;
	virtual void RegisterSpiderNavGridActor_Implementation(class AActor* SpiderNavGridActor) override;
	virtual void UnRegisterSpiderNavGridActor_Implementation(class AActor* SpiderNavGridActor) override;

	TArray<FSpiderNavNode*> FindNodesPath(FSpiderNavNode* StartNode, FSpiderNavNode* EndNode);
};
