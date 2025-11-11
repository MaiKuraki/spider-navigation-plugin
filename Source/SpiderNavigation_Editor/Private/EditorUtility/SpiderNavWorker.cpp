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

// SpiderNavWorker.cpp
#include "EditorUtility/SpiderNavWorker.h"
#include "EditorUtility/SpiderNavigationBuilderWidget.h"
#include "HAL/PlatformProcess.h"
#include "Async/Async.h"

FSpiderNavWorker::FSpiderNavWorker(USpiderNavigationBuilderWidget* InOwner)
    : WakeEvent(FPlatformProcess::GetSynchEventFromPool(false))
    , Thread(nullptr)
    , bRunning(true)
    , Owner(InOwner)
{
    Thread = FRunnableThread::Create(this, TEXT("SpiderNavWorker"), 0, TPri_BelowNormal);
    UE_LOG(LogTemp, Log, TEXT("[SpiderWorker] Thread created."));
}

FSpiderNavWorker::~FSpiderNavWorker()
{
    Stop();
    if (Thread)
    {
        Thread->Kill(true);
        delete Thread;
        Thread = nullptr;
    }
    FPlatformProcess::ReturnSynchEventToPool(WakeEvent);
    WakeEvent = nullptr;

    UE_LOG(LogTemp, Log, TEXT("[SpiderWorker] Thread destroyed."));
}

bool FSpiderNavWorker::Init()
{
    UE_LOG(LogTemp, Log, TEXT("[SpiderWorker] Init OK."));
    return true;
}

uint32 FSpiderNavWorker::Run()
{
    while (bRunning)
    {
        TFunction<void()> Task;
        if (TaskQueue.Dequeue(Task))
        {
            // Führe Task aus (Worker-Kontext)
            Task();
        }
        else
        {
            // Keine Tasks → warten, um CPU zu schonen
            WakeEvent->Wait(100); // 100ms oder bis Signal
        }
    }
    return 0;
}

void FSpiderNavWorker::Stop()
{
    bRunning = false;
    if (WakeEvent)
        WakeEvent->Trigger();
}

void FSpiderNavWorker::Exit()
{
    UE_LOG(LogTemp, Log, TEXT("[SpiderWorker] Exiting."));
}

void FSpiderNavWorker::EnqueueTask(TFunction<void()> InTask)
{
    TaskQueue.Enqueue(MoveTemp(InTask));
    WakeEvent->Trigger();
}
