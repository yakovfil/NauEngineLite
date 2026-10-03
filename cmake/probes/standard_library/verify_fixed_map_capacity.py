import subprocess, sys
r = subprocess.run([*sys.argv[1:], "--overflow-disabled"], capture_output=True, text=True, timeout=20)
if r.returncode == 0 or "Nau FixedMap capacity exceeded" not in r.stderr:
    raise SystemExit(f"Unexpected result: {r.returncode}\n{r.stderr}")
print("Fixed map overflow rejected with diagnostic")
