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

#include "Subsystems/SpiderNavigationSubsystem.h"

#include "Kismet/GameplayStatics.h"

#include "SaveGame/SpiderNavGridSaveGame.h"

DEFINE_LOG_CATEGORY(SpiderNAVSubsystem_LOG);
void USpiderNavigationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void USpiderNavigationSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

FSavedSpiderNavGrid USpiderNavigationSubsystem::LoadGrid(FString GridSaveName, int32 GridIndex)
{
	UE_LOG(SpiderNAVSubsystem_LOG, Log, TEXT("Start loading Spider nav data"));

	FVector* NormalRef = NULL;
	FVector Normal;
	FSavedSpiderNavGrid SavedGrid;

	USpiderNavGridSaveGame* LoadGameInstance = Cast<USpiderNavGridSaveGame>(UGameplayStatics::CreateSaveGameObject(USpiderNavGridSaveGame::StaticClass()));
	LoadGameInstance = Cast<USpiderNavGridSaveGame>(UGameplayStatics::LoadGameFromSlot(LoadGameInstance->SaveSlotName, LoadGameInstance->UserIndex));
	if (LoadGameInstance) {
		UE_LOG(SpiderNAVSubsystem_LOG, Log, TEXT("After getting load game instance"));
		for (auto It = LoadGameInstance->NavLocations.CreateConstIterator(); It; ++It) {
			NormalRef = LoadGameInstance->NavNormals.Find(It.Key());
			if (NormalRef) {
				Normal = *NormalRef;
			}
			else {
				Normal = FVector(0.0f, 0.0f, 1.0f);
			}
			AddGridNode(SavedGrid, It.Key(), It.Value(), Normal);
		}
		UE_LOG(SpiderNAVSubsystem_LOG, Log, TEXT("After setting locations"));

		for (auto It = LoadGameInstance->NavRelations.CreateConstIterator(); It; ++It) {
			SetGridNodeNeighbors(SavedGrid, It.Key(), It.Value().Neighbors);
		}
		UE_LOG(SpiderNAVSubsystem_LOG, Log, TEXT("After setting relations"));

		UE_LOG(SpiderNAVSubsystem_LOG, Log, TEXT("Nav Nodes Loaded: %d"), SavedGrid.GetNavNodesCount());
	}
	return SavedGrid;
}

void USpiderNavigationSubsystem::AddGridNode(FSavedSpiderNavGrid& SavedGrid, int32 SavedIndex, FVector Location, FVector Normal)
{
	FSpiderNavNode NavNode;
	NavNode.Location = Location;
	NavNode.Normal = Normal;

	int32 Index = SavedGrid.NavNodes.Add(NavNode);
	SavedGrid.NavNodes[Index].Index = Index;

	SavedGrid.NodesSavedIndexes.Add(SavedIndex, Index);
}

void USpiderNavigationSubsystem::SetGridNodeNeighbors(FSavedSpiderNavGrid& SavedGrid, int32 SavedIndex, TArray<int32> NeighborsSavedIndexes)
{
	int32* Index = SavedGrid.NodesSavedIndexes.Find(SavedIndex);
	if (Index) {
		FSpiderNavNode* NavNode = &(SavedGrid.NavNodes[*Index]);
		for (int32 i = 0; i != NeighborsSavedIndexes.Num(); ++i) {
			int32* NeighborIndex = SavedGrid.NodesSavedIndexes.Find(NeighborsSavedIndexes[i]);
			if (NeighborIndex) {
				FSpiderNavNode* NeighborNode = &(SavedGrid.NavNodes[*NeighborIndex]);
				NavNode->Neighbors.Add(NeighborNode);
			}
		}
	}
}

