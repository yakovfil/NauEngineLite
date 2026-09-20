"""Require observable diagnostics and a failing exit from the real Wasm process."""
import subprocess
import sys

result = subprocess.run(sys.argv[1:], capture_output=True, text=True, timeout=30)
output = result.stdout + result.stderr
print(output)
if result.returncode == 0 or "Nau failure:" not in output or "core fatal fixture" not in output:
    sys.exit("Expected a nonzero fatal diagnostic exit")
