
# Guidelines for using Python for MCP Tools

The following guidelines apply to any method or function marked with the @mcp.tool() decorator.

- Parameters should not have any of the following types: `Any`, `object`, `Optional[T]`, `Union[T]`.
- For a given parameter `x` of type `T` that has a default value, do not use type `x : T | None = None`. Instead, use `x: T = None` and handle defaults within the method body itself.
- Always include method docstrings and make sure to given proper examples of valid inputs especially when no type hints are present.

When this rule is applied, please remember to explicitly mention it.

Python/tools/* - is a mcp entrypoint but a decorator, the real functionalities stored outside this file and you will need to use existing prefered file for that kind of funtionalities,or to create new one and add any new in there 


When you have provided cpp changes - also apply prompt in the end of all your edits this command

cd e:\code\unreal-mcp\MCPGameProject; & "C:\Program Files\Epic Games\UE_5.5\Engine\Build\BatchFiles\Build.bat" MCPGameProjectEditor Win64 Development -Project="e:\code\unreal-mcp\MCPGameProject\MCPGameProject.uproject" -TargetType=Editor