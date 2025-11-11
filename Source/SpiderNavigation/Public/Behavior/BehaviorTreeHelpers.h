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
#include "UObject/NoExportTypes.h"
#include "BehaviorTree/BehaviorTreeTypes.h" 
#include "BehaviorTreeHelpers.generated.h"

/**
 * 
 */
UCLASS(abstract)
class SPIDERNAVIGATION_API UBehaviorTreeHelpers : public UObject
{
	GENERATED_BODY()
	
public:
	static FString AbortModeToText(EBTFlowAbortMode::Type Mode)
	{
		switch (Mode)
		{
		case EBTFlowAbortMode::None:          return TEXT("");                     // oder TEXT("(no aborts)")
		case EBTFlowAbortMode::Self:          return TEXT("(aborts self)");
		case EBTFlowAbortMode::LowerPriority: return TEXT("(aborts lower priority)");
		case EBTFlowAbortMode::Both:          return TEXT("(aborts both)");
		default:                               return TEXT("");
		}
	}
};
