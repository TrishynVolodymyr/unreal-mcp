# Dialogue Functionality Implementation - Issues Log

## Дата: November 1, 2025

### Завдання:
Створити діалогову функціональність з наступними компонентами:
1. Віджет для відображення тексту діалогів
2. Структура та DataTable для діалогових даних
3. Радіус визначення для Third Person Character
4. Попап над NPC з підказкою про можливість діалогу
5. Відображення діалогового віджету при натисканні кнопки

---

## Процес імплементації:

### Крок 1: Створення UMG віджету для діалогів
**Час початку:** [Starting]
**Дії:**
- Створено віджет WBP_DialogueWindow ✓
- Додано компоненти: DialogueBackground (Border), SpeakerNameText, DialogueText ✓

### Крок 2: Структура та DataTable
**Дії:**
- Створено структуру DialogueData з полями: SpeakerName, DialogueText, ResponseOptions ✓
- Створено DataTable DT_DialogueData на основі DialogueData ✓
- Додано 3 приклади діалогів (Greeting_1, Quest_1, Farewell_1) ✓

### Крок 3: Попап-віджет
**Дії:**
- Створено віджет WBP_InteractionPrompt ✓
- Додано компоненти: PromptBackground, PromptText ✓

### Крок 4: NPC Blueprint
**Дії:**
- Створено BP_DialogueNPC (Character) ✓
- Додано компонент InteractionWidget (WidgetComponent) ✓
- Налаштовано WidgetClass для InteractionWidget на WBP_InteractionPrompt ✓
- Створено змінні: InteractionRadius (Float), DialogueRowName (Name) ✓

### Крок 5: DialogueComponent
**Дії:**
- Створено BP_DialogueComponent (ActorComponent) ✓
- Додано змінні: InteractionRadius, CurrentNearbyNPC, DialogueWidgetReference ✓
- Створено функцію CheckForNearbyNPCs ✓
- Створено логіку перевірки відстані в функції CheckForNearbyNPCs ✓

**ПРОБЛЕМА #1:** 
- **Дія:** Компіляція BP_DialogueComponent
- **Помилка:** "This blueprint (self) is not a Actor, therefore ' Target ' must have a connection."
- **Деталі:** У функції CheckForNearbyNPCs, нода GetOwner потребує підключення self піна
- **Контекст:** GetOwner викликається на ActorComponent і повертає Actor-власника. Система обрала НЕПРАВИЛЬНУ версію GetOwner (для Actor або CheatManager) замість версії для ActorComponent
- **Час:** [During CheckForNearbyNPCs function creation]
- **Root Cause:** 
  - При виклику `create_node_by_action_name` без `class_name` система обирає першу знайдену функцію з такою назвою
  - GetOwner існує в багатьох класах (Actor, CheatManager, ActorComponent, тощо)
  - Без явного вказання класу отримуємо випадкову версію
- **Вирішення:** 
  1. Використав mcp_nodemcp_replace_node для заміни старої GetOwner ноди
  2. Створив нову GetOwner ноду з ЯВНИМ класом ActorComponent через mcp_blueprintacti_create_node_by_action_name
  3. Видалив стару ноду через mcp_nodemcp_delete_node
  4. З'єднав нову GetOwner з усіма потрібними нодами (GetActorLocation, GetAllActorsOfClass, GetDistanceTo)
  5. Компіляція пройшла успішно ✓
- **УРОК ДЛЯ МАЙБУТНЬОГО:**
  - ❌ **НІКОЛИ не викликати** `create_node_by_action_name(function_name="GetOwner")` без class_name
  - ✅ **ЗАВЖДИ використовувати** `get_actions_for_class` перед створенням ноди
  - ✅ **ЗАВЖДИ вказувати** параметр `class_name` при виклику `create_node_by_action_name`
  - ✅ Для ActorComponent використовувати: `create_node_by_action_name(function_name="GetOwner", class_name="ActorComponent")`

### Крок 6: EventGraph для Tick
**Дії:**
- Додано виклик CheckForNearbyNPCs в EventTick ✓
- Компіляція BP_DialogueComponent успішна ✓

### Крок 7: Додавання компонента до ThirdPersonCharacter
**Дії:**
- Додано BP_DialogueComponent до BP_ThirdPersonCharacter ✓
- Створено EnhancedInputAction івент для IA_Interact ✓

**ПРОБЛЕМА #2 - КРИТИЧНА (✅ ВИПРАВЛЕНО І ПРОТЕСТОВАНО):** 
- **Дія:** Виклик mcp_blueprintacti_search_blueprint_actions для blueprint_name з повним шляхом
- **Помилка:** Fatal error - "Attempted to create a package with name containing double slashes. PackageName: /Game//Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"
- **Деталі:** 
  - Crash call stack: UnrealMCPBlueprintActionCommands::SearchBlueprintActions() [Line: 1522]
  - Викликав search_blueprint_actions з blueprint_name="/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"
  - Система додала додатковий "/Game/" префікс, що призвело до "/Game//Game/..."
- **Контекст:** При пошуку локальних змінних Blueprint (DialogueComponent) в BP_ThirdPersonCharacter
- **Час:** [After creating IA_Interact input event]
- **Root Cause:** Багато функцій у C++ плагіні некоректно обробляли шляхи з `/Game/` або `Game/` префіксами
- **Наслідки:** Повний краш Unreal Editor
- **ВИПРАВЛЕННЯ (всі зміни застосовані та скомпільовані):**
  1. ✅ **UnrealMCPBlueprintActionCommands.cpp:1522** - Додано логіку:
     - Перевіряє чи BlueprintName починається з `/Game/`
     - Якщо так - витягує ім'я ассета і формує правильний шлях
     - Якщо ні - додає `/Game/` префікс як раніше
  2. ✅ **UnrealMCPCommonUtils.cpp:218** - Додано перевірку:
     - Якщо SubPath починається з "Game" (ignoreCase) - додає тільки `/`
     - Інакше додає `/Game/`
  3. ✅ **ComponentFactory.cpp:107** - Додано очищення TypeName:
     - Видаляє `/Game/` та `Game/` префікси
     - Витягує чисте ім'я через FPaths::GetBaseFilename
  4. ✅ **AssetDiscoveryService.cpp:489** - Додано перевірку в BuildGamePath:
     - Перевіряє чи CleanPath починається з `Game/`
     - Якщо так - додає тільки `/` 
     - Інакше додає `/Game/`
- **Статус:** Компіляція успішна, всі warning не критичні, Editor запущено
- **ТЕСТУВАННЯ (✅ ПРОЙДЕНО):**
  1. ✅ Тест з повним шляхом: `blueprint_name="/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"` - працює, не крашить
  2. ✅ Знайдено компонент DialogueComponent в BP_ThirdPersonCharacter
  3. ✅ Знайдено BP_DialogueNPC через Cast
  4. ✅ Пошук працює як з повними шляхами так і з короткими іменами
- **Результат:** Проблема подвійних слешів **ПОВНІСТЮ ВИПРАВЛЕНА** і протестована ✅

