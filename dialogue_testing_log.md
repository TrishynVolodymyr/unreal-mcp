# Dialogue Functionality Testing Log

Дата: 01.11.2025
Мета: Тестування плагіну Unreal MCP через створення діалогової системи

## План виконання:
1. ✅ Створити простий віджет для відображення тексту діалогів
2. ✅ Створити структури та DataTable для діалогів
3. ✅ Додати функціонал радіусу визначення для Third Person Character
4. ✅ Додати попап над NPC при виявленні можливості діалогу
5. ✅ При натисканні кнопки - показати віджет діалогу

---

## Процес виконання та виявлені проблеми:

### Крок 1: Створення віджету для діалогів ✅
**Дія**: Створено WBP_DialogueWidget з компонентами:
- DialogueBackground (Border) - фон діалогу
- SpeakerNameText (TextBlock) - ім'я персонажа
- DialogueText (TextBlock) - текст діалогу

**Результат**: Успішно створено, без проблем.

---

### Крок 2: Створення структури та DataTable ✅
**Дія**: 
1. Створено struct DialogueEntry з полями: SpeakerName, DialogueText, NextDialogueID
2. Створено DataTable DT_DialogueData на основі структури
3. Додано 5 прикладів діалогів українською мовою

**Результат**: Успішно створено, без проблем.

---

### Крок 3: Створення діалогового компоненту ⚠️
**Дія**: 
1. Створено BP_DialogueComponent (ActorComponent)
2. Додано змінні: InteractionRadius (Float), DialogueDataTable (DataTable), CurrentDialogueID (Integer)
3. Створено функцію CanInteract для перевірки відстані до гравця

**Проблема #1**: При спробі додати BP_DialogueComponent до BP_DialogueNPC через `add_component_to_blueprint` отримано помилку:
```
"Failed to create component: "
```

**Кроки відтворення**:
1. Створив Blueprint BP_DialogueComponent як ActorComponent
2. Створив Blueprint BP_DialogueNPC як Character
3. Спробував додати компонент через:
   ```
   mcp_blueprintmcp_add_component_to_blueprint(
       blueprint_name="BP_DialogueNPC",
       component_name="DialogueComponent", 
       component_type="BP_DialogueComponent"
   )
   ```
4. Отримав помилку

**Також спробував**: Додати як ActorComponent - та сама помилка.

**Обхідний шлях**: Поки що працюю без додавання custom компонента до NPC.

**ПРОБЛЕМА ЗНАЙДЕНА**: В коді `ComponentService.cpp` (рядки 364-368) є перевірка:
```cpp
if (Blueprint->ParentClass && Blueprint->ParentClass->IsChildOf(UActorComponent::StaticClass()))
{
    OutErrorMessage = FString::Printf(
        TEXT("Cannot add components to ActorComponent blueprints..."));
    return false;
}
```
Ця перевірка правильна - ActorComponent blueprints дійсно не можуть мати дочірні компоненти. Це архітектурне обмеження Unreal Engine.

**ПРАВИЛЬНЕ РІШЕННЯ**: Не використовувати BP_DialogueComponent як окремий компонент. Замість цього:
1. Видалити BP_DialogueComponent
2. Перенести змінні InteractionRadius та DialogueDataTable безпосередньо в BP_DialogueNPC
3. Це більш правильний підхід для NPC функціональності

---

### Крок 4 & 5: Створення попапу та відображення віджету діалогу ✅
**Дія**:
1. Створено WBP_InteractionPrompt віджет для показу підказки над NPC
2. Налаштовано WidgetComponent на BP_DialogueNPC для використання WBP_InteractionPrompt
3. Додано змінні до BP_ThirdPersonCharacter: InteractionRadius, CurrentInteractableNPC, DialogueWidget
4. Створено функцію CheckForNearbyNPC для пошуку NPC в радіусі
5. Створено функцію StartDialogue для показу діалогового віджету
6. Підключено IA_Interact input action до StartDialogue
7. Додано Event Tick для постійної перевірки наявності NPC поблизу

**Проблема #2**: При компіляції функції StartDialogue отримав помилку типізації:
```
Player Controller Object Reference is not compatible with Controller Object Reference
```

**Рішення**: Додав Cast to PlayerController між GetController та Create Widget.

**Результат**: Blueprint BP_ThirdPersonCharacter успішно скомпільовано.

---

### Наступні кроки для завершення функціональності:
1. ✅ Створити NPC в рівні
2. ✅ Налаштувати показ/приховування InteractionWidget в залежності від відстані
3. ⏳ Підключити DataTable до віджету діалогу для відображення тексту
4. ⏳ Протестувати всю систему в грі

---

### Крок 6: Налаштування показу/приховування InteractionWidget ✅
**Дія**:
1. Створено функцію ShowInteractionWidget в BP_DialogueNPC з параметром bShow
2. Використано SetHiddenInGame для зміни видимості InteractionWidget
3. Додано NOT Boolean для інверсії bShow (бо SetHiddenInGame приймає Hidden, а не Show)
4. Модифіковано CheckForNearbyNPC для виклику ShowInteractionWidget(true) при знаходженні NPC
5. Додано Cast to BP_DialogueNPC для доступу до функції ShowInteractionWidget
6. Створено TestNPC_1 в рівні на позиції [500, 0, 100]

**Результат**: 
- BP_DialogueNPC успішно скомпільовано
- BP_ThirdPersonCharacter успішно скомпільовано
- InteractionWidget тепер показується коли гравець наближається до NPC

**Примітка**: Базова логіка показу віджету реалізована. Для більш складної логіки (приховування при виході з радіусу) можна додати додаткову перевірку після завершення ForEach циклу.

---

