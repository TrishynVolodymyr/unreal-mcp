# Quick Setup Guide - Діалогова система

## Швидке завершення налаштування (5-10 хвилин)

### Крок 1: Налаштування BP_DialogueNPC

1. Відкрити `Content/Dialogue/BP_DialogueNPC`
2. В Components панелі:
   - Додати **Widget Component** (Add Component → Widget)
     - Rename: `InteractionPopupWidget`
     - Details → User Interface:
       - Widget Class: `WBP_InteractionPopup`
       - Draw Size: (200, 60)
       - Pivot: (0.5, 1.0) - центр внизу
     - Transform:
       - Location: (0, 0, 100) - над головою NPC
     - Rendering:
       - Space: Screen
       - Visibility: Hidden
   
   - Вибрати **InteractionRadius**:
     - Details → Shape:
       - Sphere Radius: **300**
     - Details → Collision:
       - Collision Presets: OverlapAllDynamic
       - Object Type: WorldDynamic
       - ☑ Generate Overlap Events

3. В Event Graph:
   - Знайти InteractionRadius в Components
   - Перетягнути в граф
   - Right Click → Add Event → OnComponentBeginOverlap
     - З'єднати з: InteractionPopupWidget → SetVisibility(true)
   
   - Right Click → Add Event → OnComponentEndOverlap
     - З'єднати з: InteractionPopupWidget → SetVisibility(false)

4. Compile і Save

### Крок 2: Налаштування BP_ThirdPersonCharacter

1. Відкрити `Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter`

2. В Event Graph:
   - Right Click → Input → Input Actions → **IA_Interact**
   - Вибрати "Started"
   - З'єднати з существующим Custom Event: **OnInteractPressed**
   
   - (Опціонально) Видалити Event Tick → CheckForNearbyNPC якщо не потрібно

3. В функції **StartDialogue**:
   - Додати Branch: перевірка CurrentInteractableNPC != null
   - Якщо Valid:
     - Create Widget (вже є) → Add to Viewport (вже є)
   - Compile і Save

4. В Details панелі:
   - Variables → InteractionRadius:
     - Default Value: **300.0**

### Крок 3: Тестування

1. Відкрити рівень (наприклад, Content/ThirdPerson/Maps/ThirdPersonMap)
2. Додати екземпляр **BP_DialogueNPC**:
   - Place Actors → All Classes → BP_DialogueNPC
   - Розмістити в рівні
3. Вибрати екземпляр NPC:
   - Details → DialogueKey: **"Greeting"**
4. PIE (Play In Editor)
5. Підійти до NPC (має з'явитись "Press [E] to talk")
6. Натиснути **E** → має відкритись діалоговий віджет

### Якщо щось не працює:

1. **Popup не показується:**
   - Перевірити Visibility InteractionPopupWidget: Hidden за замовчуванням
   - Перевірити InteractionRadius: Generate Overlap Events ☑

2. **Діалог не відкривається:**
   - Перевірити Input: IA_Interact має бути прив'язаний до OnInteractPressed
   - Перевірити StartDialogue: перевірка на null

3. **Popup надто низько/високо:**
   - Змінити Location Z InteractionPopupWidget (зараз 100)

## Що далі:

- Додати логіку читання з DialogueTable в StartDialogue
- Додати binding для текстових полів у віджеті
- Додати кнопки для ResponseOptions
- Додати анімації

Файл з повною документацією: `DIALOGUE_SYSTEM_CREATED.md`
