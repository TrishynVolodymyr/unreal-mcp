#include "Commands/Editor/ExecuteConsoleCommandCommand.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Engine.h"
#include "Editor.h"
#include "Misc/StringOutputDevice.h"

FString FExecuteConsoleCommandCommand::Execute(const FString& Parameters)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		TSharedPtr<FJsonObject> Err = MakeShared<FJsonObject>();
		Err->SetBoolField(TEXT("success"), false);
		Err->SetStringField(TEXT("error"), TEXT("Invalid JSON parameters"));
		FString Out;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
		FJsonSerializer::Serialize(Err.ToSharedRef(), Writer);
		return Out;
	}

	FString Command;
	if (!JsonObject->TryGetStringField(TEXT("command"), Command))
	{
		TSharedPtr<FJsonObject> Err = MakeShared<FJsonObject>();
		Err->SetBoolField(TEXT("success"), false);
		Err->SetStringField(TEXT("error"), TEXT("Missing 'command' parameter"));
		FString Out;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
		FJsonSerializer::Serialize(Err.ToSharedRef(), Writer);
		return Out;
	}

	// Get the editor world for command context
	UWorld* World = nullptr;
	if (GEditor)
	{
		World = GEditor->GetEditorWorldContext().World();
	}

	// Execute with output capture
	FStringOutputDevice OutputDevice;
	if (GEngine && World)
	{
		GEngine->Exec(World, *Command, OutputDevice);
	}
	else if (GEngine)
	{
		GEngine->Exec(nullptr, *Command, OutputDevice);
	}

	FString CapturedOutput = OutputDevice;

	UE_LOG(LogTemp, Log, TEXT("Console command executed: '%s' → Output: '%s'"), *Command, *CapturedOutput);

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("command"), Command);
	Result->SetStringField(TEXT("output"), CapturedOutput);
	Result->SetStringField(TEXT("message"), FString::Printf(TEXT("Executed: %s"), *Command));

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(Result.ToSharedRef(), Writer);
	return OutputString;
}

FString FExecuteConsoleCommandCommand::GetCommandName() const
{
	return TEXT("execute_console_command");
}

bool FExecuteConsoleCommandCommand::ValidateParams(const FString& Parameters) const
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Parameters);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
		return false;
	FString Command;
	return JsonObject->TryGetStringField(TEXT("command"), Command) && !Command.IsEmpty();
}
