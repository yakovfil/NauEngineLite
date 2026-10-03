"""Verify both empty-call overloads terminate with the approved diagnostic."""
import subprocess
import sys

for mode in ("--empty", "--empty-const"):
    result = subprocess.run([*sys.argv[1:], mode], capture_output=True, text=True, timeout=20)
    if result.returncode == 0 or "Nau FixedMoveOnlyFunction invocation requires a callable" not in result.stderr:
        raise SystemExit(f"Unexpected {mode}: {result.returncode}\n{result.stderr}")
print("Both empty move-only overloads rejected with fatal diagnostics")
