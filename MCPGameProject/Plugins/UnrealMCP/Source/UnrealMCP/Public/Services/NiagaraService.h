#pragma once

#include "CoreMinimal.h"
#include "Services/INiagaraService.h"

// Log category for Niagara service - shared across all split implementation files
DECLARE_LOG_CATEGORY_EXTERN(LogNiagaraService, Log, All);

// Forward declarations
class UNiagaraSystem;
class UNiagaraEmitter;
struct FVersionedNiagaraEmitterData;
class UNiagaraRendererProperties;
class UNiagaraDataInterface;
class UNiagaraNodeFunctionCall;
struct FNiagaraEmitterHandle;

/**
 * Concrete implementation of INiagaraService
 * Provides Niagara VFX operations with proper error handling and logging
 */
class UNREALMCP_API FNiagaraService : public INiagaraService
{
public:
    /** Constructor */
    FNiagaraService();

    /** Destructor */
    virtual ~FNiagaraService() = default;

    /**
     * Get singleton instance
     * @return Reference to the singleton instance
     */
    static FNiagaraService& Get();

    // ========================================================================
    // INiagaraService interface implementation - Core Asset Management
    // ========================================================================

    virtual UNiagaraSystem* CreateSystem(const FNiagaraSystemCreationParams& Params, FString& OutSystemPath, FString& OutError) override;
    virtual UNiagaraEmitter* CreateEmitter(const FNiagaraEmitterCreationParams& Params, FString& OutEmitterPath, FString& OutError) override;
    virtual bool AddEmitterToSystem(const FString& SystemPath, const FString& EmitterPath, const FString& EmitterName, FGuid& OutEmitterHandleId, FString& OutError) override;
    virtual bool SetEmitterEnabled(const FString& SystemPath, const FString& EmitterName, bool bEnabled, FString& OutError) override;
    virtual bool RemoveEmitterFromSystem(const FString& SystemPath, const FString& EmitterName, FString& OutError) override;
    virtual bool SetEmitterProperty(const FNiagaraEmitterPropertyParams& Params, FString& OutError) override;
    virtual bool GetEmitterProperties(const FString& SystemPath, const FString& EmitterName, TSharedPtr<FJsonObject>& OutProperties, FString& OutError) override;
    virtual bool GetMetadata(const FString& AssetPath, const TArray<FString>* Fields, TSharedPtr<FJsonObject>& OutMetadata, const FString& EmitterName = TEXT(""), const FString& Stage = TEXT("")) override;
    virtual bool GetModuleInputs(const FString& SystemPath, const FString& EmitterName, const FString& ModuleName, const FString& Stage, TSharedPtr<FJsonObject>& OutInputs) override;
    virtual bool GetEmitterModules(const FString& SystemPath, const FString& EmitterName, TSharedPtr<FJsonObject>& OutModules) override;
    virtual bool CompileAsset(const FString& AssetPath, FString& OutError) override;
    virtual bool DuplicateSystem(const FString& SourcePath, const FString& NewName, const FString& FolderPath, FString& OutNewPath, FString& OutError) override;

    // ========================================================================
    // INiagaraService interface implementation - Module System
    // ========================================================================

    virtual bool AddModule(const FNiagaraModuleAddParams& Params, FString& OutModuleId, FString& OutError) override;
    virtual bool RemoveModule(const FNiagaraModuleRemoveParams& Params, FString& OutError) override;
    virtual bool SearchModules(const FString& SearchQuery, const FString& StageFilter, int32 MaxResults, TArray<TSharedPtr<FJsonObject>>& OutModules) override;
    virtual bool SetModuleInput(const FNiagaraModuleInputParams& Params, FString& OutError) override;
    virtual bool MoveModule(const FNiagaraModuleMoveParams& Params, FString& OutError) override;
    virtual bool SetModuleCurveInput(const FNiagaraModuleCurveInputParams& Params, FString& OutError) override;
    virtual bool SetModuleColorCurveInput(const FNiagaraModuleColorCurveInputParams& Params, FString& OutError) override;
    virtual bool SetModuleRandomInput(const FNiagaraModuleRandomInputParams& Params, FString& OutError) override;

    // ========================================================================
    // INiagaraService interface implementation - Parameters
    // ========================================================================

    virtual bool AddParameter(const FNiagaraParameterAddParams& Params, FString& OutError) override;
    virtual bool SetParameter(const FString& SystemPath, const FString& ParameterName, const TSharedPtr<FJsonValue>& Value, FString& OutError) override;

    // ========================================================================
    // INiagaraService interface implementation - Data Interfaces
    // ========================================================================

    virtual bool AddDataInterface(const FNiagaraDataInterfaceParams& Params, FString& OutInterfaceId, FString& OutError) override;
    virtual bool SetDataInterfaceProperty(const FString& SystemPath, const FString& EmitterName, const FString& InterfaceName, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue, FString& OutError) override;

    // ========================================================================
    // INiagaraService interface implementation - Renderers
    // ========================================================================

    virtual bool AddRenderer(const FNiagaraRendererParams& Params, FString& OutRendererId, FString& OutError) override;
    virtual bool SetRendererProperty(const FString& SystemPath, const FString& EmitterName, const FString& RendererName, const FString& PropertyName, const TSharedPtr<FJsonValue>& PropertyValue, FString& OutError) override;
    virtual bool GetRendererProperties(const FString& SystemPath, const FString& EmitterName, const FString& RendererName, TSharedPtr<FJsonObject>& OutProperties, FString& OutError) override;

    // ========================================================================
    // INiagaraService interface implementation - Level Integration
    // ========================================================================

    virtual ANiagaraActor* SpawnActor(const FNiagaraActorSpawnParams& Params, FString& OutActorName, FString& OutError) override;

    // ========================================================================
    // INiagaraService interface implementation - Utility Methods
    // ========================================================================

    virtual UNiagaraSystem* FindSystem(const FString& SystemPath) override;
    virtual UNiagaraEmitter* FindEmitter(const FString& EmitterPath) override;
    virtual void RefreshEditors(UObject* Asset) override;

private:
    /** Singleton instance */
    static TUniquePtr<FNiagaraService> Instance;

    // ========================================================================
    // Internal Helper Methods
    // ========================================================================

    /**
     * Get ENiagaraScriptUsage from stage string
     * @param Stage - "Spawn", "Update", or "Event"
     * @param OutUsage - Output script usage enum
     * @param OutError - Error message if conversion fails
     * @return true if conversion succeeded
     */
    bool GetScriptUsageFromStage(const FString& Stage, uint8& OutUsage, FString& OutError) const;

    /**
     * Get stage string from ENiagaraScriptUsage
     * @param Usage - Script usage enum value
     * @return Stage string
     */
    FString GetStageFromScriptUsage(uint8 Usage) const;

    /**
     * Find an emitter handle in a system by name
     * @param System - System to search
     * @param EmitterName - Name of the emitter to find
     * @param OutHandle - Output pointer to the handle
     * @return true if found
     */
    bool FindEmitterHandleByName(UNiagaraSystem* System, const FString& EmitterName, const FNiagaraEmitterHandle** OutHandle) const;

    /**
     * Find an emitter handle index in a system by name
     * @param System - System to search
     * @param EmitterName - Name of the emitter to find
     * @return Index or INDEX_NONE if not found
     */
    int32 FindEmitterHandleIndex(UNiagaraSystem* System, const FString& EmitterName) const;

    /**
     * Get versioned emitter data from a handle
     * @param Handle - Emitter handle
     * @return Versioned emitter data or nullptr
     */
    FVersionedNiagaraEmitterData* GetEmitterData(const FNiagaraEmitterHandle& Handle) const;

    /**
     * Create renderer properties by type string
     * @param RendererType - "Sprite", "Mesh", "Ribbon", "Light", "Decal", "Component"
     * @param Outer - Outer object for the renderer
     * @return Created renderer or nullptr
     */
    UNiagaraRendererProperties* CreateRendererByType(const FString& RendererType, UObject* Outer) const;

    /**
     * Create data interface by type string
     * @param InterfaceType - Data interface type
     * @param Outer - Outer object for the interface
     * @return Created interface or nullptr
     */
    UNiagaraDataInterface* CreateDataInterfaceByType(const FString& InterfaceType, UObject* Outer) const;

    /**
     * Add system metadata to JSON object
     * @param System - System to get metadata from
     * @param Fields - Optional fields filter
     * @param OutMetadata - Output JSON object
     * @param EmitterName - Optional emitter name filter (required for "modules" field)
     * @param Stage - Optional stage filter for "modules" field
     */
    void AddSystemMetadata(UNiagaraSystem* System, const TArray<FString>* Fields, TSharedPtr<FJsonObject>& OutMetadata, const FString& EmitterName = TEXT(""), const FString& Stage = TEXT("")) const;

    /**
     * Add emitter metadata to JSON object
     * @param Emitter - Emitter to get metadata from
     * @param Fields - Optional fields filter
     * @param OutMetadata - Output JSON object
     */
    void AddEmitterMetadata(UNiagaraEmitter* Emitter, const TArray<FString>* Fields, TSharedPtr<FJsonObject>& OutMetadata) const;

    /**
     * Create a unique package for an asset
     * @param Path - Base path
     * @param Name - Asset name
     * @param OutPackage - Output package
     * @param OutError - Error message if creation fails
     * @return true if package was created
     */
    bool CreateAssetPackage(const FString& Path, const FString& Name, UPackage*& OutPackage, FString& OutError) const;

    /**
     * Save an asset to disk
     * @param Asset - Asset to save
     * @param OutError - Error message if saving fails
     * @return true if saved successfully
     */
    bool SaveAsset(UObject* Asset, FString& OutError) const;

    /**
     * Mark a system dirty and notify changes
     * @param System - System to mark dirty
     */
    void MarkSystemDirty(UNiagaraSystem* System) const;
};
