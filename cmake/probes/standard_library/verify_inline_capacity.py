"""Verify disabled overflow fails before writing outside inline storage."""
import subprocess
import sys

result = subprocess.run([*sys.argv[1:], "--overflow-disabled"], capture_output=True, text=True, timeout=20)
if result.returncode == 0 or "Nau InlineVector capacity exceeded" not in result.stderr:
    raise SystemExit(f"Unexpected capacity failure: {result.returncode}\n{result.stderr}")
print("Inline capacity enforced with fatal diagnostic")
