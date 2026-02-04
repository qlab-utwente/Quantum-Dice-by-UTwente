#!/usr/bin/env bash
set -e

echo "--- Running Clang-Tidy & Clang-Format (ESP32) ---"

if [ ! -f "QuantumDice/build/compile_commands.json" ]; then
    echo "ERROR: compile_commands.json not found for QuantumDice"
    echo "Run Arduino/setup_arduino_esp32.sh [QuantumDice] first."
    exit 1
fi

# -------- Find files --------
FILES=$(git ls-files \
  | grep -E '\.(cpp|cxx|cc|hpp|h|ino)$' \
  | grep -v -E 'build/|ImageLibrary/')

if [ -z "$FILES" ]; then
    echo "No C++ files found."
    exit 0
fi

# -------- Auto-fix --------
echo "Applying clang-tidy auto-fixes..."
echo "$FILES" | xargs -r -P 4 \
  clang-tidy -p "QuantumDice/build" --quiet --fix

CLANG_FORMAT_EXEC = /usr/lib/llvm18/bin/clang-format
if [ -x "$CLANG_FORMAT_EXEC" ]; then
	CLANG_FORMAT="$CLANG_FORMAT_EXEC"
else
	CLANG_FORMAT="clang-format"
fi

echo "Running clang-format..."
echo "$FILES" | xargs -r \
  $CLANG_FORMAT_EXEC -i -style=file

# -------- Report remaining issues --------
echo "--- Reporting remaining issues ---"
sh scripts/lint.sh "QuantumDice"