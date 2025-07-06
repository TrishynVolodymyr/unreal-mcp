# UMG Tools - Unreal MCP

This document provides comprehensive information about using UMG (Unreal Motion Graphics) tools through the Unreal MCP (Model Context Protocol). These tools allow you to create and manage user interface widgets, build complex UI layouts, and set up interactive elements using natural language commands through AI assistants.

## Overview

UMG tools enable you to:
- Create UMG Widget Blueprints for user interfaces
- Add and configure UI components (buttons, text, images, etc.)
- Build complex layouts with containers and hierarchies
- Set up event bindings for interactive elements
- Manage widget placement, sizing, and alignment
- Add widgets to the viewport with z-ordering
- Create property bindings for dynamic content
- Get layout information and container dimensions

## Natural Language Usage Examples

All examples below show how to request these operations using natural language with your AI assistant.

### Creating Widget Blueprints

**Creating Basic Widgets:**
```
"Create a UMG Widget Blueprint called 'MainMenu'"

"Make a new widget called 'PlayerHUD' for the game interface"

"Create a widget Blueprint named 'SettingsMenu' in the UI folder"

"Make a new widget called 'InventoryScreen' for item management"

"Create a widget Blueprint called 'DialogueBox' for conversations"
```

**Creating Widgets with Custom Parent Classes:**
```
"Create a widget called 'BaseMenu' with a custom parent class 'MyUserWidget'"

"Make a widget Blueprint called 'HUDWidget' based on the BaseHUD class"

"Create a widget called 'InteractiveWidget' with parent class 'MyInteractiveBase'"
```

### Adding Widget Components

**Adding Basic Components:**
```
"Add a text block called 'TitleText' to the MainMenu widget"

"Add a button named 'StartButton' to the MainMenu widget"

"Create an image component called 'BackgroundImage' in the PlayerHUD widget"

"Add a progress bar named 'HealthBar' to the HUD widget"

"Add a text block called 'ScoreText' at position [100, 50] in the HUD widget"
```

**Adding Components with Positioning:**
```
"Add a button called 'QuitButton' at position [400, 300] with size [200, 60] to MainMenu"

"Create a text block 'PlayerName' at [50, 20] with size [300, 40] in the HUD widget"

"Add an image 'PlayerAvatar' at position [20, 20] with size [80, 80] to the HUD"

"Add a button 'InventoryButton' at [600, 50] sized [120, 40] to the game interface"
```

### Layout and Container Management

**Creating Container Hierarchies:**
```
"Add a vertical box called 'ButtonContainer' to the MainMenu widget"

"Create a horizontal box named 'TopBar' for the HUD layout"

"Add a scroll box called 'ItemsList' for the inventory screen"

"Create a border component called 'MenuBorder' as a background container"

"Add a canvas panel called 'MainCanvas' as the root container"
```

**Adding Components to Containers:**
```
"Add the StartButton as a child of the ButtonContainer in MainMenu"

"Put the PlayerName text block inside the TopBar horizontal box"

"Add the HealthBar to the HUD's TopBar container"

"Place the ItemsList scroll box inside the MenuBorder"

"Add all menu buttons to the ButtonContainer vertical box"
```

**Creating Parent-Child Relationships:**
```
"Create a border called 'ContentArea' and add a text block 'ContentText' inside it"

"Make a vertical box 'MenuOptions' and add three buttons: Start, Settings, Quit"

"Create a horizontal box 'StatusBar' with text blocks for Health, Mana, and Score"

"Build a scroll box 'ChatHistory' with multiple text blocks for chat messages"
```

### Widget Properties and Styling

**Setting Text Properties:**
```
"Set the TitleText in MainMenu to display 'My Awesome Game'"

"Change the PlayerName text block to show 'Player 1'"

"Set the ScoreText content to '0' and make the font size 24"

"Update the ContentText to display 'Welcome to the game!'"
```

**Setting Button Properties:**
```
"Set the StartButton text to 'Start Game'"

"Change the QuitButton background color to red"

"Set the InventoryButton text to 'Inventory' and make it blue"

"Configure the SettingsButton with text 'Settings' and gray background"
```

**Setting Component Colors and Appearance:**
```
"Set the MenuBorder background color to semi-transparent black"

"Change the HealthBar color to green"

"Set the BackgroundImage to use transparency 50%"

"Make the TitleText color bright yellow"
```

**Advanced Property Configuration:**
```
"Set the MenuBorder background to [0.1, 0.1, 0.1, 0.8] for dark transparent background"

"Configure the HealthBar with green color [0, 1, 0, 1] and 100% fill"

"Set the StartButton background color to [0.2, 0.6, 1.0, 1.0] for blue"

"Configure the TitleText with color [1, 1, 0, 1] and font size 48"
```

### Widget Placement and Layout

**Setting Component Placement:**
```
"Move the TitleText to position [400, 100] in the MainMenu"

"Set the StartButton position to [350, 200] with size [300, 60]"

"Place the HealthBar at [50, 50] with size [200, 20] in the HUD"

"Position the QuitButton at [350, 400] in the main menu"
```

**Setting Alignment:**
```
"Center-align the TitleText in the MainMenu"

"Set the StartButton alignment to center both horizontally and vertically"

"Align the HealthBar to the top-left corner"

"Center the MenuBorder both horizontally and vertically"
```

**Getting Container Dimensions:**
```
"What are the dimensions of the MainCanvas in MainMenu?"

"Get the size of the ButtonContainer in the main menu"

"Show me the dimensions of the TopBar container in the HUD"

"What's the size of the root canvas panel in InventoryScreen?"
```

### Event Binding and Interactivity

**Button Event Binding:**
```
"Bind the StartButton OnClicked event to a function called 'StartGame' in MainMenu"

"Set up the QuitButton to call 'ExitGame' when clicked"

"Bind the InventoryButton click event to 'OpenInventory'"

"Connect the SettingsButton OnClicked to 'ShowSettings' function"

"Set up the SaveButton to trigger 'SaveGame' when pressed"
```

**Custom Function Names:**
```
"Bind the StartButton OnClicked event to a custom function called 'BeginGameplay'"

"Connect the RestartButton click to a function named 'RestartLevel'"

"Set up the PauseButton to call 'TogglePause' when clicked"

"Bind the BackButton OnClicked to 'ReturnToMainMenu' function"
```

### Property Binding and Dynamic Content

**Text Block Property Binding:**
```
"Set up text binding for ScoreText to display the 'CurrentScore' property"

"Bind the PlayerName text block to the 'PlayerNameString' property"

"Connect the HealthText to display the 'PlayerHealth' value"

"Set up the TimerText to show the 'RemainingTime' property"

"Bind the LevelText to display the 'CurrentLevel' property"
```

**Dynamic Content Updates:**
```
"Create text binding for StatusText to show game status updates"

"Set up the ExperienceText to display 'PlayerExperience' with automatic updates"

"Bind the MoneyText to the 'CurrentCurrency' property for real-time updates"

"Connect the MessageText to 'SystemMessage' for dynamic notifications"
```

### Viewport Management

**Adding Widgets to Viewport:**
```
"Add the MainMenu widget to the viewport"

"Show the PlayerHUD widget on screen with z-order 5"

"Display the InventoryScreen widget above other elements"

"Add the DialogueBox widget to viewport with high z-order priority"

"Show the SettingsMenu widget on top of everything else"
```

**Z-Order Management:**
```
"Add the HUD widget with z-order 1 for background UI"

"Display the MainMenu with z-order 10 to appear on top"

"Show the NotificationWidget with z-order 20 for alerts"

"Add the LoadingScreen widget with maximum z-order priority"
```

### Layout Information and Debugging

**Getting Component Layout:**
```
"Show me the layout hierarchy of the MainMenu widget"

"Get the layout information for all components in the PlayerHUD"

"Display the component structure of the InventoryScreen widget"

"Show me how components are arranged in the SettingsMenu"
```

**Component Existence Checks:**
```
"Check if the StartButton component exists in MainMenu"

"Verify that the HealthBar component is present in PlayerHUD"

"Does the InventoryScreen have a component called 'ItemsList'?"

"Check if the TitleText component exists in the main menu"
```

## Advanced Usage Patterns

### Creating Complete Menu Systems

**Main Menu Creation:**
```
"Create a complete main menu system:
1. Create UMG widget 'MainMenu'
2. Add background border with dark transparent color
3. Add title text 'My Game' at the top center
4. Create vertical box for buttons
5. Add Start, Settings, and Quit buttons to the container
6. Set up click events for each button
7. Add the widget to viewport"
```

**Player HUD Setup:**
```
"Build a complete player HUD:
1. Create UMG widget 'PlayerHUD' 
2. Add horizontal box 'TopBar' at top of screen
3. Add health bar, mana bar, and score text to TopBar
4. Add mini-map image in top-right corner
5. Add inventory button in bottom-right
6. Set up property bindings for dynamic values
7. Add to viewport with z-order 1"
```

### Inventory System Interface

**Inventory Screen Creation:**
```
"Create inventory interface:
1. Create UMG widget 'InventoryScreen'
2. Add border background for main area
3. Create scroll box for item list
4. Add horizontal box for each item slot
5. Include item image, name text, and quantity text per slot
6. Add close button in top-right corner
7. Set up event bindings for item interactions"
```

### Dialogue System UI

**Dialogue Box Setup:**
```
"Build dialogue system interface:
1. Create UMG widget 'DialogueBox'
2. Add border for dialogue background
3. Add text block for speaker name
4. Add larger text block for dialogue content
5. Create horizontal box for response buttons
6. Add portrait image for character
7. Set up text bindings for dynamic content"
```

### Settings Menu Interface

**Settings Screen Creation:**
```
"Create settings menu:
1. Create UMG widget 'SettingsMenu'
2. Add vertical box for settings categories
3. Add horizontal boxes for each setting row
4. Include text labels and input controls (sliders, checkboxes)
5. Add Apply and Cancel buttons at bottom
6. Set up event bindings for all controls
7. Add background border for visual separation"
```

## Best Practices for Natural Language Commands

### Be Specific with Component Names
Instead of: *"Add a button"*  
Use: *"Add a button called 'StartButton' to the MainMenu widget"*

### Include Position and Size Information
Instead of: *"Add text to the widget"*  
Use: *"Add a text block 'TitleText' at position [400, 100] with size [200, 50] to MainMenu"*

### Specify Container Relationships
Instead of: *"Put the button in the container"*  
Use: *"Add the StartButton as a child of the ButtonContainer vertical box"*

### Name Events and Functions Clearly
Instead of: *"Set up the button click"*  
Use: *"Bind the StartButton OnClicked event to a function called 'StartGame'"*

### Group Related Operations
*"Create MainMenu with title text 'Game Title', add vertical box, and add Start, Settings, Quit buttons with proper event bindings"*

## Common Use Cases

### Game Menus
- Creating main menus with navigation buttons
- Building settings screens with controls
- Designing pause menus with game options
- Creating save/load interfaces

### HUD Elements
- Building player status displays (health, mana, score)
- Creating mini-maps and radar displays
- Designing notification and alert systems
- Building chat and communication interfaces

### Inventory Systems
- Creating item grid layouts
- Building drag-and-drop interfaces
- Designing equipment and character screens
- Creating trading and shop interfaces

### Dialogue Systems
- Building conversation interfaces
- Creating choice selection systems
- Designing quest and objective displays
- Building narrative text presentations

### Administrative Interfaces
- Creating debug and development tools
- Building analytics and statistics displays
- Designing configuration and setup screens
- Creating editor and level-building tools

## Layout Architecture

### Container-Based Design
Use container widgets (Vertical Box, Horizontal Box, Canvas Panel) to organize components logically and maintain responsive layouts.

### Hierarchical Structure
Build interfaces using parent-child relationships where containers hold related components, making them easier to manage and style.

### Responsive Design
Consider different screen sizes and resolutions when positioning components, using alignment and anchoring appropriately.

### Event-Driven Interaction
Use event bindings to connect UI interactions with gameplay logic, keeping the interface responsive to user actions.

## Error Handling and Troubleshooting

If you encounter issues:

1. **Component Not Found**: Use "check if [component] exists in [widget]" to verify component presence
2. **Layout Problems**: Use "get layout information for [widget]" to understand component hierarchy
3. **Event Binding Issues**: Ensure function names are valid and components exist before binding
4. **Property Binding Problems**: Verify property names match exactly with Blueprint variables

## Performance Considerations

- Use z-ordering efficiently to avoid unnecessary overdraw
- Minimize the number of widgets in viewport simultaneously
- Use property bindings sparingly for frequently updated values
- Consider widget pooling for dynamic lists with many items
- Use appropriate container types for your layout needs

Remember that all operations are performed through natural language with your AI assistant, making UI creation and management intuitive and accessible without requiring detailed knowledge of Unreal Engine's UMG editor interface. 