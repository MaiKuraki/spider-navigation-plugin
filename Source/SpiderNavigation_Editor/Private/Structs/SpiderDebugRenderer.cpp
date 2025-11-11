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

#include "Structs/SpiderDebugRenderer.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

void FSpiderDebugRenderer::EnqueueLine(const FVector& Start, const FVector& End, const FColor& Color, float LifeTime)
{
    FSpiderDebugDrawCommand Cmd;
    Cmd.Type = FSpiderDebugDrawCommand::EType::Line;
    Cmd.A = Start;
    Cmd.B = End;
    Cmd.Color = Color;
    Cmd.LifeTime = LifeTime;
    Pending.Enqueue(MoveTemp(Cmd));
}

void FSpiderDebugRenderer::EnqueueSphere(const FVector& Center, float Radius, const FColor& Color, float LifeTime)
{
    FSpiderDebugDrawCommand Cmd;
    Cmd.Type = FSpiderDebugDrawCommand::EType::Sphere;
    Cmd.A = Center;
    Cmd.Size = Radius;
    Cmd.Color = Color;
    Cmd.LifeTime = LifeTime;
    Pending.Enqueue(MoveTemp(Cmd));
}

void FSpiderDebugRenderer::EnqueuePoint(const FVector& Pos, float Size, const FColor& Color, float LifeTime)
{
    FSpiderDebugDrawCommand Cmd;
    Cmd.Type = FSpiderDebugDrawCommand::EType::Point;
    Cmd.A = Pos;
    Cmd.Size = Size;
    Cmd.Color = Color;
    Cmd.LifeTime = LifeTime;
    Pending.Enqueue(MoveTemp(Cmd));
}

void FSpiderDebugRenderer::FlushToWorld(UWorld* World)
{
    if (!World)
        return;

    FSpiderDebugDrawCommand Cmd;
    while (Pending.Dequeue(Cmd))
    {
        switch (Cmd.Type)
        {
        case FSpiderDebugDrawCommand::EType::Line:
            DrawDebugLine(World, Cmd.A, Cmd.B, Cmd.Color, false, Cmd.LifeTime, 0, 1.f);
            break;

        case FSpiderDebugDrawCommand::EType::Sphere:
            DrawDebugSphere(World, Cmd.A, Cmd.Size, 8, Cmd.Color, false, Cmd.LifeTime);
            break;

        case FSpiderDebugDrawCommand::EType::Point:
            DrawDebugPoint(World, Cmd.A, Cmd.Size, Cmd.Color, false, Cmd.LifeTime);
            break;
        }
    }
}
