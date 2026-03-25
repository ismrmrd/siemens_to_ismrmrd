#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VENV_DIR="$SCRIPT_DIR/.venv"

python3 -m venv "$VENV_DIR"
"$VENV_DIR/bin/pip" install --upgrade pip --quiet
"$VENV_DIR/bin/pip" install -r "$SCRIPT_DIR/requirements.txt"

echo "Virtualenv ready at $VENV_DIR"
echo "Activate with: source $VENV_DIR/bin/activate"
echo "Run tests with: $VENV_DIR/bin/pytest $SCRIPT_DIR"
