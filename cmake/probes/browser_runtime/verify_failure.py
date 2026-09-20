"""Check deliberate failures without mistaking infrastructure errors for success."""
import json
import pathlib
import subprocess
import sys

scenario, build = sys.argv[1:]
run = subprocess.run([sys.executable, str(pathlib.Path(__file__).with_name('run.py')),
    '--scenario', scenario, '--build', build], capture_output=True, text=True)
print(run.stdout)
print(run.stderr, file=sys.stderr)
result = json.loads(run.stdout)
assert run.returncode == 1 and result['runnerCode'] == 1, result
if scenario == 'abort':
    assert 'error' not in result, result
    assert result['code'] == 1 and result['status']['state'] == 'Failed', result
    assert result['status']['phase'] == 'Fatal abort' and not result['status']['cleanupComplete'], result
else:
    assert result['code'] == 2 and result['status']['state'] == 'Stopped', result
    assert result['status']['cleanupComplete'] and 'error' in result, result
