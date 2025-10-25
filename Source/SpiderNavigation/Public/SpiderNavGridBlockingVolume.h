// /* Copyright (C) 2020 Yves Tanas - All Rights Reserved */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpiderNavGridBlockingVolume.generated.h"

UCLASS()
class SPIDERNAVIGATION_API ASpiderNavGridBlockingVolume : public AActor
{
	GENERATED_BODY()
	
public:	
	ASpiderNavGridBlockingVolume();

public:	
	class UBoxComponent* GetBlockingVolume() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Components)
	class UBoxComponent* BlockingVolume;
};
