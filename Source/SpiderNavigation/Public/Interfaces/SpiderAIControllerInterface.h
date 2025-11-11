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
#include "UObject/Interface.h"
#include "Navigation/PathFollowingComponent.h"
#include "SpiderAIControllerInterface.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FHandleMoveCompleted, bool);


// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class USpiderAIControllerInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for AI controllers in the SpiderNavigation system.
 * Use this to define common AI behavior such as target acquisition,
 * navigation, and visibility checks.
 */
class SPIDERNAVIGATION_API ISpiderAIControllerInterface
{
	GENERATED_BODY()

public:
	virtual FHandleMoveCompleted& GetHandleMoveCompleted() = 0;

	/** Returns the current target actor that the AI is focused on */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Spider|Target")
	AActor* GetTargetActor() const;

	/** Returns the world-space location of this AI or its controlled pawn */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Spider|Pawn")
	FVector GetLocation() const;

	/** Determines if this AI should perform visibility checks on its target */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Spider|Target")
	bool MustCheckTargetVisibility() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Spider|Move")
	void MoveTo(const FVector& Destination);
};
