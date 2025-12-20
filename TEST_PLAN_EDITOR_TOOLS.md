# EDITOR_TOOLS: get_level_metadata Test Plan

This document provides test cases for verifying the `get_level_metadata` consolidation in EDITOR_TOOLS.

---

## Overview

**New Tool:** `get_level_metadata`
**Consolidates:** `get_actors_in_level`, `find_actors_by_name`

---

## Prerequisites

- Have some actors in the current level (e.g., lights, static meshes, cameras)
- Recommended: Create a few actors with recognizable names like "TestLight", "Floor", "PlayerStart"

---

## Test Cases

### Test 1: Get All Actors (Default - No Parameters)
**Tool:** `get_level_metadata`
```json
{}
```
**Expected Response:**
- `success: true`
- `actors` object containing:
  - `count`: number of actors
  - `items`: array of actor objects
- Each actor in `items` has properties like `name`, `class`, `location`

---

### Test 2: Get All Actors (Explicit fields)
**Tool:** `get_level_metadata`
```json
{
  "fields": ["actors"]
}
```
**Expected Response:**
- Same as Test 1
- `success: true`
- `actors.count` and `actors.items` present

---

### Test 3: Get All Actors (Wildcard fields)
**Tool:** `get_level_metadata`
```json
{
  "fields": ["*"]
}
```
**Expected Response:**
- `success: true`
- `actors` object with all level actors

---

### Test 4: Filter Actors by Name Pattern (Contains)
**Tool:** `get_level_metadata`
```json
{
  "actor_filter": "*Light*"
}
```
**Expected Response:**
- `success: true`
- `actors.filter`: "*Light*"
- `actors.items` contains ONLY actors with "Light" in their name
- `actors.count` reflects filtered count

---

### Test 5: Filter Actors by Name Pattern (Prefix)
**Tool:** `get_level_metadata`
```json
{
  "actor_filter": "Floor*"
}
```
**Expected Response:**
- `success: true`
- `actors.items` contains only actors starting with "Floor"

---

### Test 6: Filter Actors by Name Pattern (Suffix)
**Tool:** `get_level_metadata`
```json
{
  "actor_filter": "*Start"
}
```
**Expected Response:**
- `success: true`
- `actors.items` contains only actors ending with "Start" (e.g., "PlayerStart")

---

### Test 7: Combined fields and actor_filter
**Tool:** `get_level_metadata`
```json
{
  "fields": ["actors"],
  "actor_filter": "*Point*"
}
```
**Expected Response:**
- `success: true`
- `actors.filter`: "*Point*"
- Only actors with "Point" in their name

---

### Test 8: No Matching Actors
**Tool:** `get_level_metadata`
```json
{
  "actor_filter": "NonExistentActorXYZ123"
}
```
**Expected Response:**
- `success: true`
- `actors.count`: 0
- `actors.items`: empty array `[]`

---

## Migration Tests (Backward Compatibility)

### Old Tool: get_actors_in_level
**Old Call:**
```json
{}
```
**New Equivalent:**
```json
{
  "fields": ["actors"]
}
```
Both should return the same actor list.

---

### Old Tool: find_actors_by_name
**Old Call:**
```json
{
  "pattern": "*PointLight*"
}
```
**New Equivalent:**
```json
{
  "actor_filter": "*PointLight*"
}
```
Both should return the same filtered actor list.

---

## Quick Smoke Test

Run these in sequence:

1. **Get all actors:**
   ```json
   {"tool": "get_level_metadata", "params": {}}
   ```
   Verify: Returns `success: true` with actors list

2. **Filter by pattern:**
   ```json
   {"tool": "get_level_metadata", "params": {"actor_filter": "*Light*"}}
   ```
   Verify: Returns only light actors

3. **Verify old tools still work:**
   ```json
   {"tool": "get_actors_in_level", "params": {}}
   ```
   Verify: Still returns actors (backward compatibility)

   ```json
   {"tool": "find_actors_by_name", "params": {"pattern": "*Light*"}}
   ```
   Verify: Still returns filtered actors (backward compatibility)

---

## Summary

| Old Tool | New Tool | Migration |
|----------|----------|-----------|
| `get_actors_in_level` | `get_level_metadata` | Use `fields=["actors"]` or no params |
| `find_actors_by_name` | `get_level_metadata` | Use `actor_filter` parameter |
