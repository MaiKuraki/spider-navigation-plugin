// SpiderDebugRenderer.h
#pragma once

#include "CoreMinimal.h"
#include "HAL/ThreadSafeBool.h"
#include "Containers/Queue.h"

struct FSpiderDebugDrawCommand
{
    enum class EType { Line, Sphere, Point } Type;
    FVector A, B;
    FColor Color;
    float Size;
    float LifeTime;
};

class FSpiderDebugRenderer
{
public:
    static FSpiderDebugRenderer& Get()
    {
        static FSpiderDebugRenderer Instance;
        return Instance;
    }

    void EnqueueLine(const FVector& Start, const FVector& End, const FColor& Color, float LifeTime = 1.f);
    void EnqueueSphere(const FVector& Center, float Radius, const FColor& Color, float LifeTime = 1.f);
    void EnqueuePoint(const FVector& Pos, float Size, const FColor& Color, float LifeTime = 1.f);

    void FlushToWorld(UWorld* World);

private:
    FCriticalSection Mutex;
    TQueue<FSpiderDebugDrawCommand, EQueueMode::Spsc> Pending;
};
