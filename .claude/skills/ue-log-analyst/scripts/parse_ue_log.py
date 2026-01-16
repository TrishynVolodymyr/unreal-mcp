#!/usr/bin/env python3
"""
UE Log Parser - Extract user-generated logs from Unreal Engine PIE sessions.
Filters for LogBlueprintUserMessages, LogMCP*, and LogTemp only.
"""

import argparse
import re
import sys
from pathlib import Path
from dataclasses import dataclass
from typing import Optional

# Categories to extract (everything else is ignored)
TARGET_CATEGORIES = [
    r'LogBlueprintUserMessages',
    r'LogMCP\w*',  # LogMCP, LogMCPBlueprint, LogMCPEditor, etc.
    r'LogTemp',
]

# Compiled pattern for matching target categories
CATEGORY_PATTERN = re.compile(
    r'^\[[\d.:-]+\]\[[\s\d]+\](' + '|'.join(TARGET_CATEGORIES) + r'):\s*(.*)',
    re.IGNORECASE
)

# Pattern for PIE session markers
PIE_START_PATTERN = re.compile(r'PIE.*(?:Play in editor|BeginPlay|Starting PIE)', re.IGNORECASE)
PIE_END_PATTERN = re.compile(r'PIE.*(?:End|Shutdown|Stopping)', re.IGNORECASE)

# Full log line pattern to extract timestamp
TIMESTAMP_PATTERN = re.compile(r'^\[(\d{4}\.\d{2}\.\d{2}-\d{2}\.\d{2}\.\d{2}:\d{3})\]')


@dataclass
class LogEntry:
    timestamp: str
    category: str
    level: str
    message: str
    line_number: int
    
    def __str__(self):
        level_indicator = f"[{self.level}] " if self.level != "Log" else ""
        return f"[{self.timestamp}] {self.category}: {level_indicator}{self.message}"


def parse_log_line(line: str, line_num: int) -> Optional[LogEntry]:
    """Parse a single log line if it matches our target categories."""
    
    # Try to match against target categories
    match = CATEGORY_PATTERN.match(line)
    if not match:
        return None
    
    category = match.group(1)
    content = match.group(2)
    
    # Extract timestamp
    ts_match = TIMESTAMP_PATTERN.match(line)
    timestamp = ts_match.group(1) if ts_match else "Unknown"
    
    # Detect log level (Warning, Error, etc.)
    level = "Log"
    if content.startswith("Warning:"):
        level = "Warning"
        content = content[8:].strip()
    elif content.startswith("Error:"):
        level = "Error"
        content = content[6:].strip()
    elif content.startswith("Display:"):
        level = "Display"
        content = content[8:].strip()
    
    return LogEntry(
        timestamp=timestamp,
        category=category,
        level=level,
        message=content.strip(),
        line_number=line_num
    )


def find_last_pie_session(lines: list[str]) -> tuple[int, int]:
    """Find the start and end line indices of the last PIE session."""
    last_start = 0
    last_end = len(lines)
    
    for i, line in enumerate(lines):
        if PIE_START_PATTERN.search(line):
            last_start = i
        elif PIE_END_PATTERN.search(line) and i > last_start:
            last_end = i
    
    return last_start, last_end


def parse_log_file(
    filepath: Path,
    last_session_only: bool = False,
    category_filter: Optional[str] = None
) -> list[LogEntry]:
    """Parse the log file and return filtered entries."""
    
    if not filepath.exists():
        print(f"Error: Log file not found: {filepath}", file=sys.stderr)
        sys.exit(1)
    
    with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
        lines = f.readlines()
    
    # Determine line range
    start_line, end_line = 0, len(lines)
    if last_session_only:
        start_line, end_line = find_last_pie_session(lines)
        if start_line == 0 and end_line == len(lines):
            print("Note: No PIE session markers found, showing all matching logs", file=sys.stderr)
    
    entries = []
    for i, line in enumerate(lines[start_line:end_line], start=start_line + 1):
        entry = parse_log_line(line.strip(), i)
        if entry:
            # Apply category filter if specified
            if category_filter and category_filter.lower() not in entry.category.lower():
                continue
            entries.append(entry)
    
    return entries


def format_output(entries: list[LogEntry]) -> str:
    """Format entries for display."""
    if not entries:
        return "No matching log entries found."
    
    output_lines = []
    output_lines.append(f"Found {len(entries)} log entries:\n")
    output_lines.append("-" * 80)
    
    # Group by category for summary
    categories = {}
    for entry in entries:
        cat = entry.category
        if cat not in categories:
            categories[cat] = {"total": 0, "warnings": 0, "errors": 0}
        categories[cat]["total"] += 1
        if entry.level == "Warning":
            categories[cat]["warnings"] += 1
        elif entry.level == "Error":
            categories[cat]["errors"] += 1
    
    output_lines.append("\nSummary by Category:")
    for cat, stats in sorted(categories.items()):
        warn_err = ""
        if stats["warnings"] or stats["errors"]:
            warn_err = f" (⚠ {stats['warnings']} warnings, ❌ {stats['errors']} errors)"
        output_lines.append(f"  {cat}: {stats['total']} entries{warn_err}")
    
    output_lines.append("\n" + "-" * 80)
    output_lines.append("\nChronological Log:\n")
    
    for entry in entries:
        prefix = ""
        if entry.level == "Warning":
            prefix = "⚠ "
        elif entry.level == "Error":
            prefix = "❌ "
        
        output_lines.append(f"[L{entry.line_number}] [{entry.timestamp}] {entry.category}:")
        output_lines.append(f"    {prefix}{entry.message}\n")
    
    return "\n".join(output_lines)


def main():
    parser = argparse.ArgumentParser(
        description='Parse UE logs for user-generated content (Print Strings, MCP logs)'
    )
    parser.add_argument('logfile', type=Path, help='Path to MCPGameProject.log')
    parser.add_argument(
        '--last-session', '-l',
        action='store_true',
        help='Only show logs from the most recent PIE session'
    )
    parser.add_argument(
        '--category', '-c',
        type=str,
        help='Filter to specific category (e.g., LogBlueprintUserMessages)'
    )
    
    args = parser.parse_args()
    
    entries = parse_log_file(
        args.logfile,
        last_session_only=args.last_session,
        category_filter=args.category
    )
    
    print(format_output(entries))


if __name__ == '__main__':
    main()
