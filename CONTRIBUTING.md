# Contributing to complex

Thank you for considering contributing to complex. Your involvement helps make complex a valuable tool for the community. There are lots of

## Building from Source

For instructions on building from source see our [Guide to Building From Source](/docs/Build_From_Source.md).

## Getting Started

1. Fork the repository to your GitHub account.
2. Clone the repository to your local machine: `git clone https://github.com/YOUR_USERNAME/complex.git`
3. Create a new branch: `git checkout -b NAME_YOUR_BRANCH`
4. Make your changes and commit them: `git commit -m 'Add some feature or fix some issue'`
5. Push your changes to your fork: `git push origin NAME_YOUR_BRANCH`
6. Create a pull request from your branch to the complex `main` or `master` branch.

## Code of Conduct

By participating in this project, you are expected to uphold our [Code of Conduct](/CODE_OF_CONDUCT.md).

If you experience or witness unacceptable behavior—or have any other concerns—please report it by contacting the project maintainers at <info@bluequartz.net>. All reports will be handled with discretion.

## How Can I Contribute?

### Reporting Bugs

1. Check the [issue tracker](https://github.com/bluequartzsoftware/complex/issues) to ensure the bug hasn't already been reported.
2. If the bug hasn't been reported, create a new issue. Provide a clear description of the problem, including steps to reproduce.

### Suggesting Enhancements

1. Check the [issue tracker](https://github.com/bluequartzsoftware/complex/issues) to ensure the enhancement hasn't already been suggested.
2. If not, create a new issue. Provide a clear description of the enhancement.

### Handling Issues with Help Wanted Tag

1. Check the [issue tracker](https://github.com/bluequartzsoftware/complex/issues) and sort by Help Wanted to see issues that aren't actively being handled internally.
2. Once you select an issue, check for existing comments containing an "In Progress" title.
3. If there is not an exitisting _In Progress_ comment proceed to step 4, else do the following:
   - Check the _Description of Planned Changes_ in the comment to see if the part of they are already handling the change you want to make. If not continue to step 4
   - Look at _Extra Information_ to see if they are willing to accept help. If not select another change associated with the issue (if it has multiple tasks) or go select a new issue to work on.
   - Add a comment to the issue where you ping the collaborator with @ and their username and ask if you can help. Be sure to be polite and abide by our [Code of Conduct](/CODE_OF_CONDUCT.md).
4. If theres not an existing comment, go a head and create a branch and add a comment to the issue formatted as follows:

``` markdown
## In Progress

### Description of Planned Changes

Example Description:
- Fix example bug
- Adding example feature
- Porting exampleModule documentation to example language
- Handling example task from checklist above

### Working Branch

[MyExampleFork/branch_type/branch_name](https://github.com/profileName/MyExampleFork/tree/branch_type/branch_name)

### Extra Information

Willing to Collaborate with Other Contributors on These Changes: Y/N

[Any other comments you may have]

```

### Pull Requests

1. Ensure any install or build dependencies are removed before the end of the layer when doing a build.
2. Ensure your code adheres to our existing coding standards.
3. Include test cases wherever possible.
4. Describe your changes in the pull request description.

## Styleguides

### Git Commit Messages

- Use the present tense ("Add feature" not "Added feature")
- Use the imperative mood ("Move cursor to..." not "Moves cursor to...")
- Limit the first line to 72 characters or less
- Reference issues and pull requests liberally

### Code Styleguide

For information about our specifc naming conventions and styling see our [coding style guide](/docs/Code_Style_Guide.md).

For the basic code styling complex uses `clang-format`, the formatting file is included at the root level in the repository.

`clang-format` is a tool to automatically format C++ code. You should run 'clang-format' on your code before opening a Pull Request to make sure it passes the _clang-format pr_ check. You can install `clang-format` and `git-clang-format` with `npm install -g clang-format`. To automatically format a file according to a project's C++ code style, run `clang-format -i path/to/complex/file`, which is supported on macOS/Linux/Windows. If you want to run `clang-format` on all the changed code on your latest git commit (HEAD), you can run `git-clang-format HEAD~1`. Run `git-clang-format -h`  for extra help.

### Documentation Styleguide

- Use [Markdown](https://daringfireball.net/projects/markdown/)
- Reference methods and classes in backticks (`MethodName`, `ClassName`)
- Leave a blank newline at the end of file
- Be sure to include the following at the bottom of all documentation files

```markdown
## DREAM3DNX Help ##

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.
```

## Community

For those looking to engage with the DREAM3DNX community, see the discussions board of [DREAM3DNX-Issues discussions](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions). All of our filter documentation now links back to this repository.

## Additional Notes

It is highly recommended for new code contributors to look at the [porting filters guidelines](/docs/Porting_Filters.md) to get a better idea of API and see how to bring older filters over from [SIMPL](https://github.com/BlueQuartzSoftware/SIMPL).

Thank you for contributing!
