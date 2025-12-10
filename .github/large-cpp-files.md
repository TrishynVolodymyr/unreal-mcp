# Великі C++ файли проекту (>1000 рядків)

**ВАЖЛИВО**: Ці файли потребують особливої уваги через їх розмір. При редагуванні використовуйте `multi_replace_string_in_file` для множинних змін.

## Список файлів

| Рядків | Файл | Опис |
|--------|------|------|
| 2119 | `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/Utils/UnrealMCPCommonUtils.cpp` | Утиліти загального призначення |
| 1709 | `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/Services/BlueprintService.cpp` | Сервіс для роботи з Blueprint |
| 1612 | `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/Services/BlueprintNodeService.cpp` | Сервіс створення та управління Blueprint нодами |
| 1486 | `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/Services/UMG/WidgetComponentService.cpp` | Сервіс для UMG компонентів |
| 1175 | `MCPGameProject/Packaged/Source/UnrealMCP/Private/Commands/UnrealMCPBlueprintCommands.cpp` | Blueprint команди (packaged версія) |
| 1168 | `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/Services/BlueprintNodeCreationService.cpp` | Сервіс створення Blueprint нодів |

## Рекомендації по роботі

### При читанні
- Використовуйте `grep_search` для пошуку конкретних функцій замість читання всього файлу
- Читайте тільки необхідні секції через `read_file` з чіткими діапазонами рядків

### При редагуванні
- **НЕ СТВОРЮЙТЕ** нові файли понад 1000 рядків
- Розбивайте великі файли на менші модулі при рефакторингу
- Використовуйте `multi_replace_string_in_file` для пакетних змін
- Тестуйте зміни інкрементально

### При додаванні функціоналу
- Додавайте нову логіку в окремі файли замість розширення існуючих
- Створюйте нові сервісні класи для нових доменів
- Тримайте файли під 800 рядків для оптимальної роботи AI

## Потенційні кандидати для рефакторингу

1. **UnrealMCPCommonUtils.cpp** (2119 рядків) - можна розділити на:
   - `AssetUtils.cpp` - операції з асетами
   - `ValidationUtils.cpp` - валідація та перевірки
   - `ConversionUtils.cpp` - конвертація типів

2. **BlueprintService.cpp** (1709 рядків) - можна розділити на:
   - `BlueprintCreationService.cpp` - створення Blueprint
   - `BlueprintPropertyService.cpp` - робота з властивостями
   - `BlueprintCompilationService.cpp` - компіляція
