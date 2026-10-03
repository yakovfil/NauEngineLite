"""Verify allocation failure terminates with the allocator diagnostic."""

import subprocess
import sys

result = subprocess.run(
    [sys.argv[1], "--overflow"], capture_output=True, text=True, timeout=20
)
if result.returncode == 0 or "Nau resource allocation failed" not in result.stderr:
    raise SystemExit(f"Unexpected failure result: {result.returncode}\n{result.stdout}\n{result.stderr}")
print("Overflow request rejected with diagnostic and nonzero exit")
