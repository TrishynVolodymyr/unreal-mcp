# План рефакторингу великих C++ файлів

## 📋 Загальна стратегія

**Мета**: Розділити файли понад 1000 рядків на логічні, функціональні модулі по ~500-700 рядків кожен.

**Принципи**:
- Один файл = одна відповідальність (Single Responsibility Principle)
- Зберегти всі публічні API без змін
- Мінімізувати міжфайлові залежності
- Покращити читабельність та підтримуваність

---

## 🔴 1. UnrealMCPCommonUtils.cpp (2119 рядків) → 6 файлів

### 📁 Нова структура: `Utils/` (підпапки)

#### **1.1. JsonUtils.cpp** (~300 рядків)
**Призначення**: Робота з JSON
```cpp
// Методи:
- CreateErrorResponse()
- CreateSuccessResponse()
- GetIntArrayFromJson()
- GetFloatArrayFromJson()
- GetStringArrayFromJson()
- JsonToStruct() / StructToJson()
```

#### **1.2. AssetSearchUtils.cpp** (~400 рядків)
**Призначення**: Пошук та локація асетів
```cpp
// Методи:
- FindAssetsByType()
- FindAssetsByName()
- FindWidgetBlueprints()
- FindBlueprints()
- FindDataTables()
- FindAssetByPath()
- FindAssetByName()
- GetCommonAssetSearchPaths()
- NormalizeAssetPath()
- IsValidAssetPath()
```

#### **1.3. ActorConversionUtils.cpp** (~200 рядків)
**Призначення**: Конвертація акторів в JSON
```cpp
// Методи:
- ActorToJson()
- ActorToJsonObject()
- ComponentToJson()
- TransformToJson()
```

#### **1.4. PropertyUtils.cpp** (~600 рядків)
**Призначення**: Робота з UProperty та рефлексією
```cpp
// Методи:
- SetObjectProperty()
- GetObjectProperty()
- SetPropertyFromJson()
- ParseVector()
- ParseLinearColor()
- ParseRotator()
- ConvertJsonToProperty()
```

#### **1.5. NodeGraphUtils.cpp** (~400 рядків)
**Призначення**: Blueprint Graph операції
```cpp
// Методи:
- ConnectGraphNodes()
- GetNodePinInfoRuntime()
- GetPinTypeInfo()
- GetPinCategoryDisplayName()
- GetAllGraphsFromBlueprint()
- FindPinByName()
- ValidatePinConnection()
```

#### **1.6. FunctionCallUtils.cpp** (~200 рядків)
**Призначення**: Виклик функцій через рефлексію
```cpp
// Методи:
- CallFunctionByName()
- PrepareParametersForFunction()
- ValidateFunctionParameters()
```

---

## 🟠 2. BlueprintService.cpp (1709 рядків) → 5 файлів

### 📁 Нова структура: `Services/Blueprint/`

#### **2.1. BlueprintCreationService.cpp** (~350 рядків)
**Призначення**: Створення Blueprint
```cpp
// Методи:
- CreateBlueprint()
- CreateBlueprintFromClass()
- CreateDirectoryStructure()
- NormalizeBlueprintPath()
- ValidateBlueprintParams()
```

#### **2.2. BlueprintCompilationService.cpp** (~400 рядків)
**Призначення**: Компіляція та валідація
```cpp
// Методи:
- CompileBlueprint()
- ValidateBlueprint()
- GetCompilationErrors()
- FixCompilationWarnings()
- RebuildGraphNodes()
```

#### **2.3. BlueprintVariableService.cpp** (~300 рядків)
**Призначення**: Управління змінними
```cpp
// Методи:
- AddVariableToBlueprint()
- RemoveVariable()
- SetVariableDefaultValue()
- SetVariableType()
- ResolveVariableType()
- ConvertStringToPinType()
```

#### **2.4. BlueprintComponentService.cpp** (~350 рядків)
**Призначення**: Управління компонентами
```cpp
// Методи:
- AddComponentToBlueprint()
- GetBlueprintComponents()
- SetStaticMeshProperties()
- SetPhysicsProperties()
- SetComponentTransform()
```

#### **2.5. BlueprintFunctionService.cpp** (~300 рядків)
**Призначення**: Користувацькі функції та інтерфейси
```cpp
// Методи:
- CreateCustomBlueprintFunction()
- AddInterfaceToBlueprint()
- AddFunctionParameter()
- SetFunctionReturnType()
- CallBlueprintFunction()
```

**Залишається**: `BlueprintCache` (~200 рядків) - залишається в головному файлі як допоміжний клас

---

## 🟡 3. BlueprintNodeService.cpp (1612 рядків) → 4 файли

### 📁 Нова структура: `Services/Blueprint/Node/`

#### **3.1. NodeConnectionService.cpp** (~400 рядків)
**Призначення**: З'єднання нодів
```cpp
// Методи:
- ConnectBlueprintNodes()
- ConnectPins()
- ConnectNodesWithAutoCast()
- ArePinTypesCompatible()
- ValidateConnection()
```

#### **3.2. NodeCreationService.cpp** (~500 рядків)
**Призначення**: Створення різних типів нодів
```cpp
// Методи:
- AddEventNode()
- AddFunctionCallNode()
- AddCustomEventNode()
- AddVariableNode()
- AddInputActionNode()
- GenerateNodeId()
```

#### **3.3. NodeCastService.cpp** (~500 рядків)
**Призначення**: Автоматичні касти між типами
```cpp
// Методи:
- CreateCastNode()
- CreateIntToStringCast()
- CreateFloatToStringCast()
- CreateBoolToStringCast()
- CreateStringToIntCast()
- CreateStringToFloatCast()
- CreateObjectCast()
```

#### **3.4. NodeQueryService.cpp** (~200 рядків)
**Призначення**: Пошук та інформація про ноди
```cpp
// Методи:
- FindBlueprintNodes()
- GetBlueprintGraphs()
- GetVariableInfo()
- GetNodeInfo()
- GetCleanTypePromotionTitle()
```

---

## 🟢 4. WidgetComponentService.cpp (1486 рядків) → 4 файли

### 📁 Нова структура: `Services/UMG/Widgets/`

#### **4.1. WidgetCreationService.cpp** (~200 рядків)
**Призначення**: Базова логіка створення
```cpp
// Методи:
- CreateWidgetComponent() // main dispatcher
- AddWidgetToTree()
- SaveWidgetBlueprint()
- GetJsonArray()
- GetKwargsToUse()
```

#### **4.2. BasicWidgetFactory.cpp** (~400 рядків)
**Призначення**: Базові UI компоненти
```cpp
// Методи:
- CreateTextBlock()
- CreateButton()
- CreateImage()
- CreateCheckBox()
- CreateSlider()
- CreateProgressBar()
- CreateEditableText()
- CreateEditableTextBox()
```

#### **4.3. LayoutWidgetFactory.cpp** (~400 рядків)
**Призначення**: Layout контейнери
```cpp
// Методи:
- CreateVerticalBox()
- CreateHorizontalBox()
- CreateOverlay()
- CreateGridPanel()
- CreateCanvasPanel()
- CreateSizeBox()
- CreateScrollBox()
- CreateWrapBox()
- CreateUniformGridPanel()
```

#### **4.4. AdvancedWidgetFactory.cpp** (~450 рядків)
**Призначення**: Складні та спеціалізовані віджети
```cpp
// Методи:
- CreateListView()
- CreateTileView()
- CreateTreeView()
- CreateComboBox()
- CreateMenuAnchor()
- CreateWidgetSwitcher()
- CreateExpandableArea()
- CreateRichTextBlock()
- CreateMultiLineEditableText()
- CreateSpinBox()
- CreateRadialSlider()
- CreateThrobber()
- CreateCircularThrobber()
- CreateSafeZone()
- CreateBackgroundBlur()
- CreateNativeWidgetHost()
- CreateNamedSlot()
- CreateScaleBox()
- CreateBorder()
- CreateSpacer()
```

---

## 🔵 5. BlueprintNodeCreationService.cpp (1168 рядків) → 3 файли

### 📁 Нова структура: `Services/Blueprint/NodeCreation/`

#### **5.1. ActionNodeCreationService.cpp** (~400 рядків)
**Призначення**: Створення нодів через Blueprint Actions
```cpp
// Методи:
- CreateNodeByActionName()
- FindActionInDatabase()
- SpawnNodeFromAction()
```

#### **5.2. SpecializedNodeCreationService.cpp** (~400 рядків)
**Призначення**: Спеціалізовані ноди (Custom Events, Casts, etc.)
```cpp
// Методи:
- CreateCustomEventNode()
- CreateCastNode()
- CreateEnhancedInputNode()
- CreateMacroNode()
```

#### **5.3. NodePropertyService.cpp** (~350 рядків)
**Призначення**: Налаштування властивостей нодів
```cpp
// Методи:
- SetNodePinValue()
- SetNodePosition()
- SetNodeMetadata()
- ConfigureNodeDefaults()
```

---

## 🟣 6. UnrealMCPBlueprintCommands.cpp (1175 рядків) - Packaged версія

**Рішення**: Це packaged версія, яка генерується автоматично. Рефакторимо тільки source версію в `Plugins/`.

---

## 📝 Порядок виконання рефакторингу

### Етап 1: Підготовка (1-2 години)
1. ✅ Створити нові директорії
2. ✅ Створити header файли з деклараціями
3. ✅ Додати нові файли в Build.cs

### Етап 2: Рефакторинг (по пріоритету)
1. **UnrealMCPCommonUtils** (найбільший, найважливіший)
2. **WidgetComponentService** (добре структурований)
3. **BlueprintService** (середній)
4. **BlueprintNodeService** (складний)
5. **BlueprintNodeCreationService** (найменший)

### Етап 3: Тестування після кожного файлу
1. ✅ Компіляція
2. ✅ Запуск редактора
3. ✅ Тест базових команд через MCP
4. ✅ Перевірка існуючого функціоналу

---

## 🎯 Очікувані результати

### До рефакторингу:
- 6 файлів > 1000 рядків
- Всього: ~9,269 рядків у великих файлах
- Важко читати, важко підтримувати
- AI погано працює з такими файлами

### Після рефакторингу:
- 0 файлів > 1000 рядків
- ~22 нових модульних файли по 200-700 рядків
- Чітка структура по відповідальностям
- Легко знаходити потрібний код
- AI працює ефективно
- Простіше додавати новий функціонал

---

## ⚠️ Застереження

1. **Не змінювати публічні API** - всі існуючі методи залишаються доступними
2. **Backward compatibility** - старий код продовжує працювати
3. **Інкрементальний підхід** - по одному файлу за раз
4. **Повне тестування** - після кожного рефакторингу
5. **Git commits** - окремий коміт для кожного рефакторингу

---

## 🚀 Готовність до старту

Чи маєш якісь питання або хочеш почати з конкретного файлу?

Мої рекомендації:
- **Почати з**: `WidgetComponentService.cpp` (найпростіший, добре структурований)
- **Або з**: `UnrealMCPCommonUtils.cpp` (найбільший ефект, але складніше)
