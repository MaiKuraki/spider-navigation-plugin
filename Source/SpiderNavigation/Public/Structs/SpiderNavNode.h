//The MIT License
//
//Copyright(C) 2017 Roman Nix
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

#include "SpiderNavNode.generated.h"

/** Describes navigation point in grid */
USTRUCT()
struct FSpiderNavNode
{
	GENERATED_BODY()

	/** Location of node */
	UPROPERTY()
	FVector Location;

	/** Normal of node from nearest world object with collision */
	UPROPERTY()
	FVector Normal;

	/** Index (id) of node */
	UPROPERTY()
	int32 Index;

	/** Relations */
	TArray <FSpiderNavNode*> Neighbors;

	/** F-value of node from A-star */
	float F;

	/** G-value of node from A-star */
	float G;

	/** H-value of node from A-star */
	float H;

	/** Opened property of node from A-star */
	bool Opened;

	/** Closed propery of node from A-star */
	bool Closed;

	/** Index (id) of parent node from A-star */
	int32 ParentIndex;

	/** Initialization of node */
	FSpiderNavNode()
	{
		Location = FVector(0.0f, 0.0f, 0.0f);
		Index = -1;
		F = 0.0f;
		G = 0.0f;
		H = 0.0f;
		Opened = false;
		Closed = false;
		ParentIndex = -1;

		Neighbors.Empty();
	}

	/** Resets metriks of A-star */
	void ResetMetrics()
	{
		F = 0.0f;
		G = 0.0f;
		H = 0.0f;
		Opened = false;
		Closed = false;
		ParentIndex = -1;
	}
};

/** Comparator < of nodes by F-value */
struct LessThanByNodeF {
	bool operator()(const FSpiderNavNode* lhs, const FSpiderNavNode* rhs) const {
		return lhs->F > rhs->F;
	}
};

/**  Comparator < of nodes by H-value */
struct LessThanByNodeH {
	bool operator()(const FSpiderNavNode* lhs, const FSpiderNavNode* rhs) const {
		return lhs->H > rhs->H;
	}
};