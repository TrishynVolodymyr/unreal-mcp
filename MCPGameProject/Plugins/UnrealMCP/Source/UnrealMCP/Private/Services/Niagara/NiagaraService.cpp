#include "Services/NiagaraService.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "UObject/SavePackage.h"

// Niagara includes
#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraScript.h"
#include "NiagaraActor.h"
#include "NiagaraComponent.h"
#include "NiagaraRendererProperties.h"
#include "NiagaraDataInterface.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraMeshRendererProperties.h"
#include "NiagaraRibbonRendererProperties.h"
#include "NiagaraLightRendererProperties.h"
#include "NiagaraComponentRendererProperties.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraNodeInput.h"
#include "NiagaraScriptSource.h"

// Niagara Editor includes
#include "NiagaraSystemFactoryNew.h"
#include "NiagaraEmitterFactoryNew.h"
#include "NiagaraEditorUtilities.h"
#include "ViewModels/Stack/NiagaraStackGraphUtilities.h"
#include "ViewModels/Stack/NiagaraParameterHandle.h"
#include "EdGraphSchema_Niagara.h"
#include "NiagaraTypes.h"
#include "NiagaraParameterMapHistory.h"
#include "NiagaraCommon.h"  // For FNiagaraUtilities::ConvertVariableToRapidIterationConstantName

DEFINE_LOG_CATEGORY(LogNiagaraService);

// Singleton instance
TUniquePtr<FNiagaraService> FNiagaraService::Instance;

FNiagaraService::FNiagaraService()
{
    UE_LOG(LogNiagaraService, Log, TEXT("FNiagaraService initialized"));
}

FNiagaraService& FNiagaraService::Get()
{
    if (!Instance.IsValid())
    {
        Instance = MakeUnique<FNiagaraService>();
    }
    return *Instance;
}
