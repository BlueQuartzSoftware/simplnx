<!-- The text within this markup is a comment, and is intended to provide
guidelines to open a Pull Request for the complex repository. This text will not
be part of the Pull Request. -->


<!-- See the CONTRIBUTING (CONTRIBUTING.md) guide. Specifically:

Start complex commit messages with a standard prefix (and a space):

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

## PR Checklist

- [ ] Documentation copied from SIMPL Repo and updated (if necessary)
- [ ] Parameter argument strings are snake_case
```
static inline constexpr StringLiteral k_AlignmentType_Key = "alignment_type";
```
- [ ] Naming conventions for classes/structs/variables and such are being followed
  - [ ] Class and Structs are `UpperCamelCase`
  - [ ] Class private member variables are `m_UpperCamelCase`
  - [ ] Class methods are `lowerCamelCase`
  - [ ] Method arguments are `lowerCamelCase`
  - [ ] Constants are `k_UpperCamelCase`
  - [ ] Global statics are `s_UpperCamelCase`
  - [ ] Macros are `ALL_UPPER_SNAKE_CASE`
- [ ] Unit test will tests data output from the filter.
- [ ] If parallelization is used, proper use of the abstracted complex classes are used. Using TBB specifically in a filter should be frowned upon unless for a really good reason.
- [ ] No commented out code (rare exceptions to this is allowed..)
- [ ] Filters should have both the Filter class and Algorithm class for anything beyond trivial needs
- [ ] No API changes were made (or the changes have been approved)
- [ ] No major design changes were made (or the changes have been approved)
- [ ] Added test (or behavior not changed)
- [ ] Updated API documentation (or API not changed)
- [ ] Added license to new files (if any)
- [ ] Added Python wrapping to new files (if any) as necessary
- [ ] Added example pipelines that use the filter


<!-- **Thanks for contributing to complex!** -->
