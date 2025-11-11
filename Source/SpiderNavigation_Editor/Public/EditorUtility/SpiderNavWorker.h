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

#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "HAL/Event.h"
#include "Containers/Queue.h"

//#include "SpiderNavWorker.generated.h"
class USpiderNavigationBuilderWidget;

/**
 * Hintergrund-Worker f�r Spider Navigation Tasks
 */
class FSpiderNavWorker : public FRunnable
{
public:
    FSpiderNavWorker(USpiderNavigationBuilderWidget* InOwner);
    virtual ~FSpiderNavWorker() override;

    // FRunnable Interface
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;
    virtual void Exit() override;

    // Public API
    void EnqueueTask(TFunction<void()> InTask);
    bool IsRunning() const { return bRunning; }

private:
    TQueue<TFunction<void()>, EQueueMode::Mpsc> TaskQueue;
    FEvent* WakeEvent;
    FRunnableThread* Thread;
    FThreadSafeBool bRunning;
    USpiderNavigationBuilderWidget* Owner;
};
