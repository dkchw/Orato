#!/usr/bin/env bash
# ==============================================================================
# Cleanup build and temporary artifacts
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "Cleaning up build artifacts in $REPO_ROOT..."
rm -rf "$REPO_ROOT/build"
rm -rf "$REPO_ROOT/result"
rm -rf "$REPO_ROOT/.cache"
find "$REPO_ROOT" -name "*~" -delete
find "$REPO_ROOT" -name "*.part" -delete
find "$REPO_ROOT" -name "__pycache__" -type d -exec rm -rf {} + 2>/dev/null || true

echo "✓ Build and temporary artifacts cleaned successfully!"
