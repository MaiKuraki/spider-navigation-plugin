#include "SpiderNavigation_Editor.h"

DEFINE_LOG_CATEGORY(SpiderNavigation_Editor);

#define LOCTEXT_NAMESPACE "FSpiderNavigation_Editor"

void FSpiderNavigation_Editor::StartupModule()
{
	UE_LOG(SpiderNavigation_Editor, Warning, TEXT("SpiderNavigation_Editor module has been loaded"));
}

void FSpiderNavigation_Editor::ShutdownModule()
{
	UE_LOG(SpiderNavigation_Editor, Warning, TEXT("SpiderNavigation_Editor module has been unloaded"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSpiderNavigation_Editor, SpiderNavigation_Editor)