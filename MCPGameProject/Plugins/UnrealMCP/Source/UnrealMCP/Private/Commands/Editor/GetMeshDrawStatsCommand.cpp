#include "Commands/Editor/GetMeshDrawStatsCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Engine.h"
#include "Editor.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

FString FGetMeshDrawStatsCommand::GetCommandName() const
{
	return TEXT("get_mesh_draw_stats");
}

bool FGetMeshDrawStatsCommand::ValidateParams(const FString& Parameters) const
{
	return true;
}

FString FGetMeshDrawStatsCommand::Execute(const FString& Parameters)
{
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();

	// Parse optional mesh_filter parameter
	TSharedPtr<FJsonObject> Params;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
	FJsonSerializer::Deserialize(Reader, Params);

	FString MeshFilter;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("mesh_filter"), MeshFilter);
	}

	// Trigger dump: r.MeshDrawCommands.DumpStats requests stats collection
	// for the next rendered frame, then writes CSV at end of that frame.
	if (GEngine)
	{
		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		GEngine->Exec(World, TEXT("r.MeshDrawCommands.DumpStats"));
	}

	// Find CSV files in Saved/Profiling/
	FString ProfilingDir = FPaths::ProfilingDir();
	TArray<FString> FoundFiles;
	IFileManager::Get().FindFiles(
		FoundFiles,
		*FPaths::Combine(ProfilingDir, TEXT("MeshDrawCommandStats*.csv")),
		true, false);

	if (FoundFiles.Num() == 0)
	{
		// No CSV yet — stats collection was just requested
		Result->SetBoolField(TEXT("success"), true);
		Result->SetBoolField(TEXT("stats_pending"), true);
		Result->SetStringField(TEXT("message"),
			TEXT("Mesh draw stats dump requested. Call again after one rendered frame for data."));

		FString Out;
		TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
		FJsonSerializer::Serialize(Result.ToSharedRef(), W);
		return Out;
	}

	// Sort to find most recent file
	FoundFiles.Sort();
	FString LatestCSV = FPaths::Combine(ProfilingDir, FoundFiles.Last());

	// Parse CSV
	FString CSVContent;
	if (!FFileHelper::LoadFileToString(CSVContent, *LatestCSV))
	{
		Result->SetBoolField(TEXT("success"), false);
		Result->SetStringField(TEXT("error"),
			FString::Printf(TEXT("Failed to read CSV: %s"), *LatestCSV));

		FString Out;
		TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
		FJsonSerializer::Serialize(Result.ToSharedRef(), W);
		return Out;
	}

	// CSV header (from UE 5.7 MeshDrawCommandStats.cpp):
	// Pass, VisiblePrimitiveCount, VisibleVertices, VisibleInstances,
	// Category, ResourceName, LODIndex, SegmentIndex, MaterialName,
	// PrimitiveCount, VertexCount, TotalInstanceCount, TotalPrimitiveCount, TotalVertexCount
	TArray<FString> Lines;
	CSVContent.ParseIntoArrayLines(Lines);

	if (Lines.Num() < 2)
	{
		Result->SetBoolField(TEXT("success"), true);
		Result->SetBoolField(TEXT("stats_pending"), true);
		Result->SetStringField(TEXT("message"),
			TEXT("CSV is empty — stats not yet collected. Call again after one rendered frame."));

		FString Out;
		TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
		FJsonSerializer::Serialize(Result.ToSharedRef(), W);
		return Out;
	}

	// Parse header to find column indices dynamically
	TArray<FString> Headers;
	Lines[0].ParseIntoArray(Headers, TEXT(","), false);

	auto FindCol = [&Headers](const FString& Name) -> int32 {
		return Headers.IndexOfByPredicate([&Name](const FString& S) {
			return S.TrimStartAndEnd().Equals(Name, ESearchCase::IgnoreCase);
		});
	};

	int32 ColPass = FindCol(TEXT("Pass"));
	int32 ColVisPrim = FindCol(TEXT("VisiblePrimitiveCount"));
	int32 ColVisVert = FindCol(TEXT("VisibleVertices"));
	int32 ColVisInst = FindCol(TEXT("VisibleInstances"));
	int32 ColCategory = FindCol(TEXT("Category"));
	int32 ColResource = FindCol(TEXT("ResourceName"));
	int32 ColLOD = FindCol(TEXT("LODIndex"));
	int32 ColSegment = FindCol(TEXT("SegmentIndex"));
	int32 ColMaterial = FindCol(TEXT("MaterialName"));
	int32 ColTotalInst = FindCol(TEXT("TotalInstanceCount"));
	int32 ColTotalPrim = FindCol(TEXT("TotalPrimitiveCount"));
	int32 ColTotalVert = FindCol(TEXT("TotalVertexCount"));

	// Parse data rows
	struct FEntry
	{
		FString Pass, Resource, Category, Material;
		int64 VisiblePrimitives = 0, VisibleVertices = 0, VisibleInstances = 0;
		int32 LODIndex = 0, SegmentIndex = 0;
		int64 TotalInstances = 0, TotalPrimitives = 0, TotalVertices = 0;
	};
	TArray<FEntry> Entries;

	auto GetCol = [](const TArray<FString>& Cols, int32 Idx) -> FString {
		return (Idx >= 0 && Idx < Cols.Num()) ? Cols[Idx].TrimStartAndEnd() : TEXT("");
	};
	auto GetColInt = [&GetCol](const TArray<FString>& Cols, int32 Idx) -> int64 {
		FString Val = GetCol(Cols, Idx);
		return Val.IsEmpty() ? 0 : FCString::Atoi64(*Val);
	};

	for (int32 i = 1; i < Lines.Num(); ++i)
	{
		if (Lines[i].TrimStartAndEnd().IsEmpty()) continue;

		TArray<FString> Cols;
		Lines[i].ParseIntoArray(Cols, TEXT(","), false);
		if (Cols.Num() < 2) continue;

		FString Resource = GetCol(Cols, ColResource);

		// Apply mesh filter
		if (!MeshFilter.IsEmpty() &&
			!Resource.Contains(MeshFilter, ESearchCase::IgnoreCase))
		{
			continue;
		}

		FEntry E;
		E.Pass = GetCol(Cols, ColPass);
		E.Resource = Resource;
		E.Category = GetCol(Cols, ColCategory);
		E.Material = GetCol(Cols, ColMaterial);
		E.VisiblePrimitives = GetColInt(Cols, ColVisPrim);
		E.VisibleVertices = GetColInt(Cols, ColVisVert);
		E.VisibleInstances = GetColInt(Cols, ColVisInst);
		E.LODIndex = (int32)GetColInt(Cols, ColLOD);
		E.SegmentIndex = (int32)GetColInt(Cols, ColSegment);
		E.TotalInstances = GetColInt(Cols, ColTotalInst);
		E.TotalPrimitives = GetColInt(Cols, ColTotalPrim);
		E.TotalVertices = GetColInt(Cols, ColTotalVert);
		Entries.Add(MoveTemp(E));
	}

	// Sort by visible primitives descending
	Entries.Sort([](const FEntry& A, const FEntry& B) {
		return A.VisiblePrimitives > B.VisiblePrimitives;
	});

	// Build JSON (limit to top 100)
	TArray<TSharedPtr<FJsonValue>> JsonEntries;
	int32 Limit = FMath::Min(100, Entries.Num());
	for (int32 i = 0; i < Limit; ++i)
	{
		const FEntry& E = Entries[i];
		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetStringField(TEXT("pass"), E.Pass);
		Obj->SetStringField(TEXT("resource"), E.Resource);
		Obj->SetStringField(TEXT("category"), E.Category);
		if (!E.Material.IsEmpty())
			Obj->SetStringField(TEXT("material"), E.Material);
		Obj->SetNumberField(TEXT("visible_primitives"), (double)E.VisiblePrimitives);
		Obj->SetNumberField(TEXT("visible_vertices"), (double)E.VisibleVertices);
		Obj->SetNumberField(TEXT("visible_instances"), (double)E.VisibleInstances);
		Obj->SetNumberField(TEXT("lod_index"), E.LODIndex);
		Obj->SetNumberField(TEXT("total_instances"), (double)E.TotalInstances);
		Obj->SetNumberField(TEXT("total_primitives"), (double)E.TotalPrimitives);
		Obj->SetNumberField(TEXT("total_vertices"), (double)E.TotalVertices);
		JsonEntries.Add(MakeShared<FJsonValueObject>(Obj));
	}

	// Totals
	int64 TotalVisPrim = 0, TotalVisVert = 0, TotalVisInst = 0;
	for (const FEntry& E : Entries)
	{
		TotalVisPrim += E.VisiblePrimitives;
		TotalVisVert += E.VisibleVertices;
		TotalVisInst += E.VisibleInstances;
	}

	Result->SetBoolField(TEXT("success"), true);
	Result->SetArrayField(TEXT("entries"), JsonEntries);
	Result->SetNumberField(TEXT("total_entries"), Entries.Num());
	Result->SetNumberField(TEXT("shown_entries"), JsonEntries.Num());
	Result->SetNumberField(TEXT("total_visible_primitives"), (double)TotalVisPrim);
	Result->SetNumberField(TEXT("total_visible_vertices"), (double)TotalVisVert);
	Result->SetNumberField(TEXT("total_visible_instances"), (double)TotalVisInst);
	Result->SetStringField(TEXT("csv_file"), LatestCSV);

	if (!MeshFilter.IsEmpty())
		Result->SetStringField(TEXT("mesh_filter"), MeshFilter);

	Result->SetStringField(TEXT("message"), FString::Printf(
		TEXT("%d entries (%d shown), %lld visible primitives, %lld visible vertices, %lld visible instances"),
		Entries.Num(), JsonEntries.Num(), TotalVisPrim, TotalVisVert, TotalVisInst));

	// Clean up old CSV files (keep only the latest)
	for (int32 i = 0; i < FoundFiles.Num() - 1; ++i)
	{
		IFileManager::Get().Delete(*FPaths::Combine(ProfilingDir, FoundFiles[i]));
	}

	FString Out;
	TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(Result.ToSharedRef(), W);
	return Out;
}
