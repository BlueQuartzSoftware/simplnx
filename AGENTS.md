# AGENTS.md

Guidance for every coding agent working in this repository. Coding rules live in the `bluequartz-skills` skills
below; this file holds what a skill cannot know about this checkout.

## Required skills

Every agent that writes, changes or reviews C++ here (the main session and each subagent) first loads these
`bluequartz-skills` skills and applies them. A subagent does not inherit them: it loads them itself, and every
dispatch prompt says so.

- `bluequartz-skills:cpp-best-practices`: all C++.
- `bluequartz-skills:simplnx-best-practices`: simplnx and simplnx plugin code.

Task skills own the details: `filter-implementation` for filters and algorithms, `implement-filter-tests` and
`exemplar-testing` for tests, `publish-exemplars` for test-data archives, `error-messages` for error and warning
messages, `thread-safety` for parallel code, `ooc-architecture` for out-of-core storage, `pipeline-files` for
`.d3dpipeline` files, and `github-workflow` for commits and pull requests.

## Where new rules go

When work teaches something a later agent or developer needs, record it in the same change, in exactly one place.
Search all of them first and update an existing rule instead of adding a second one.

- `docs/Coding_Knowledge_Base.md`: a lesson from a real defect in simplnx or its plugins. State the rule, the evidence
  (what broke and the pull request or issue that fixed it) and how to recognize it next time, using the entry format
  in that file.
- `docs/Code_Style_Guide.md`: a convention of this project that well-written C++ could still break (names, layout,
  project APIs, filter and test structure). No defect history is needed.
- This repository is public: an entry may name a private repository but contains no code from it.
- A `bluequartz-skills` skill: a general C++ rule that applies to every BlueQuartz repository
  (`cpp-best-practices`), or a procedure (how to implement, test, review or publish). Skills point to the two
  documents above; they do not copy their rules. Skill changes go to the `bluequartz-skills` plugin repository, not
  to this one.
- This file: only facts about this checkout that no skill can know, such as layout and build and test commands.
- A rule that is true only on your own machine (tool paths, presets, build directories, personal habits): your personal
  `~/.codex/AGENTS.md`, in a section for this project headed by its checkout path. Never in this file or in an
  `AGENTS.override.md` here: the override replaces this file instead of adding to it.

## Project

simplnx is the C++20 data-processing library behind DREAM3D-NX. It has no UI dependencies.

- `src/simplnx/`: the core library (DataStructure, filters, parameters, pipelines, utilities).
- `src/Plugins/<Plugin>/`: in-tree filter plugins. Each holds its filters, `Filters/Algorithms/`, `docs/` (one Markdown
  page per filter) and `test/`.
- `src/nxrunner/`: the command-line pipeline runner. `test/`: core library tests. `docs/`: developer documentation,
  including `docs/Code_Style_Guide.md`.
- Out-of-tree filter plugins, such as SimplnxReview, live in their own repositories and join the build through
  `SIMPLNX_PLUGIN_SEARCH_DIRS` and `SIMPLNX_EXTRA_PLUGINS`. These variables do not enable SimplnxOoc. When you need
  out-of-core storage, follow the build instructions of the SimplnxOoc repository.
- Leave `scripts/`, `conda/` and build trees alone unless the task needs them.
- DREAM3D-NX builds this checkout as a subdirectory, so a change here can break the application build.

## Build and test

- Configure with a preset from `CMakePresets.json` (CI) or your `CMakeUserPresets.json`. Use the build directory whose
  cache points at this source tree; a DREAM3D-NX build that includes this checkout counts.
- With `SIMPLNX_DOWNLOAD_TEST_FILES=ON`, build the `Fetch_Remote_Data_Files` target to download test data. The target
  does not exist when that option is OFF.
- With a multi-config generator (Visual Studio), pass `--config <Release|Debug>` to `cmake --build` and
  `-C <Release|Debug>` to ctest.
- After you add or remove an entry in a CMake source list, reconfigure the build directory. This can rerun vcpkg, so run
  it in the foreground with a long timeout. Treat a vcpkg dependency failure as a problem of the environment until you
  show otherwise.
- Never pipe a build or test command into `tail` or another command that hides its exit status.
- Run tests only through `ctest`, never the test executable: ctest extracts and removes the test data. Never run tests
  in parallel; they share data files. Replace `<build-dir>` with the verified build directory and `<pattern>` with the
  test-name regular expression:
  `ctest --test-dir "<build-dir>" -R "<pattern>" -j 1 --output-on-failure --timeout 1500 --no-tests=error`.
- `SIMPLNX_TEST_ALGORITHM_PATH` picks which algorithm paths the tests run (0 both, 1 out-of-core only, 2 in-core only).
  Validate in-core and out-of-core together with `bluequartz-skills:dual-build-protocol`. When out-of-core behavior
  changes, run the whole `[ooc]` set, not one narrow tag.
- Format only the files you changed, with clang-format 19.1.1 from inside the repository so `.clang-format` applies
  (`bluequartz-skills:clang-format` finds the binary). A whole-tree format belongs in its own commit.

## Repository

- Pull requests target `develop` on `BlueQuartzSoftware/simplnx`.
- This repository is public. Issues, pull requests, comments and reviews may name or link private repositories
  (SimplnxOoc, DREAM3D-NX) but contain no code from them.
- A published test-data archive is immutable once merged code references it; publish changed data under a new
  versioned name.
