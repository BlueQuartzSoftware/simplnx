import argparse

from pathlib import Path

def main() -> None:
    parser = argparse.ArgumentParser(description='Fixes the generated .pyi simplnx module file')
    parser.add_argument('-i', '--input_file', type=Path, help='Input simplnx.pyi file to fix')

    args = parser.parse_args()

    input_file_path: Path = args.input_file
    with open(input_file_path, 'r+') as input_file:
        file_contents = input_file.read()
        file_contents = file_contents.replace('from typing import Any, Callable, ClassVar, Iterator, List, Optional, Tuple', 'from typing import Any, Callable, ClassVar, Iterator, List, Optional, Set, Tuple')
        file_contents = file_contents.replace('def load_python_plugin(arg0: module_) -> None: ...', 'def load_python_plugin(arg0: Any) -> None: ...')
        input_file.seek(0)
        input_file.write(file_contents)

if __name__ == '__main__':
    main()
