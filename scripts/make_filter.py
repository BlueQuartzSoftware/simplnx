import argparse
from pathlib import Path
import uuid


def make_filter(output_dir: Path, name: str, template_dir:Path):
    header_template_file = str(template_dir) + "/filter.hpp.in"
    header_file_contents = ''
    with open (header_template_file, "r") as header_file:
        header_file_contents = header_file.read().replace('@FILTER_NAME@', name).replace('@UUID@', str(uuid.uuid4()))

    # write contents out to the new target header file
    header_target_file = str(output_dir) + "/" + name + ".hpp"
    print(f'Writing Header file: {header_target_file}')
    with open (header_target_file, "wt") as header_file:
        header_file.write(header_file_contents)


    cpp_template_file = str(template_dir) + "/filter.cpp.in"
    cpp_file_contents = ''
    with open (cpp_template_file, "r") as cpp_file:
        cpp_file_contents = cpp_file.read().replace('@FILTER_NAME@', name)


    # write contents out to the new target header file
    cpp_target_file = str(output_dir) + "/" + name + ".cpp"
    print(f'Writing CPP file:    {cpp_target_file}')
    with open (cpp_target_file, "wt") as cpp_file:
        cpp_file.write(cpp_file_contents)

    print(f'===================================================================')
    print(f'Do NOT forget to add your filter to the appropriate CMake Files')
    print(f'===================================================================')


def main() -> None:
    parser = argparse.ArgumentParser(description='Creates complex filter header and implementation skeleton codes')
    parser.add_argument('-o', '--output_dir', type=Path, help='Input directory where files will be created')
    parser.add_argument('-n', '--name', type=str, help='Name of filter')
    parser.add_argument('-t', '--template_dir', type=Path, help='Location of template files')

    args = parser.parse_args()


    print('args:')
    print(f'  output_dir = \"{args.output_dir}\"')
    print(f'  name = \"{args.name}\"')
    print(f'  template_dir = \"{args.template_dir}\"')
    print('')

    print(f'===================================================================')
    print(f'        THIS WILL OVERWRITE ANY EXISTING FILE')
    print(f'===================================================================')
    make_filter(args.output_dir, args.name, args.template_dir)



if __name__ == '__main__':
    main()
