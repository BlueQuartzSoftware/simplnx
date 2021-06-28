import argparse
import re
import subprocess

from typing import List, Optional, Tuple

EXE = 'clang-format'
FILE_TYPES = (
  '.h',
  '.H',
  '.hpp',
  '.hh',
  '.h++',
  '.hxx',
  '.c',
  '.C',
  '.cpp',
  '.cc',
  '.c++',
  '.cxx',
)

def filter_files(files: List[str]) -> List[str]:
  return [file for file in files if file.endswith(FILE_TYPES)]

def get_git_files() -> List[str]:
  try:
    result = subprocess.run(['git', 'ls-files'], text=True, capture_output=True, check=True)
  except subprocess.CalledProcessError as error:
    print(error.stderr)
    raise
  git_files = result.stdout.splitlines()
  return filter_files(git_files)

def get_git_diff_files(previous_commit: str, current_commit: str) -> List[str]:
  try:
    result = subprocess.run(
      ['git', 'diff', '--name-only', '--diff-filter=d', f'{previous_commit}...{current_commit}'],
      text=True,
      capture_output=True,
      check=True
    )
  except subprocess.CalledProcessError as error:
    print(error.stderr)
    raise
  git_files = result.stdout.splitlines()
  return filter_files(git_files)

def format(file: str, should_modify: bool, version: Optional[int] = None) -> int:
  exe = EXE
  if version:
    exe = f'{EXE}-{version}'
  command = [exe, '--style=file']
  if should_modify:
    command.append('-i')
  else:
    command.extend(['--dry-run', '--Werror'])
  command.append(file)
  result = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
  for line in result.stdout.splitlines():
    print(line, flush=True)
  return result.returncode

def get_clang_format_version() -> Tuple[int, int, int]:
  try:
    version_result = subprocess.run([EXE, '--version'], text=True, capture_output=True, check=True)
  except subprocess.CalledProcessError as error:
    print(error.stderr)
    raise
  version_string = version_result.stdout
  match = re.match(r'clang-format version (\d+)\.(\d+)\.(\d+)', version_string)
  major = int(match.group(1))
  minor = int(match.group(2))
  patch = int(match.group(3))
  return (major, minor, patch)

def main() -> int:
  parser = argparse.ArgumentParser(description='Run clang-format on files in a git repo')
  parser.add_argument('--modify', help='Apply clang-format modifications in place', action='store_true')
  parser.add_argument('--commits', help='Check formatting of diff between two commits instead of all files (previous, current)', nargs=2)
  parser.add_argument('--format-version', help='clang-format version', type=int)

  args = parser.parse_args()

  # clang-format must be at least major version 10 since that's when --dry-run and --Werror were introduced
  if args.format_version:
    if args.format_version < 10:
      parser.error('format-version must be at least 10')
  else:
    version = get_clang_format_version()
    if version[0] < 10:
      raise RuntimeError('clang-format must be at least major version 10')

  if args.commits:
    files = get_git_diff_files(args.commits[0], args.commits[1])
  else:
    files = get_git_files()

  if not files:
    print('No files to format')

  failed_files: List[str] = []

  result = 0
  for file in files:
    format_result = format(file, args.modify, args.format_version)
    if format_result != 0:
      failed_files.append(file)
      result = 1

  if result == 0:
    print('All files were formatted correctly')
  else:
    print('Incorrectly formatted files:')
    for file in failed_files:
      print(f'\"{file}\"')

  return result

if __name__ == '__main__':
  exit(main())
