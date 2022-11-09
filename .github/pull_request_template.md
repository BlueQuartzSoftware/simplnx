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


## Naming Conventions

Naming of variables should descriptive where needed. Loop Control Variables can use `i` if warranted. Most of these conventions are enforced through the clang-tidy and clang-format configuration files.

- [ ] Class and Structs are `UpperCamelCase`
- [ ] Class private member variables are `m_UpperCamelCase`
- [ ] Class methods are `lowerCamelCase`
- [ ] Method arguments are `lowerCamelCase`
- [ ] Normal variables are `lowerCamelCase`
- [ ] Constants are `k_UpperCamelCase`
- [ ] Global statics are `s_UpperCamelCase`
- [ ] Free Functions are `UpperCamelCase`
- [ ] Macros are `ALL_UPPER_SNAKE_CASE`
- [ ] Unit test will test data output from the filter.
- [ ] Reused strings should be constants in an anonymous namespace
- [ ] If parallelization is used, proper use of the abstracted complex classes are used. Using TBB specifically in a filter should be frowned upon unless for a really good reason.

## Filter Checklist

- [ ] Parameters should be generally broken down into "Comments", "Input Parameters", "Required Data Objects", "Created Data Objects". There can be exceptions to this.
- [ ] ChoicesParameter selections should be an enumeration defined in the filer header
- [ ] Documentation copied from SIMPL Repo and updated (if necessary)
- [ ] Parameter argument variables are k_CamelCase_Key
- [ ] Parameter argument strings are lower_snake_case
- [ ] CommentParameter placed underneath the Comments separator as the first parameter
```
static inline constexpr StringLiteral k_AlignmentType_Key = "alignment_type";
```

## Unit Testing
- [ ] 1 Unit test to instantiate the filter with the default arguments.
- [ ] 1 Unit test to test output from the filter against known exemplar set of data
- [ ] 1 Unit test to test invalid input code paths that are specific to a filter. Don't test that a DataPath does not exist since that test is already performed as part of the SelectDataArrayAction.

## Code Cleanup
- [ ] No commented out code (rare exceptions to this is allowed..)
- [ ] Filters should have both the Filter class and Algorithm class for anything beyond trivial needs
- [ ] No API changes were made (or the changes have been approved)
- [ ] No major design changes were made (or the changes have been approved)
- [ ] Added test (or behavior not changed)
- [ ] Updated API documentation (or API not changed)
- [ ] Added license to new files (if any)
- [ ] Added Python wrapping to new files (if any) as necessary
- [ ] Added example pipelines that use the filter
- [ ] Classes and methods are properly documented.


<!-- **Thanks for contributing to complex!** -->
