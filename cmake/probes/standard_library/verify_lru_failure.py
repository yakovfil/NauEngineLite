"""Check zero-capacity access fails before creating a value."""

import subprocess
import sys

for mode in ("--zero-get", "--zero-insert"):
    result = subprocess.run([sys.argv[1], mode], capture_output=True, text=True, timeout=20)
    if (
        result.returncode == 0
        or "Nau LRU cache access requires nonzero capacity" not in result.stderr
        or "CREATOR_CALLED" in result.stderr
    ):
        raise SystemExit(f"Unexpected {mode}: {result.returncode}\n{result.stderr}")
print("Zero-capacity get/insert rejected with fatal diagnostics")
