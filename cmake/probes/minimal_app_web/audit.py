"""Audit real sample preset graphs, source/link inputs and supported configurations."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from test_minimal_runtime import targets, source_names, samples_root

engine = Path(__file__).resolve().parents[3]
build_root = engine / 'build'
wrapper = engine / 'run_tests.bat'
wrapper_before = wrapper.read_bytes() if wrapper.exists() else None
native_caches = [build_root / name / 'CMakeCache.txt' for name in
                 ('native-minimal-debug', 'native-minimal-release', 'desktop-platform-after')]
native_before = {p: hashlib.sha256(p.read_bytes()).hexdigest() for p in native_caches if p.exists()}
environment = dict(os.environ, VCPKG_ROOT=str(build_root / 'unavailable-vcpkg'),
                   CMAKE_PREFIX_PATH=str(build_root / 'unavailable-desktop-dependencies'))


def run(command):
    return subprocess.run(command, cwd=engine, env=environment, capture_output=True, text=True, timeout=180)


for configuration in ('debug', 'release'):
    preset = 'web-minimal-' + configuration
    build = build_root / preset
    query = build / '.cmake/api/v1/query/codemodel-v2'
    query.parent.mkdir(parents=True, exist_ok=True)
    query.touch()
    configured = run(['cmake', '--preset', preset])
    assert configured.returncode == 0, configured.stdout + configured.stderr
    (build / 'sample-audit-configure.log').write_text(configured.stdout + configured.stderr)
    graph = targets(build)
    assert {'MinimalAppSample', 'NauKernel', 'NauFrameworkMinimal', 'PlatformApp'} <= graph.keys()
    allowed = {'EABase', 'EASTL', 'tiny-utf8', 'fmt', 'utfcpp', 'ModifiedSonyMath', 'jsoncpp', 'wyhash', 'fast_float'}
    for target in graph.values():
        path = target['paths']['source']
        if path.startswith('engine/3rdparty_libs/'):
            assert path.split('/')[2] in allowed, path
        else:
            assert path == '.' or (engine / path).resolve() == samples_root(build) / 'minimalApp' or path.startswith((
                'engine/core/kernel', 'engine/core/app_framework', 'engine/core/modules/platform_app')), path
    sample_sources = source_names(graph['MinimalAppSample'])
    assert 'app_sample_main_min.cpp' in sample_sources and 'app_sample_main.cpp' not in sample_sources
    assert 'generated_static_modules_initialization.cpp' in sample_sources
    registration = (build / 'samples/minimalApp/generated_static_modules_initialization.cpp').read_text()
    assert 'createModule_PlatformApp' in registration
    commands = json.loads((build / 'compile_commands.json').read_text())
    assert sum(Path(c['file']).name == 'app_sample_main_min.cpp' for c in commands) == 1
    assert any(Path(c['file']).resolve() == samples_root(build) / 'minimalApp/app_sample_main_min.cpp' for c in commands)
    forbidden = ('-D_WIN32', '-DWIN32', '-D_WIN64', '-D_M_X64', '-D_TARGET_PC', ' /MD', ' /EH', '-mavx')
    for entry in commands:
        command = entry['command']
        assert '-pthread' in command and not any(flag in command for flag in forbidden), command
        assert '-O0' in command if configuration == 'debug' else '-O3' in command, command
        if Path(entry['file']).suffix in ('.cpp', '.cxx', '.h'):
            assert '-fno-exceptions' in command and '-fno-rtti' in command, command
    fragments = graph['MinimalAppSample']['link']['commandFragments']
    link = ' '.join(f['fragment'] for f in fragments)
    for flag in ('-pthread', '-sPROXY_TO_PTHREAD=1', '-sEXIT_RUNTIME=1', '-sPTHREAD_POOL_SIZE=12', '-sALLOW_BLOCKING_ON_MAIN_THREAD=0'):
        assert flag in link, link
    assert '.lib' not in link and '.dll' not in link, link
    assert all(name in link for name in ('NauFrameworkMinimal', 'PlatformApp', 'NauKernel')), link
    inventory = sorted(p.name for p in (build / 'bin').glob('MinimalAppSample.*'))
    assert 'MinimalAppSample.js' in inventory and 'MinimalAppSample.wasm' in inventory
    summary = {'preset': preset, 'targets': sorted(graph), 'compileCommands': len(commands),
               'sampleSources': sorted(sample_sources), 'link': link, 'runtimeOutputs': inventory}
    (build / 'sample-graph-audit.json').write_text(json.dumps(summary, indent=2))
    print(f'PASS {preset}: {len(graph)} targets, {len(commands)} compile commands, {inventory}')

with tempfile.TemporaryDirectory(prefix='sample-config-contract-', dir=build_root) as temp:
    scratch = Path(temp).resolve()
    assert scratch.is_relative_to(build_root.resolve())
    for name, option, message in (
        ('desktop-profile', '-DNAU_RUNTIME_PROFILE=desktop', 'require a minimal or demo runtime profile'),
        ('shared', '-DBUILD_SHARED_LIBS=ON', 'require static linking')):
        result = run(['cmake', '--preset', 'web-minimal-debug', '-B', str(scratch / name), option])
        output = result.stdout + result.stderr
        assert result.returncode != 0 and message in output, output
        print(f'PASS unsupported browser {name} rejected')

assert (wrapper.read_bytes() if wrapper.exists() else None) == wrapper_before, 'Desktop wrapper changed'
assert all(hashlib.sha256(p.read_bytes()).hexdigest() == digest for p, digest in native_before.items()), 'Native cache changed'
print('PASS desktop wrapper and native caches untouched')
