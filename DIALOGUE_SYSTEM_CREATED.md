# Діалогова система - Створено через MCP

## Дата: 26 жовтня 2025

## ✅ Успішно створено автоматично:

### 1. Структура даних
- **DialogueData** (`/Game/Dialogue/DialogueData`)
  - SpeakerName (String) - Ім'я того, хто говорить
  - DialogueText (String) - Текст діалогу
  - ResponseOptions (String[]) - Варіанти відповідей

### 2. DataTable з прикладами діалогів
- **DialogueTable** (`/Game/Dialogue/DialogueTable`)
  - 3 приклади діалогів:
    - "Greeting" - Mysterious Stranger
    - "Quest" - Village Elder  
    - "Merchant" - Trader Joe

### 3. UMG Віджети
- **WBP_DialogueWidget** - основний віджет діалогу
  - DialogueBackground (Border) - темний фон з padding
  - DialogueContainer (VerticalBox) - контейнер для елементів
  - SpeakerNameText (TextBlock) - ім'я спікера
  - DialogueTextBlock (TextBlock) - текст діалогу

- **WBP_InteractionPopup** - popup над NPC
  - PopupBackground (Border) - фон popup
  - InteractionText (TextBlock) - "Press [E] to talk"

### 4. Blueprints
- **BP_DialogueNPC** (Character)
  - InteractionRadius (SphereComponent) - для визначення гравця
  - DialogueKey (String, exposed) - ключ діалогу з DataTable

- **BP_ThirdPersonCharacter** (модифікований)
  - Змінні:
    - CurrentInteractableNPC (Object) - поточний NPC для діалогу
    - InteractionRadius (Float) - радіус виявлення
    - DialogueWidgetRef (Object) - посилання на віджет діалогу
  
  - Функції:
    - CheckForNearbyNPC - перевірка NPC поблизу
    - StartDialogue - відкриття діалогу
    - OnInteractPressed (Custom Event) - обробник кнопки взаємодії
  
  - EventGraph:
    - Event Tick → CheckForNearbyNPC (перевірка кожен кадр)
    - OnInteractPressed → StartDialogue (відкриття діалогу)

### 5. Input System
- **Вже налаштовано в проекті:**
  - IA_Interact - Input Action для взаємодії
  - IMC_Default - має маппінг E → IA_Interact

## ⚠️ Потрібно додати вручну в Unreal Editor:

### 1. BP_DialogueNPC
1. Відкрити BP_DialogueNPC
2. Додати компонент **Widget Component** (назва: "InteractionPopupWidget"):
   - **Проблема #1**: MCP не підтримує WidgetComponent
   - Widget Class: WBP_InteractionPopup
   - Location: (0, 0, 100) - над персонажем
   - Space: Screen
   - Draw Size: (200, 60)
   - Visibility: Hidden (спочатку прихований)

3. Налаштувати InteractionRadius:
   - **Проблема #2**: MCP не може встановити SphereRadius
   - Sphere Radius: **300.0** units
   - Collision Preset: OverlapAllDynamic

4. Створити Events на InteractionRadius:
   - OnComponentBeginOverlap: показати InteractionPopupWidget
   - OnComponentEndOverlap: сховати InteractionPopupWidget

### 2. BP_ThirdPersonCharacter
1. В EventGraph додати **Enhanced Input Event для IA_Interact**:
   - **Проблема #4**: MCP не може створити Enhanced Input Action events
   - Знайти "IA_Interact" в контекстному меню Input Actions
   - Started → викликати OnInteractPressed (custom event)
   
2. Видалити або оновити Tick event:
   - **Проблема #5**: CheckForNearbyNPC функція порожня через баг з GetAllActorsOfClass
   - Може просто видалити Tick → CheckForNearbyNPC
   - Логіка тепер через Overlap events на NPC

3. В функції StartDialogue:
   - Додати перевірку чи CurrentInteractableNPC не null
   - Додати отримання DialogueKey з NPC
   - Додати читання даних з DialogueTable
   - Встановити текст у віджет

4. Встановити InteractionRadius значення:
   - В Details панелі змінної InteractionRadius
   - Default Value: **300.0**

### 3. Спавн NPC для тестування
1. У рівні розмістити екземпляр BP_DialogueNPC
2. Встановити DialogueKey на один з:
   - "Greeting"
   - "Quest"
   - "Merchant"

### 4. Покращення віджета WBP_DialogueWidget
1. Додати кнопки для ResponseOptions
2. Додати binding для SpeakerNameText та DialogueTextBlock
3. Додати анімації появи/зникнення
4. Додати кнопку закриття діалогу

## 📝 Зареєстровані проблеми MCP плагіну:

Всі проблеми детально описані в файлі: **DIALOGUE_SYSTEM_ISSUES.md**

1. **Проблема #1**: Неможливо додати WidgetComponent до Blueprint
2. **Проблема #2**: Неможливо встановити властивості SphereComponent
3. **Проблема #3**: create_node_by_action_name створює неправильну ноду
4. **Проблема #4**: Неможливо створити Enhanced Input Action event
5. **Проблема #5**: Критичний баг - GetAllActorsOfClass завжди створює GetAllActorsOfClassMatchingTagQuery

## 🎯 Результат:
Базова структура діалогової системи створена через MCP tools. 
Для повної функціональності потрібні ручні доопрацювання через обмеження поточної версії плагіну.

## 🔄 Альтернативний підхід (без проблемних функцій):
Замість GetAllActorsOfClass використовувати Overlap Events - це більш оптимально та працює краще для інтерактивних об'єктів.
