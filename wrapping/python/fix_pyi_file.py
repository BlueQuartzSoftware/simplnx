import argparse
import uuid

from pathlib import Path

def main() -> None:
    parser = argparse.ArgumentParser(description='Fixes the generated .pyi complex module file')
    parser.add_argument('-i', '--input_file', type=Path, help='Input complex.pyi file to fix')

    args = parser.parse_args()

    input_file_path = f'{args.input_file}'
    file_contents: str
    with open(input_file_path, 'r') as input_file:
        file_contents = input_file.read()
        file_contents = file_contents.replace('from typing import Any, Callable, ClassVar, Iterator, List, Optional, Tuple', 'from typing import Any, Callable, ClassVar, Iterator, List, Optional, Set, Tuple')
        file_contents = file_contents.replace('def load_python_plugin(arg0: module_) -> None: ...', 'def load_python_plugin(arg0: Any) -> None: ...')

    with open(input_file_path, 'w') as cpp_file:
        cpp_file.write(file_contents)


if __name__ == '__main__':
  main()
