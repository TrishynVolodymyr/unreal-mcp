# Known Issues and Future Improvements

This file tracks MCP tool limitations discovered during development. When encountering issues, document them here.

## Currently No Known Issues

All previously tracked issues have been resolved:

- **Array Length Helper** - Fixed in BlueprintActionDatabaseNodeCreator.cpp
- **TextBlock Font Size** - Fixed in UMGService.cpp (use `font_size` kwarg)
- **Orphaned Nodes Detection** - Implemented in metadata commands (use `fields=["orphaned_nodes"]`)

## Notes

- When working on MCP improvements, check this file first
- Add new issues as they are discovered during development
