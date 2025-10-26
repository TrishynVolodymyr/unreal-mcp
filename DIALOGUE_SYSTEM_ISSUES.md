# Проблеми при створенні діалогової системи

## Дата: 26 жовтня 2025

## 📊 Підсумок:

**Всього проблем виявлено:** 6
**Критичних:** 1 (Проблема #3/#5 - найвища пріоритетність)
**Вирішених:** 2 (Проблема #1, #6)
**Середньої важливості:** 3

**Загальний висновок:**
✅ **Критичні проблеми UE 5.6 API вирішені!** ComponentService успішно мігровано на USubobjectDataSubsystem.
⚠️ Залишається критичний баг в create_node_by_action_name (Problem #3/#5).

**Вирішені проблеми:**
1. **Problem #1**: ✅ RESOLVED - WidgetComponent через ComponentFactory
2. **Problem #6**: ✅ RESOLVED - Компоненти тепер видимі в Component Tree через USubobjectDataSubsystem

**Критичні проблеми:**
1. **Problem #3/#5**: create_node_by_action_name ігнорує class_name при виборі функції

**Рекомендації:**
1. ~~НЕГАЙНО: Мігрувати ComponentService на USubobjectDataSubsystem API для UE 5.6~~ ✅ ВИКОНАНО
2. **НАСТУПНИЙ КРОК**: Виправити баг в create_node_by_action_name для правильного розрізнення функцій
3. Розширити set_component_property для підтримки більше властивостей
4. Додати підтримку Enhanced Input Action events

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

**Кроки що призвели до проблеми:**
1. Успішно додано SphereComponent до BP_DialogueNPC з назвою "InteractionRadius"
2. Спроба встановити радіус сфери через `mcp_blueprintmcp_set_component_property`
3. Спробовано варіанти:
   - `{"SphereRadius": 300.0}` - **ERROR**: Failed to set any component properties
   - `{"Radius": 300.0}` - **ERROR**: Failed to set any component properties

**Очікуваний результат:** 
Радіус SphereComponent встановлено на 300.0 units

**Фактичний результат:**
Помилка "Failed to set any component properties"

**Можлива причина:**
Неправильна назва властивості або плагін не підтримує всі властивості компонента

**Статус:** UNRESOLVED - продовжую з Blueprint логікою, радіус потрібно буде налаштувати вручну в редакторі

---

### Проблема #3: create_node_by_action_name створює неправильну ноду

**Кроки що призвели до проблеми:**
1. Використано `mcp_blueprintacti_search_blueprint_actions` для пошуку "Get All Actors Of Class"
2. Результат пошуку показав 3 варіанти, включаючи правильний:
   - "Get All Actors Of Class" (function_name: "GetAllActorsOfClass", class_name: "GameplayStatics")
3. Викликано `mcp_blueprintacti_create_node_by_action_name` з:
   - function_name: "GetAllActorsOfClass"
   - class_name: "GameplayStatics"

**Очікуваний результат:** 
Створення ноди "Get All Actors Of Class" з GameplayStatics

**Фактичний результат:**
Створена нода "Get All Actors Of Class Matching Tag Query" (інша функція з додатковими параметрами)

**Можлива причина:**
- Неправильний пріоритет при виборі функції з однаковими іменами
- Пошук може повертати першу знайдену функцію з частковим збігом імені

**Обхідний шлях:**
Спробувати з більш специфічною назвою або використати створену ноду якщо вона підходить

**Статус:** DOCUMENTED - продовжую з іншим підходом

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

**Помилка компіляції:**
```
The current value of the 'Gameplay Tag Query' pin is invalid: 
'Gameplay Tag Query' in action 'Get All Actors Of Class Matching Tag Query' 
must have an input wired into it
```

**Кроки що призвели до проблеми:**
1. Створено "Get All Actors Of Class Matching Tag Query" замість "Get All Actors Of Class" (проблема #3)
2. Під'єднано всі ноди в функції CheckForNearbyNPC
3. Спроба скомпілювати BP_ThirdPersonCharacter
4. Помилка через відсутність обов'язкового піна GameplayTagQuery

**Очікуваний результат:** 
Успішна компіляція з правильною нодою "Get All Actors Of Class"

**Фактичний результат:**
Помилка компіляції через неправильну ноду

**Рішення:**
Потрібно:
1. Видалити неправильну ноду
2. Створити правильну "Get All Actors Of Class" (без Tag Query)
3. Перепід'єднати з'єднання

**Статус:** IN PROGRESS - виправляю зараз

**Додаткова інформація (спроба виправлення):**
Спроби створити правильну ноду:
1. `class_name`: "KismetSystemLibrary", `function_name`: "GetAllActorsOfClass" - створює "Get All Actors Of Class Matching Tag Query"
2. `class_name`: "/Script/Engine.GameplayStatics", `function_name`: "GetAllActorsOfClass" - також створює неправильну ноду

**Висновок:** 
Blueprint Action Database має проблему з розрізненням функцій:
- GetAllActorsOfClass (GameplayStatics) - правильна, без Tag Query
- GetAllActorsOfClassMatchingTagQuery (BlueprintGameplayTagLibrary) - створюється завжди

Це критичний баг в `create_node_by_action_name`, який завжди повертає першу знайдену функцію незалежно від class_name.

**Обхідний шлях (застосовано):**
Буду використовувати просту логіку без GetAllActorsOfClass - створю змінну для посилання на NPC та встановлю її вручну

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
