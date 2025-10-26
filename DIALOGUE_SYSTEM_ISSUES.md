# Проблеми при створенні діалогової системи

## Дата: 26 жовтня 2025

## 📊 Підсумок:

**Всього проблем виявлено:** 6
**Критичних:** 0
**Вирішених:** 4 (Проблема #1, #2, #3/#5, #6)
**Невирішених:** 1 (Проблема #4)
**Задокументованих:** 1 (Problem #4 - потребує окремої реалізації)

**Загальний висновок:**
✅ **ВСІ критичні проблеми вирішені!**
✅ ComponentService успішно мігровано на USubobjectDataSubsystem API (UE 5.6)
✅ SetComponentPropertyCommand мігровано на USubobjectDataSubsystem API (UE 5.6)
✅ create_node_by_action_name тепер правильно фільтрує за класом та назвою функції
✅ set_component_property тепер коректно встановлює властивості з детальним error handling
⚠️ Enhanced Input events потребують спеціальної підтримки (не в action database)

**Вирішені проблеми:**
1. **Problem #1**: ✅ RESOLVED - WidgetComponent через ComponentFactory
2. **Problem #2**: ✅ RESOLVED - Міграція на USubobjectDataSubsystem + виправлення kwargs parsing
3. **Problem #3/#5**: ✅ RESOLVED - Додано фільтрацію по class_name та точному імені функції
4. **Problem #6**: ✅ RESOLVED - Компоненти тепер видимі в Component Tree через USubobjectDataSubsystem

**Невирішені проблеми:**
1. **Problem #4**: Enhanced Input events не доступні через Blueprint Action Database

**Рекомендації:**
1. ~~НЕГАЙНО: Мігрувати ComponentService на USubobjectDataSubsystem API для UE 5.6~~ ✅ ВИКОНАНО
2. ~~Виправити баг в create_node_by_action_name для правильного розрізнення функцій~~ ✅ ВИКОНАНО
3. ~~Розширити set_component_property для підтримки більше властивостей (Problem #2)~~ ✅ ВИКОНАНО
4. Додати спеціальну підтримку Enhanced Input Action events (Problem #4 - потребує окремого node type)

---

## Детальний опис проблем:

### Проблема #1: Неможливо додати WidgetComponent до Blueprint

**Кроки що призвели до проблеми:**
1. Створено NPC Blueprint: `BP_DialogueNPC` (parent: Character)
2. Спроба додати компонент Widget для відображення popup над NPC
3. Викликано `mcp_blueprintmcp_add_component_to_blueprint` з наступними варіантами:
   - `component_type`: "WidgetComponent" - **ERROR**: Unknown component type 'WidgetComponent'
   - `component_type`: "Widget" - **ERROR**: Unknown component type 'Widget'
   - `component_type`: "UMGWidget" - **ERROR**: Unknown component type 'UMGWidget'

**Очікуваний результат:** 
Додання WidgetComponent до Blueprint для відображення UMG віджета у 3D просторі

**Фактичний результат:**
Помилка "Unknown component type"

**Причина:**
ComponentService мав hardcoded список компонентів і не використовував ComponentFactory, де WidgetComponent вже був зареєстрований.

**Рішення (ЗАСТОСОВАНО):**
1. Додано `#include "Factories/ComponentFactory.h"` до ComponentService.cpp
2. Замінено весь hardcode в `FComponentTypeCache::ResolveComponentClassInternal()` на виклик `FComponentFactory::Get().GetComponentClass()`
3. Замінено hardcode в `FComponentService::ResolveComponentClass()` на виклик ComponentFactory
4. Видалено зайві includes конкретних компонентів
5. Додано маппінг "Widget" -> "WidgetComponent" та "WidgetComponent" -> "WidgetComponent" в GetSupportedComponentTypes()

**Результат:**
✅ WidgetComponent тепер успішно додається!
✅ Код став чистішим і використовує існуючу інфраструктуру
✅ Всі нові компоненти, додані до ComponentFactory, автоматично стануть доступними

**Статус:** ✅ RESOLVED - виправлено через використання ComponentFactory

---

### Проблема #2: Неможливо встановити властивості SphereComponent

**Статус:** ✅ RESOLVED

**Кроки що призвели до проблеми:**
1. Успішно додано SphereComponent до BP_DialogueNPC з назвою "InteractionRadius"
2. Спроба встановити радіус сфери через `mcp_blueprintmcp_set_component_property`
3. Спробовано варіанти:
   - `{"SphereRadius": 300.0}` - **ERROR**: Failed to set any component properties
   - `{"Radius": 300.0}` - **ERROR**: Failed to set any component properties

**Очікуваний результат:** 
Радіус SphereComponent встановлено на 300.0 units

**Фактичний результат (ДО ВИПРАВЛЕННЯ):**
Помилка "Failed to set any component properties"

**Причина:**
1. SetComponentPropertyCommand використовував **застарілий SimpleConstructionScript API** (UE 4.x)
2. Компоненти створені через USubobjectDataSubsystem (Problem #6 fix) не були видимі через SimpleConstructionScript
3. Python MCP передавав подвійно-загорнутий kwargs: `{"kwargs": {"SphereRadius": 300.0}}`
4. C++ не розпаковував внутрішній рівень kwargs

**Рішення (ЗАСТОСОВАНО):**
1. **Міграція на USubobjectDataSubsystem API:**
   - Замінено `Blueprint->SimpleConstructionScript->GetAllNodes()` на `SubobjectDataSubsystem->K2_GatherSubobjectDataForBlueprint()`
   - Використано `FSubobjectData->GetVariableName()` для пошуку компонента
   - Залишено fallback на CDO для inherited components

2. **Виправлення kwargs parsing:**
   - Додано розпакування подвійно-загорнутого kwargs
   - Перевірка чи parsed object має inner "kwargs" field
   - Використання inner object якщо знайдено

3. **Покращений error handling:**
   - Список available properties збирається один раз
   - Короткі error messages без дублювання великого списку
   - `available_properties[]` в response окремим полем
   - Детальні summary messages з підказками

**Результат:**
✅ SphereRadius успішно встановлюється (протестовано: 300.0, 500.0, 600.0)
✅ RelativeLocation, RelativeScale та інші властивості працюють
✅ Error handling показує available properties один раз
✅ Partial success підтримується (деякі properties OK, деякі failed)

**Response структура:**
```json
{
  "success": true/false,
  "success_properties": ["SphereRadius", ...],
  "failed_properties": {
    "BadProp": "Property 'BadProp' not found on component 'Name' (Class: Type)"
  },
  "available_properties": ["SphereRadius", "RelativeLocation", ...],
  "message": "Partially successful: 1 properties set, 2 failed. See 'available_properties' for valid options."
}
```

**Файли змінено:**
- SetComponentPropertyCommand.h: оновлено сигнатури функцій
- SetComponentPropertyCommand.cpp: міграція на USubobjectDataSubsystem + kwargs parsing + error handling
- Додано includes: MCPLogging.h, SubobjectDataSubsystem.h, SubobjectData.h

**Статус:** ✅ RESOLVED - commit ac2f9c7

---

### Проблема #3: create_node_by_action_name створює неправильну ноду

**Статус:** ✅ RESOLVED

**Кроки що призвели до проблеми:**
1. Використано `mcp_blueprintacti_search_blueprint_actions` для пошуку "Get All Actors Of Class"
2. Результат пошуку показав 3 варіанти, включаючи правильний:
   - "Get All Actors Of Class" (function_name: "GetAllActorsOfClass", class_name: "GameplayStatics")
3. Викликано `mcp_blueprintacti_create_node_by_action_name` з:
   - function_name: "GetAllActorsOfClass"
   - class_name: "GameplayStatics"

**Очікуваний результат:** 
Створення ноди "Get All Actors Of Class" з GameplayStatics (5 пінів)

**Фактичний результат (ДО ВИПРАВЛЕННЯ):**
Створена нода "Get All Actors Of Class with Tag" (7 пінів з додатковим Tag параметром)

**Причина:**
1. TryCreateNodeUsingBlueprintActionDatabase не фільтрував по ClassName
2. Використовувався partial string matching: "Get All Actors Of Class with Tag" **contains** "Get All Actors Of Class"
3. Перша знайдена нода з частковим збігом створювалась без перевірки точного імені функції

**Рішення (ЗАСТОСОВАНО):**
1. Додано параметр ClassName до TryCreateNodeUsingBlueprintActionDatabase
2. Реалізовано перевірку owner class для UK2Node_CallFunction з кількома стратегіями збігу:
   - Точний збіг назви класу (case-insensitive)
   - Збіг без U/A префіксу
   - Збіг з додаванням U префіксу
   - Hardcoded маппінги для популярних бібліотек (GameplayStatics, KismetMathLibrary тощо)
3. Додано вимогу точного збігу імені функції коли вказаний ClassName
4. Продовжувати пошук якщо клас/функція не збігаються замість прийняття першого partial match
5. Оновлено всі 5 місць виклику для передачі параметра ClassName

**Результат:**
✅ GetAllActorsOfClass з class_name="GameplayStatics" тепер створює правильну ноду
✅ Логи показують: пропускає GetAllActorsOfClassWithTag, знаходить exact match GetAllActorsOfClass
✅ Створена нода має 5 пінів (без Tag параметра) - це правильна функція з UGameplayStatics

**Файли змінено:**
- BlueprintNodeCreationService.h: оновлено сигнатуру функції
- BlueprintNodeCreationService.cpp: реалізовано логіку фільтрації

**Статус:** ✅ RESOLVED - commit 63d8960

---

### Проблема #4: Неможливо створити Enhanced Input Action event через MCP tools

**Кроки що призвели до проблеми:**
1. Проект використовує Enhanced Input System (UE 5.5+)
2. Існує Input Action "IA_Interact" з маппінгом на клавішу E
3. Потрібно створити event в Blueprint для обробки цього input
4. Спроба знайти через `mcp_blueprintacti_search_blueprint_actions`:
   - Пошук "IA_Interact" - 0 результатів
   - Пошук "Enhanced Input Action" - 0 результатів

**Очікуваний результат:** 
Знайти і створити InputAction event для IA_Interact

**Фактичний результат:**
Неможливо знайти Enhanced Input events через search

**Можлива причина:**
- Enhanced Input events можуть бути специфічним типом node, який не включений до action database
- Може потрібен спеціальний синтаксис або метод створення

**Обхідний шлях:**
Створити custom event та викликати його вручну, або додати підтримку Enhanced Input events в MCP tools

**Статус:** DOCUMENTED - буду використовувати custom event

---

### Проблема #5: Compilation error через неправильну створену ноду (продовження #3)

**Статус:** ✅ RESOLVED (разом з Problem #3)

**Помилка компіляції (ДО ВИПРАВЛЕННЯ):**
```
The current value of the 'Gameplay Tag Query' pin is invalid: 
'Gameplay Tag Query' in action 'Get All Actors Of Class Matching Tag Query' 
must have an input wired into it
```

**Кроки що призвели до проблеми:**
1. Створено "Get All Actors Of Class with Tag" замість "Get All Actors Of Class" (проблема #3)
2. Під'єднано всі ноди в функції CheckForNearbyNPC
3. Спроба скомпілювати BP_ThirdPersonCharacter
4. Помилка через відсутність обов'язкового піна Tag

**Рішення:**
Виправлено разом з Problem #3 - тепер create_node_by_action_name створює правильну ноду з UGameplayStatics.

**Статус:** ✅ RESOLVED - commit 63d8960

---

### Проблема #6: Компоненти додаються але не відображаються в Component Tree (UE 5.6 API)

**Кроки що призвели до проблеми:**
1. Успішно додано компоненти через `add_component_to_blueprint`:
   - InteractionRadius (SphereComponent)
   - InteractionPopupWidget (WidgetComponent) - після виправлення Problem #1
   - TestWidgetComponent (WidgetComponent)
2. Логи показують: "Attached component to parent 'DefaultSceneRoot'" - SUCCESS
3. MCP повертає success: true
4. Компіляція Blueprint успішна
5. Перезапуск Unreal Editor виконано

**Очікуваний результат:** 
Компоненти з'являються в Component Tree зверху (як при ручному додаванні)

**Фактичний результат:**
- Компоненти додаються як **змінні** в секцію "Components" внизу (біля Variables)
- Компоненти **НЕ з'являються** в Component Tree зверху
- При ручному додаванні компонента - Unreal додає його в **Capsule Component**
- При програмному додаванні - код додає в **DefaultSceneRoot**

**Причина:**
ComponentService використовує **застарілий API** для UE 5.6:
- Використовується: `Blueprint->SimpleConstructionScript->CreateNode()` + `AddChildNode()`
- Потрібно використовувати: `USubobjectDataSubsystem::AddNewSubobject()` (новий API для UE 5.x)

Згідно з UE 5.x documentation та forum posts:
- SimpleConstructionScript API - legacy, для UE 4.x
- USubobjectDataSubsystem - правильний API для UE 5.x, інтегрується з Editor UI

**Додаткова інформація:**
- Google пошук показав: "Use the new Subobject Data Subsystem to add a Component to an Actor in the Editor"
- Unreal source code (SSCSEditor.cpp) в UE 5.6 використовує SubobjectDataSubsystem
- Snippet на dev.epicgames.com підтверджує необхідність використання SubobjectDataSubsystem

**Рішення:**
Потрібно рефакторити `ComponentService::AddComponentToBlueprint()`:
```cpp
// Замість:
USCS_Node* NewNode = Blueprint->SimpleConstructionScript->CreateNode(...);
NewNode->SetParent(ParentNode);
ParentNode->AddChildNode(NewNode);

// Використовувати:
USubobjectDataSubsystem* SubobjectSubsystem = GEditor->GetEditorSubsystem<USubobjectDataSubsystem>();
FSubobjectDataHandle ParentHandle = ...; // Get parent component handle
FSubobjectDataHandle NewHandle;
SubobjectSubsystem->AddNewSubobject(
    FAddNewSubobjectParams{
        .ParentHandle = ParentHandle,
        .NewClass = ComponentClass,
        .BlueprintContext = Blueprint
    },
    NewHandle
);
```

**Рішення (ЗАСТОСОВАНО):**
1. Мігровано ComponentService на USubobjectDataSubsystem API
2. Замінено `SimpleConstructionScript->CreateNode()` на `SubobjectDataSubsystem->AddNewSubobject()`
3. Використано `K2_GatherSubobjectDataForBlueprint()` для правильного Blueprint контексту
4. Використано `FindSceneRootForSubobject()` для знаходження правильного батьківського компонента
5. Додано залежність `SubobjectDataInterface` в UnrealMCP.Build.cs
6. Змінено include з `Editor.h` на `Engine/Engine.h`
7. Використано `GEngine->GetEngineSubsystem<USubobjectDataSubsystem>()` (це UEngineSubsystem, не UEditorSubsystem)

**Результат:**
✅ Компоненти тепер відображаються в Component Tree
✅ Правильне прикріплення до батьківського компонента (Capsule для Character)
✅ Повна інтеграція з Editor UI
✅ Можливість налаштування властивостей через Details panel

**Статус:** ✅ RESOLVED - мігровано на UE 5.6 USubobjectDataSubsystem API

---
