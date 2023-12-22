<!-- The text within this markup is a comment, and is intended to provide
guidelines to open a Pull Request for the simplnx repository. This text will not
be part of the Pull Request. -->


<!-- See the CONTRIBUTING (CONTRIBUTING.md) guide. Specifically:

Start simplnx commit messages with a standard prefix (and a space):

 * FILTER: When adding a new filter to code 
 * BUG: fix for runtime crash or incorrect result
 * COMP: compiler error or warning fix
 * DOC: documentation change
 * ENH: new functionality
 * PERF: performance improvement
 * STYLE: no logic impact (indentation, comments)
 * WIP: Work In Progress not ready for merge


Provide a short, meaningful message that describes the change you made.

When the PR is based on a single commit, the commit message is usually left as
the PR message.

A reference to a related issue or pull request (https://help.github.com/articles/basic-writing-and-formatting-syntax/#referencing-issues-and-pull-requests)
in your repository. You can automatically
close a related issues using keywords (https://help.github.com/articles/closing-issues-using-keywords/)

@mentions (https://help.github.com/articles/basic-writing-and-formatting-syntax/#mentioning-people-and-teams)
of the person or team responsible for reviewing proposed changes. -->

## Naming Conventions

Naming of variables should descriptive where needed. Loop Control Variables can use `i` if warranted. Most of these conventions are enforced through the clang-tidy and clang-format configuration files. See the file `simplnx/docs/Code_Style_Guide.md` for a more in depth explanation.


## Filter Checklist

The help file `simplnx/docs/Porting_Filters.md` has documentation to help you port or write new filters. At the top is a nice checklist of items that should be noted when porting a filter.


## Unit Testing

The idea of unit testing is to test the filter for proper execution and error handling. How many variations on a unit test each filter needs is entirely dependent on what the filter is doing. Generally, the variations can fall into a few categories:

- [ ] 1 Unit test to test output from the filter against known exemplar set of data
- [ ] 1 Unit test to test invalid input code paths that are specific to a filter. Don't test that a DataPath does not exist since that test is already performed as part of the SelectDataArrayAction.

## Code Cleanup
- [ ] No commented out code (rare exceptions to this is allowed..)
- [ ] No API changes were made (or the changes have been approved)
- [ ] No major design changes were made (or the changes have been approved)
- [ ] Added test (or behavior not changed)
- [ ] Updated API documentation (or API not changed)
- [ ] Added license to new files (if any)
- [ ] Added example pipelines that use the filter
- [ ] Classes and methods are properly documented


<!-- **Thanks for contributing to simplnx!** -->
