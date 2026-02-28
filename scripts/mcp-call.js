#!/usr/bin/env node
// Usage: node scripts/mcp-call.js <server> <tool> @<json_file>
//    or: node scripts/mcp-call.js <server> <tool> key=value key2=value2
// For array params, use a JSON file: node scripts/mcp-call.js server tool @args.json
const { spawnSync } = require('child_process');
const fs = require('fs');
const path = require('path');

const [,, server, tool, ...rest] = process.argv;
if (!server || !tool) {
  console.error('Usage: node scripts/mcp-call.js <server> <tool> @<json_file> | key=val ...');
  process.exit(1);
}

let argsJson;
if (rest.length === 1 && (rest[0].startsWith('@') || rest[0].startsWith('file:'))) {
  // Read from file
  const filePath = rest[0].replace(/^(file:|@)/, '');
  argsJson = fs.readFileSync(filePath, 'utf8').trim();
} else if (rest.length > 0) {
  // key=value pairs (no arrays support)
  const obj = {};
  for (const arg of rest) {
    const eq = arg.indexOf('=');
    if (eq > 0) obj[arg.slice(0, eq)] = arg.slice(eq + 1);
  }
  argsJson = JSON.stringify(obj);
} else {
  // Read from stdin
  argsJson = fs.readFileSync(0, 'utf8').trim();
}

const mcporterCli = path.join(process.env.APPDATA || '', 'npm', 'node_modules', 'mcporter', 'dist', 'cli.js');
const result = spawnSync('node', [mcporterCli, 'call', server, tool, '--args', argsJson], {
  encoding: 'utf8',
  maxBuffer: 10 * 1024 * 1024,
  timeout: 60000
});
if (result.stdout) process.stdout.write(result.stdout);
if (result.stderr) process.stderr.write(result.stderr);
process.exit(result.status || 0);
