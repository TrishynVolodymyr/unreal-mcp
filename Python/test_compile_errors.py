#!/usr/bin/env python3
"""
Test script to verify enhanced Blueprint compilation error reporting.
"""

import asyncio
import json

from utils.async_tcp_utils import send_tcp_command


async def test_compile_blueprint(blueprint_name: str):
    """Test compiling a Blueprint and display error details."""
    print(f"\n{'='*60}")
    print(f"Testing compilation of Blueprint: {blueprint_name}")
    print(f"{'='*60}\n")
    
    result = await send_tcp_command("compile_blueprint", {"blueprint_name": blueprint_name})
    
    print("Response received:")
    print(json.dumps(result, indent=2))
    
    if not result.get("success", False):
        print("\n" + "="*60)
        print("COMPILATION FAILED - Error Details:")
        print("="*60)
        print(result.get("error", "No error message"))
        
        if "compilation_errors" in result:
            print("\nDetailed Errors:")
            for i, error in enumerate(result["compilation_errors"], 1):
                print(f"{i}. {error}")
    else:
        print("\n" + "="*60)
        print("COMPILATION SUCCESSFUL")
        print("="*60)
        print(f"Time: {result.get('compilation_time_seconds', 0):.2f} seconds")
        print(f"Status: {result.get('status', 'unknown')}")
        
        if "warnings" in result and result["warnings"]:
            print("\nWarnings:")
            for i, warning in enumerate(result["warnings"], 1):
                print(f"{i}. {warning}")

async def main():
    """Main test function."""
    # Test with DialogueComponent which has compilation errors (as shown in screenshot)
    await test_compile_blueprint("DialogueComponent")
    
    # You can add more test cases here
    # await test_compile_blueprint("AnotherBlueprintName")

if __name__ == "__main__":
    asyncio.run(main())
