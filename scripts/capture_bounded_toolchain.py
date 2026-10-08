"""Temporarily capture the native headers used by the bounded-read CI tests."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys
import tarfile


def preprocessing_command(entry):
    """Keep the target's flags while removing compile output and dependency writes."""
    arguments = entry.get('arguments') or shlex.split(entry['command'])
    source = Path(entry['file'])
    if not source.is_absolute():
        source = Path(entry['directory']) / source
    source = source.resolve()
    result = [arguments[0]]
    skip_next = False
    for argument in arguments[1:]:
        if skip_next:
            skip_next = False
            continue
        if argument in ('-o', '-MF', '-MT', '-MQ', '-MJ'):
            skip_next = True
            continue
        if argument in ('-c', '-MD', '-MMD', '-MP', '-MG'):
            continue
        if argument.startswith(('-o', '-MF', '-MT', '-MQ', '-MJ')):
            continue
        candidate = Path(argument)
        if not candidate.is_absolute():
            candidate = Path(entry['directory']) / candidate
        if candidate.resolve() == source:
            continue
        if argument.startswith('@'):
            raise RuntimeError('Response files require explicit review before capture')
        result.append(argument)
    if skip_next:
        raise RuntimeError('Incomplete output/dependency option in compiler command')
    return [*result, '-E', '-dM', '-H', '-v', str(source)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=Path('build'))
    parser.add_argument('--output-dir', type=Path, required=True)
    args = parser.parse_args()
    output = args.output_dir.resolve()
    output.mkdir(parents=True, exist_ok=False)
    entries = json.loads((args.build_dir / 'compile_commands.json').read_text())
    entry = next(item for item in entries if Path(item['file']).name == 'DataStoreBulkTest.cpp')
    command = preprocessing_command(entry)
    (output / 'target-command.json').write_text(json.dumps(entry, indent=2) + '\n')
    (output / 'capture-command.json').write_text(json.dumps(command, indent=2) + '\n')
    result = subprocess.run(command, cwd=entry['directory'], capture_output=True, text=True, timeout=120)
    (output / 'macros.txt').write_text(result.stdout)
    (output / 'include-trace.txt').write_text(result.stderr)
    result.check_returncode()

    # Preserve the consumed standard-library headers, including target-specific
    # c++config.h/__config_site, and the expected-lite header used by Result.
    headers = set()
    for line in result.stderr.splitlines():
        match = re.match(r'^\.+ (.+?)(?: \(.*\))?$', line)
        if match is None:
            continue
        header = Path(match.group(1))
        if not header.is_absolute():
            header = Path(entry['directory']) / header
        header = header.resolve()
        if header.is_file() and ('c++' in header.parts or header.name == 'expected.hpp'):
            headers.add(header)
    if not any(header.name == 'vector' for header in headers):
        raise RuntimeError('The include trace did not identify a standard-library vector header')
    manifest = []
    with tarfile.open(output / 'headers.tar.gz', 'w:gz') as archive:
        for index, header in enumerate(sorted(headers)):
            member = f'headers/{index:04d}/{header.name}'
            archive.add(header, arcname=member, recursive=False)
            manifest.append({'source': str(header), 'member': member,
                             'sha256': hashlib.sha256(header.read_bytes()).hexdigest()})
    (output / 'headers.json').write_text(json.dumps(manifest, indent=2) + '\n')

    # Discover the built test binary through CTest rather than assuming its path.
    discovery = subprocess.run(['ctest', '--test-dir', str(args.build_dir.resolve()),
                                '-R', '^Bounded terminal representation requires reviewed native controls$',
                                '--show-only=json-v1'], capture_output=True, text=True, timeout=60)
    (output / 'test-discovery.json').write_text(discovery.stdout)
    (output / 'test-discovery-errors.txt').write_text(discovery.stderr)
    discovery.check_returncode()
    tests = json.loads(discovery.stdout)['tests']
    if len(tests) != 1:
        raise RuntimeError('Expected exactly one native representation test')
    executable = Path(tests[0]['command'][0]).resolve()
    if not executable.is_file():
        raise RuntimeError(f'The test executable is missing: {executable}')
    runtime_command = ['otool', '-L', str(executable)] if sys.platform == 'darwin' else ['ldd', str(executable)]
    runtime = subprocess.run(runtime_command, capture_output=True, text=True, timeout=30)
    runtime_record = {'test_executable': str(executable), 'command': runtime_command,
                      'exit_code': runtime.returncode, 'stdout': runtime.stdout, 'stderr': runtime.stderr}
    runtime_paths = re.findall(r'^\s*libstdc\+\+[^\s]*\s+=>\s+(\S+)', runtime.stdout, re.MULTILINE)
    runtime_record['resolved_libstdcxx'] = []
    for runtime_path in runtime_paths:
        library = Path(runtime_path).resolve()
        if library.is_file():
            runtime_record['resolved_libstdcxx'].append({
                'path': str(library), 'sha256': hashlib.sha256(library.read_bytes()).hexdigest()})
    # macOS system libc++ can live in the dyld shared cache. Record its load name
    # and OS build through otool/sw_vers without claiming a standalone file hash.
    (output / 'runtime.json').write_text(json.dumps(runtime_record, indent=2) + '\n')
    runtime.check_returncode()

    version_commands = [[command[0], '--version'], ['uname', '-a'], ['sw_vers'],
                        ['xcode-select', '-p'], ['xcodebuild', '-version'],
                        ['xcrun', '--sdk', 'macosx', '--show-sdk-path']]
    release = re.search(r'^#define _GLIBCXX_RELEASE (\d+)$', result.stdout, re.MULTILINE)
    if release is not None:
        version_commands.append(['dpkg-query', '-W', 'libstdc++6', f'libstdc++-{release.group(1)}-dev'])
        for library in runtime_record['resolved_libstdcxx']:
            version_commands.append(['dpkg-query', '-S', library['path']])
    versions = []
    for version_command in version_commands:
        if shutil.which(version_command[0]):
            version = subprocess.run(version_command, capture_output=True, text=True, timeout=15)
            versions.append({'command': version_command, 'exit_code': version.returncode,
                             'stdout': version.stdout, 'stderr': version.stderr})
    (output / 'versions.json').write_text(json.dumps(versions, indent=2) + '\n')
    names = ('_LIBCPP_VERSION', '_LIBCPP_ABI_VERSION', '_GLIBCXX_RELEASE', '__GLIBCXX__',
             '_GLIBCXX_USE_CXX11_ABI', '__apple_build_version__', '__cplusplus',
             'expected_lite_MAJOR', 'expected_lite_MINOR', 'expected_lite_PATCH', 'nsel_USES_STD_EXPECTED')
    for line in result.stdout.splitlines():
        if any(line.startswith('#define ' + name + ' ') for name in names):
            print(line)
    print(f'Captured {len(headers)} consumed library headers in {output}')


if __name__ == '__main__':
    main()
