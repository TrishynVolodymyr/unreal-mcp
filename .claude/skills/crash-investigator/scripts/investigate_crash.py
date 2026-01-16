#!/usr/bin/env python3
"""
UE Crash Investigator - Parse crash logs and correlate with MCP tool calls.
Identifies which MCP operation likely caused the crash.
"""

import argparse
import re
import sys
from pathlib import Path
from dataclasses import dataclass
from datetime import datetime
from typing import Optional

# Patterns for crash identification
CRASH_PATTERNS = {
    'connection_reset': (r'WinError 10054|Connection.*reset|WSAECONNRESET', 
                         'MCP connection lost mid-operation'),
    'access_violation': (r'Access violation|EXCEPTION_ACCESS_VIOLATION|reading.*0x0+\b',
                         'Null pointer - likely failed MCP object creation'),
    'ensure_failed': (r'Ensure.*failed|ensure\(\)|ensureMsg',
                      'Validation failed - MCP returned unexpected data'),
    'check_failed': (r'Check.*failed|check\(\)|checkf',
                     'Critical assertion - MCP state corruption'),
    'pure_virtual': (r'pure virtual|R6025',
                     'Object lifetime issue - MCP holding stale reference'),
    'out_of_memory': (r'OutOfMemory|out of memory|allocation failed',
                      'Memory exhausted - possible MCP infinite loop'),
    'fatal_error': (r'Fatal error|FatalError|Unhandled Exception',
                    'Unrecoverable error during MCP operation'),
}

# MCP log patterns
MCP_LOG_PATTERN = re.compile(
    r'\[(\d{4}\.\d{2}\.\d{2}-\d{2}\.\d{2}\.\d{2}:\d{3})\].*?(LogMCP\w*):?\s*(.*)',
    re.IGNORECASE
)

TIMESTAMP_PATTERN = re.compile(r'\[(\d{4}\.\d{2}\.\d{2}-\d{2}\.\d{2}\.\d{2}:\d{3})\]')


@dataclass
class CrashInfo:
    crash_type: str
    description: str
    timestamp: Optional[str]
    error_message: str
    call_stack: list[str]
    log_file: Path


@dataclass 
class MCPOperation:
    timestamp: str
    category: str
    message: str
    line_num: int


def find_latest_crash_folder(project_path: Path) -> Optional[Path]:
    """Find the most recent crash folder."""
    crashes_dir = project_path / "Saved" / "Crashes"
    if not crashes_dir.exists():
        return None
    
    crash_folders = [f for f in crashes_dir.iterdir() if f.is_dir()]
    if not crash_folders:
        return None
    
    # Sort by modification time, newest first
    crash_folders.sort(key=lambda x: x.stat().st_mtime, reverse=True)
    return crash_folders[0]


def find_crash_log(crash_folder: Path) -> Optional[Path]:
    """Find the crash log file in a crash folder."""
    # UE crash logs can have various names
    for pattern in ['*.log', '*.txt']:
        logs = list(crash_folder.glob(pattern))
        for log in logs:
            # Prefer the project log or crash context
            if 'MCPGameProject' in log.name or 'CrashContext' in log.name:
                return log
        if logs:
            return logs[0]
    return None


def parse_crash_log(log_path: Path) -> CrashInfo:
    """Parse a crash log file for error information."""
    content = log_path.read_text(encoding='utf-8', errors='replace')
    lines = content.split('\n')
    
    # Identify crash type
    crash_type = 'unknown'
    description = 'Unknown crash type'
    for ctype, (pattern, desc) in CRASH_PATTERNS.items():
        if re.search(pattern, content, re.IGNORECASE):
            crash_type = ctype
            description = desc
            break
    
    # Extract error message (first line matching error patterns)
    error_message = "Could not extract error message"
    for line in lines:
        for _, (pattern, _) in CRASH_PATTERNS.items():
            if re.search(pattern, line, re.IGNORECASE):
                error_message = line.strip()[:200]
                break
        if error_message != "Could not extract error message":
            break
    
    # Extract timestamp
    timestamp = None
    ts_match = TIMESTAMP_PATTERN.search(content)
    if ts_match:
        timestamp = ts_match.group(1)
    
    # Extract call stack (lines starting with common stack patterns)
    call_stack = []
    in_stack = False
    for line in lines:
        if re.match(r'^\s*(0x[0-9a-f]+|UE4Editor|UnrealEditor|\[Callstack\]|Script Stack)', line, re.IGNORECASE):
            in_stack = True
        if in_stack:
            stripped = line.strip()
            if stripped and not stripped.startswith('[20'):  # Skip timestamp lines
                call_stack.append(stripped)
            if len(call_stack) > 30:  # Limit stack depth
                break
        if in_stack and line.strip() == '':
            in_stack = False
    
    return CrashInfo(
        crash_type=crash_type,
        description=description,
        timestamp=timestamp,
        error_message=error_message,
        call_stack=call_stack[:20],
        log_file=log_path
    )


def extract_mcp_operations(project_path: Path, limit: int = 30) -> list[MCPOperation]:
    """Extract recent MCP operations from the main project log."""
    log_path = project_path / "Saved" / "Logs" / "MCPGameProject.log"
    if not log_path.exists():
        return []
    
    operations = []
    with open(log_path, 'r', encoding='utf-8', errors='replace') as f:
        for line_num, line in enumerate(f, 1):
            match = MCP_LOG_PATTERN.search(line)
            if match:
                operations.append(MCPOperation(
                    timestamp=match.group(1),
                    category=match.group(2),
                    message=match.group(3).strip()[:150],
                    line_num=line_num
                ))
    
    # Return the last N operations
    return operations[-limit:]


def correlate_crash_with_mcp(crash: CrashInfo, operations: list[MCPOperation]) -> dict:
    """Analyze which MCP operation likely caused the crash."""
    correlation = {
        'likely_culprit': None,
        'confidence': 'low',
        'reasoning': [],
        'suspicious_ops': []
    }
    
    if not operations:
        correlation['reasoning'].append("No MCP operations found in log")
        return correlation
    
    # Check for error/warning operations
    for op in reversed(operations):
        msg_lower = op.message.lower()
        if any(word in msg_lower for word in ['error', 'failed', 'exception', 'invalid']):
            correlation['suspicious_ops'].append(op)
    
    # The last operation is often the culprit
    last_op = operations[-1]
    correlation['likely_culprit'] = last_op
    
    # Increase confidence based on crash type and operation
    if crash.crash_type == 'connection_reset':
        correlation['confidence'] = 'high'
        correlation['reasoning'].append("Connection reset typically occurs during active MCP call")
    elif crash.crash_type == 'access_violation' and 'create' in last_op.message.lower():
        correlation['confidence'] = 'high'
        correlation['reasoning'].append("Access violation after create operation suggests null return")
    elif correlation['suspicious_ops']:
        correlation['confidence'] = 'medium'
        correlation['reasoning'].append(f"Found {len(correlation['suspicious_ops'])} suspicious operations before crash")
    else:
        correlation['reasoning'].append("Last MCP operation before crash is the likely trigger")
    
    return correlation


def format_report(crash: CrashInfo, operations: list[MCPOperation], correlation: dict) -> str:
    """Format the investigation report."""
    lines = []
    
    # Header
    lines.append("=" * 80)
    lines.append("                    UE CRASH INVESTIGATION REPORT")
    lines.append("=" * 80)
    
    # Crash Summary
    lines.append("\n## CRASH SUMMARY\n")
    lines.append(f"  Type:        {crash.crash_type.replace('_', ' ').title()}")
    lines.append(f"  Diagnosis:   {crash.description}")
    lines.append(f"  Timestamp:   {crash.timestamp or 'Unknown'}")
    lines.append(f"  Log File:    {crash.log_file}")
    lines.append(f"\n  Error Message:")
    lines.append(f"    {crash.error_message}")
    
    # Call Stack (abbreviated)
    if crash.call_stack:
        lines.append(f"\n  Call Stack (top {min(10, len(crash.call_stack))} frames):")
        for frame in crash.call_stack[:10]:
            lines.append(f"    {frame}")
    
    # MCP Timeline
    lines.append("\n" + "-" * 80)
    lines.append("\n## MCP OPERATIONS BEFORE CRASH\n")
    
    if operations:
        lines.append(f"  Last {len(operations)} MCP operations:\n")
        for i, op in enumerate(operations):
            marker = " >>> " if op == correlation.get('likely_culprit') else "     "
            suspicious = " ⚠" if op in correlation.get('suspicious_ops', []) else ""
            lines.append(f"{marker}[{op.timestamp}] {op.category}: {op.message}{suspicious}")
    else:
        lines.append("  No MCP operations found in log")
    
    # Correlation Analysis
    lines.append("\n" + "-" * 80)
    lines.append("\n## CORRELATION ANALYSIS\n")
    
    culprit = correlation.get('likely_culprit')
    if culprit:
        lines.append(f"  Likely Culprit: {culprit.category}")
        lines.append(f"  Operation:      {culprit.message}")
        lines.append(f"  Confidence:     {correlation['confidence'].upper()}")
    
    lines.append(f"\n  Reasoning:")
    for reason in correlation.get('reasoning', []):
        lines.append(f"    • {reason}")
    
    # Recovery Hints
    lines.append("\n" + "-" * 80)
    lines.append("\n## RECOVERY HINTS\n")
    
    hints = {
        'connection_reset': [
            "Restart the MCP server",
            "Check for long-running operations that might timeout",
            "Verify network stability between editor and MCP server"
        ],
        'access_violation': [
            "Add IsValid() checks after MCP object creation/retrieval",
            "Verify the target asset exists before MCP operations",
            "Check for race conditions in async MCP calls"
        ],
        'ensure_failed': [
            "Validate MCP tool parameters before calling",
            "Check that required inputs are not empty/null",
            "Review the specific ensure condition in call stack"
        ],
        'pure_virtual': [
            "Ensure objects aren't garbage collected during MCP operations",
            "Add UPROPERTY() to prevent premature cleanup",
            "Check for use-after-destroy patterns"
        ],
        'out_of_memory': [
            "Check for infinite loops in MCP create operations",
            "Add iteration limits to batch processing",
            "Monitor asset creation counts"
        ],
    }
    
    for hint in hints.get(crash.crash_type, ["Review crash log manually for more details"]):
        lines.append(f"  • {hint}")
    
    lines.append("\n" + "=" * 80)
    
    return '\n'.join(lines)


def main():
    parser = argparse.ArgumentParser(
        description='Investigate UE crashes and correlate with MCP operations'
    )
    parser.add_argument('project_path', type=Path, help='Path to MCPGameProject root')
    parser.add_argument('--crash-id', type=str, help='Specific crash folder name (default: latest)')
    
    args = parser.parse_args()
    
    if not args.project_path.exists():
        print(f"Error: Project path not found: {args.project_path}", file=sys.stderr)
        sys.exit(1)
    
    # Find crash folder
    if args.crash_id:
        crash_folder = args.project_path / "Saved" / "Crashes" / args.crash_id
        if not crash_folder.exists():
            print(f"Error: Crash folder not found: {crash_folder}", file=sys.stderr)
            sys.exit(1)
    else:
        crash_folder = find_latest_crash_folder(args.project_path)
        if not crash_folder:
            print("Error: No crash folders found in Saved/Crashes/", file=sys.stderr)
            sys.exit(1)
    
    print(f"Investigating crash: {crash_folder.name}", file=sys.stderr)
    
    # Find and parse crash log
    crash_log = find_crash_log(crash_folder)
    if not crash_log:
        print(f"Error: No log file found in crash folder", file=sys.stderr)
        sys.exit(1)
    
    crash = parse_crash_log(crash_log)
    
    # Extract MCP operations
    operations = extract_mcp_operations(args.project_path)
    
    # Correlate
    correlation = correlate_crash_with_mcp(crash, operations)
    
    # Output report
    print(format_report(crash, operations, correlation))


if __name__ == '__main__':
    main()
