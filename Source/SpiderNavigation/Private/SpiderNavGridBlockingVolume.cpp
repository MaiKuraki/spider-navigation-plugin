// Copyright (C) 2022 Yves Tanas - All Rights Reserved

#include "SpiderNavGridBlockingVolume.h"

#include "Components/BoxComponent.h"

ASpiderNavGridBlockingVolume::ASpiderNavGridBlockingVolume()
{ 	
	PrimaryActorTick.bCanEverTick = false;

	if (BlockingVolume == nullptr)
		BlockingVolume = CreateDefaultSubobject<UBoxComponent>(FName("BlockingVolume"));

	RootComponent = BlockingVolume;
}

UBoxComponent* ASpiderNavGridBlockingVolume::GetBlockingVolume() const
{
	return BlockingVolume;
}



