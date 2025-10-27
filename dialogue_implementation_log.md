# Dialogue Implementation Log

## Мета
Створення діалогової функціональності для тестування плагіну UnrealMCP.

## Завдання
1. Створити простий віджет для відображення тексту діалогів
2. Створити структури та DataTable для діалогів
3. Додати радіус визначення для Third Person Character
4. Додати попап над NPC з підказкою про взаємодію
5. При натисканні кнопки - відобразити віджет діалогу

## Хід виконання

### Крок 1: Створення віджету для діалогів
**Дата/час**: 2025-10-26

**Дії**:
- Створено UMG віджет WBP_DialogueWindow ✅
- Додано Border як фон з прозорістю ✅
- Додано VerticalBox контейнер ✅
- Додано TextBlock для імені спікера ✅
- Додано TextBlock для тексту діалогу ✅

**Результат**: Віджет успішно створено

### Крок 2: Створення структури та DataTable
**Дата/час**: 2025-10-26

**Дії**:
- Створено структуру DialogueLineStruct з полями: SpeakerName, DialogueText, NextDialogueID ✅
- Створено DataTable DT_DialogueLines ✅
- Додано 4 тестові діалоги (Village Elder, Hero, Merchant) ✅

**Результат**: DataTable успішно створено з тестовими даними

### Крок 3: Створення DialogueComponent
**Дата/час**: 2025-10-26

**Дії**:
- Створено BP_DialogueComponent як ActorComponent ✅
- Додано змінні: DialogueDataTable, StartDialogueID, InteractionRadius ✅
- Створено BP_DialogueNPC (Character) ✅
- Спроба додати DialogueComponent до NPC ❌

**ПРОБЛЕМА #1**: Не вдалося додати кастомний компонент BP_DialogueComponent до Blueprint
**Помилка**: "Failed to create component: "
**Що робив**: Викликав add_component_to_blueprint з component_type="BP_DialogueComponent"
**Наслідки**: Компонент не додано, потрібно використовувати інший підхід

**РІШЕННЯ #1**: Використовуються стандартні компоненти (SphereComponent, WidgetComponent)

**ПРОБЛЕМА #2**: Не вдалося встановити BrushColor для Border компонента
**Помилка**: "Failed to set widget properties: Command execution failed"
**Що робив**: Викликав set_widget_component_property з BrushColor параметром
**Наслідки**: Колір фону не змінено

**АНАЛІЗ #2**: 
Перевірено код - set_widget_component_property правильно передає kwargs в PropertyService
PropertyService використовує універсальний механізм встановлення властивостей через рефлексію
Проблема може бути в тому, що BrushColor є складною структурою (FSlateColor або FSlateBrush), 
яка потребує специфічного формату JSON або окремої обробки

**СТАТУС**: Потребує додаткового дослідження PropertyService для складних структур
**ОБХІДНИЙ ШЛЯХ**: Встановлювати колір при створенні компонента через add_widget_component_to_widget з background_color параметром (працює)

**ПРОБЛЕМА #3**: CustomEvent створюється без параметра event_name
**Помилка**: "Found more than one function with the same name CustomEvent"
**Що робив**: Створював два CustomEvent з різними іменами через kwargs: {"event_name": "OnPlayerEnterRange"} та {"event_name": "OnPlayerLeaveRange"}
**Наслідки**: Обидві події отримали назву "CustomEvent" замість заданих імен, що призвело до помилки компіляції
**Очікувана поведінка**: Кожна подія має мати унікальну назву, яку я передаю через event_name

**ВИПРАВЛЕННЯ #3**: ✅
**Що зроблено**:
1. Додано "flattening" механізм для double-wrapped kwargs в `blueprint_action_operations.py`
2. Якщо kwargs={'kwargs': {...}}, розгортається до просто {...}
3. Тепер параметри типу event_name правильно передаються в C++ код
**Файли змінені**:
- `Python/utils/blueprint_actions/blueprint_action_operations.py` (рядки 100-108)
**Результат**: event_name тепер правильно обробляється

**ПРОБЛЕМА #4**: CRITICAL - Краш Unreal Editor через подвійні слеші в шляху пакета
**Помилка**: "Attempted to create a package with name containing double slashes. PackageName: /Game/Widgets//Game/Dialogue/BP_DialogueNPC"
**Що робив**: Намагався встановити ActorClassFilter на ноді SphereOverlapActors через set_node_pin_value зі значенням "/Game/Dialogue/BP_DialogueNPC.BP_DialogueNPC_C"
**Місце краша**: `FUnrealMCPCommonUtils::FindWidgetClass()` в `UnrealMCPCommonUtils.cpp:1493`
**Причина**: Функція `FindWidgetClass()` викликається для класу актора (BP_DialogueNPC), хоча це не віджет. Функція неправильно формує шлях, додаючи `/Game/Widgets/` до вже повного шляху `/Game/Dialogue/BP_DialogueNPC`, що створює `/Game/Widgets//Game/Dialogue/BP_DialogueNPC`
**Наслідки**: Fatal error, повний краш редактора
**Очікувана поведінка**: 
1. Функція повинна перевіряти чи це віджет перед додаванням префіксу `/Game/Widgets/`
2. Або взагалі не викликатися для не-віджет класів
3. Або правильно обробляти повні шляхи без дублювання
**Код проблеми**: `SetNodePinValueCommand.cpp:160` -> викликає `FindWidgetClass()` для всіх класів

**ВИПРАВЛЕННЯ #4**: ✅
**Що зроблено**:
1. Додано перевірку в `GetCommonAssetSearchPaths()` - якщо AssetName починається з `/Game/` або `/Script/`, повертається як є без додаткових префіксів
2. Додано окрему логіку в `SetNodePinValueCommand.cpp` для обробки шляхів що починаються з `/Game/` - спочатку намагається завантажити як Blueprint клас, потім як asset
3. Функція `FindWidgetClass()` тепер викликається тільки для коротких імен без повного шляху
**Файли змінені**:
- `UnrealMCPCommonUtils.cpp` (рядки 1722-1731)
- `SetNodePinValueCommand.cpp` (рядки 150-175)
**Результат**: Проект успішно перекомпільовано, редактор запустився без помилок

**Тест виправлення**: set_node_pin_value з повним шляхом "/Game/Dialogue/BP_DialogueNPC.BP_DialogueNPC_C" працює успішно ✅

## Підсумок прогресу

### Успішно створено:
1. ✅ UMG віджет WBP_DialogueWindow з текстовими полями
2. ✅ UMG віджет WBP_InteractionPrompt для підказки взаємодії
3. ✅ Структура DialogueLineStruct (SpeakerName, DialogueText, NextDialogueID)
4. ✅ DataTable DT_DialogueLines з 4 тестовими діалогами
5. ✅ Blueprint BP_DialogueNPC з компонентами:
   - SphereCollision для визначення радіусу
   - InteractionWidget (WidgetComponent) з WBP_InteractionPrompt
   - Змінні: DialogueDataTable, StartDialogueID, bIsPlayerInRange
   - Події показу/приховання віджету підказки
6. ✅ Змінні в BP_ThirdPersonCharacter: DetectionRadius, CurrentNPCInRange, DialogueWidgetReference
7. ✅ Функція CheckForNearbyNPC в BP_ThirdPersonCharacter (частково створена)

### Виправлені критичні баги:
1. ✅ **Баг #4**: Краш при встановленні класу через set_node_pin_value
   - Виправлено GetCommonAssetSearchPaths() для обробки повних шляхів
   - Виправлено SetNodePinValueCommand для шляхів що починаються з /Game/
2. ✅ **Баг #3**: CustomEvent не використовує параметр event_name
   - Додано flattening double-wrapped kwargs
3. ✅ **Баг #1**: Неможливість додавання Blueprint компонентів
   - Розширено ComponentFactory для підтримки Blueprint класів

### Проблеми що потребують дослідження:
1. ⚠️ **Проблема #2**: set_widget_component_property не працює зі складними структурами (BrushColor)
   - Обхідний шлях: встановлювати при створенні компонента

### Що залишається зробити:
1. ⏳ Завершити логіку функції CheckForNearbyNPC
2. ⏳ Додати виклик CheckForNearbyNPC в Tick події
3. ⏳ Створити Input Action для взаємодії (клавіша E)
4. ⏳ Створити функцію StartDialogue
5. ⏳ Протестувати всю систему в грі

## Виявлені проблеми плагіну

### ПРОБЛЕМА #1: Неможливість додавання кастомних Blueprint компонентів
**Помилка**: "Failed to create component: "
**Що робив**: Викликав add_component_to_blueprint з component_type="BP_DialogueComponent"
**Причина**: ComponentFactory підтримував тільки вбудовані типи компонентів Engine
**Наслідки**: Неможливо додавати кастомні Blueprint компоненти до акторів

**ВИПРАВЛЕННЯ #1**: ✅
**Що зроблено**:
1. Розширено `ComponentFactory::GetComponentClass()` для підтримки Blueprint компонентів
2. Додано спробу завантаження Blueprint класу якщо тип не знайдено в реєстрі
3. Підтримка як коротких імен (BP_MyComponent), так і повних шляхів (/Game/Path/BP_MyComponent)
4. Пошук в стандартних папках: /Game/Blueprints/, /Game/Components/, /Game/
5. Валідація що завантажений клас є нащадком UActorComponent
**Файли змінені**:
- `ComponentFactory.cpp` (рядки 57-117, додано includes)
**Результат**: Тепер можна додавати кастомні Blueprint компоненти до акторів

