#include "NekomataGameUserSettings.h"

UNekomataGameUserSettings* UNekomataGameUserSettings::GetNekomataSettings()
{
	return Cast<UNekomataGameUserSettings>(UGameUserSettings::GetGameUserSettings());
}
