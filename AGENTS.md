# AGENTS.md

Guidance for every coding agent working in this repository. The coding rules are in the `bluequartz-skills` skills
below and in the two documents they point to, the code style guide and the coding knowledge base. This file holds what
they cannot know about this checkout.

## Required skills

Every agent that writes, changes or reviews code or project files here (C++, CMake, filter documentation, pipeline
files), the main session and each subagent, first loads these `bluequartz-skills` skills and applies them. A subagent
does not inherit them: it loads them itself, and every dispatch prompt says so.

- `bluequartz-skills:cpp-best-practices`
- `bluequartz-skills:simplnx-best-practices`

Also load every task skill whose description matches the work that the change contains, for example
`filter-implementation` for filters and algorithms, `implement-filter-tests` and `exemplar-testing` for tests,
`publish-exemplars` for test-data archives, `error-messages` for error and warning messages, `thread-safety` for
parallel code, `ooc-architecture` for out-of-core storage, `pipeline-files` for `.d3dpipeline` files, and
`github-workflow` for commits and pull requests. The plugin's skill list gives each skill's description.

The skills, DREAM3D-NX and SimplnxOoc are private BlueQuartz repositories. An agent without access to them follows
[the code style guide](docs/Code_Style_Guide.md) and [the coding knowledge base](docs/Coding_Knowledge_Base.md), which
hold the rules of this project, and runs the in-core build and tests. The pull request lists what it could not do: the
skills it could not load and the out-of-core validation it could not run. A maintainer completes that validation.

## Where new rules go

Record a rule when a reviewer or the user had to correct something that no document or skill covers, or when a defect
came from it. Record it in exactly one place. Search all of them first and
update an existing rule instead of adding a second one.

- `docs/Coding_Knowledge_Base.md`: a lesson from a real defect in simplnx or its plugins. State the rule, the evidence
  (what broke and the pull request or issue that fixed it) and how to recognize it next time, using the entry format
  in that file. When a rule comes from a defect, it goes here even if it is also a convention.
- `docs/Code_Style_Guide.md`: a convention of this project that well-written C++ could still break (names, layout,
  project APIs, filter and test structure), with no defect behind it.
- A `bluequartz-skills` skill: a general C++ rule that applies to every BlueQuartz repository
  (`cpp-best-practices`), or a procedure (how to implement, test, review or publish). Skills point to the two
  documents above; they do not copy their rules. Skill changes go to the `bluequartz-skills` plugin repository, so
  they cannot be part of a change here: open a pull request or an issue there and link it from this pull request.
  Without access to that repository, describe the proposed skill change in the pull request description.
- This file: only facts about this checkout that no skill can know, such as layout and build and test commands.
- A rule that is true only on your own machine (tool paths, presets, build directories, personal habits): your agent's
  personal instruction file, in a section for this project headed by its checkout path. Never in this file or in an
  `AGENTS.override.md` here: the override replaces this file instead of adding to it.

The documents in this repository follow the public-repository rule under "Repository".

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
- Do not edit files in `scripts/` or `conda/` unless the user asks for it.
- DREAM3D-NX builds this checkout as a subdirectory, so a change here can break the application build.

## Build and test

- Use a build directory whose `CMakeCache.txt` points at this checkout: `CMAKE_HOME_DIRECTORY` in a simplnx build,
  `simplnx_SOURCE_DIR` in a DREAM3D-NX build. In-core work can use either; when both exist, use the simplnx build.
  Out-of-core tests need a DREAM3D-NX build with `DREAM3DNX_USE_OOC=ON`; there is no simplnx-only out-of-core build.
  When no in-core build exists, configure one with a preset that sets `SIMPLNX_BUILD_TESTS=ON` (the `ci-<platform>`
  presets in `CMakePresets.json` do, and build in `build/`). For an out-of-core build, follow the build instructions
  of the DREAM3D-NX and SimplnxOoc repositories. Configure, build and test only in a build directory that you created
  for this task or that the user named; never delete or clean one.
- A normal build downloads the test data. With `SIMPLNX_DOWNLOAD_TEST_FILES=ON` (the default), the
  `Fetch_Remote_Data_Files` target is part of the default build and every plugin unit-test target depends on it; you do
  not build it separately. When the option is OFF, the target does not exist and tests that need downloaded data
  cannot run.
- With a multi-config generator (Visual Studio), pass `--config <Release|Debug>` to `cmake --build` and
  `-C <Release|Debug>` to ctest.
- After you add or remove an entry in a CMake source list, reconfigure the build directory. This can rerun vcpkg, so run
  it in the foreground with a long timeout. When vcpkg fails to build a dependency, stop and report the package and the
  path of its log; do not change `vcpkg.json`, the triplet or the toolchain to work around it unless the user asks.
- Never pipe a build or test command into `tail` or another command that hides its exit status.
- Run tests only through `ctest`, never the test executable: ctest extracts and removes the test data. Never run tests
  in parallel; they share data files. Replace `<build-dir>` with the verified build directory and `<pattern>` with the
  test-name regular expression:
  `ctest --test-dir "<build-dir>" -R "<pattern>" -j 1 --output-on-failure --timeout 1500 --no-tests=error`.
- Before you hand off a change, build it and run, in a Release build, the tests of every filter, algorithm or class that
  it changes, and the tests of the code that calls a changed base class, store or shared utility. Name the tests that
  ran in the pull request.
- `SIMPLNX_TEST_ALGORITHM_PATH` picks which algorithm paths the tests run (0 both, 1 out-of-core only, 2 in-core only).
  It is a CMake cache variable compiled into the test targets: set it when you configure
  (`-DSIMPLNX_TEST_ALGORITHM_PATH=1`) and rebuild the tests. Setting it in the environment when you run ctest does
  nothing.
- Validate in-core and out-of-core together with `bluequartz-skills:dual-build-protocol` for every change to code that
  runs when a filter executes or a store is read or written: a new or changed filter or algorithm (with or without
  `AlgorithmDispatch`), a utility that reads or writes arrays, or a store or I/O class. A change to documentation,
  comments or build scripts only does not need it.
- When out-of-core behavior changes, run every out-of-core test except the benchmarks, in the out-of-core build, not
  one narrow tag:
  `ctest --test-dir "<build-dir>" -L "^ooc$" -LE "benchmark" -j 1 --output-on-failure --timeout 1500 --no-tests=error`.
  The `ooc` label covers every test case tagged `[OOC]` (Catch2 tags ignore case), the `<Plugin>OocStoreContracts`
  entries, which run the hidden `[.OocStoreContract]` cases, and the out-of-core tests that the SimplnxOoc build adds.
  Other hidden cases are not registered with ctest.
- Format only the files you changed, with clang-format 19.1.1 from inside the repository so `.clang-format` applies
  (`bluequartz-skills:clang-format` finds the binary). A whole-tree format belongs in its own commit.

## Repository

- Pull requests target `develop` on `BlueQuartzSoftware/simplnx`.
- This repository is public. Its files, issues, pull requests, comments and reviews may name or link private
  repositories (SimplnxOoc, DREAM3D-NX) but contain no code from them.
- A published test-data archive is immutable once merged code references it; publish changed data under a new
  versioned name.
