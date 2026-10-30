# V&V Recertification Policy

This document says when V&V work happens after a filter has a V&V record, and how that work is organized. It applies to every filter that has a report in `src/Plugins/<Plugin>/vv/`. Read [`vv_policy.md`](./vv_policy.md) first; it defines what a V&V record contains.

Each rule has a status:

- **Agreed** — follow it.
- **Proposed** — the team has not decided yet. Follow it as the default, and raise any problem with it.

## 1. What triggers V&V work (Proposed)

The trigger is what a change does to the filter's output, not the fact that the change touches the filter's code.

| Class | Typical changes | V&V action in the same PR |
|---|---|---|
| **A. Output unchanged for valid input** | Refactoring, out-of-core storage changes, performance work, messages and progress, a new check that rejects invalid input | None. The filter's existing V&V tests must pass **with no change to their expected values, exemplars, or tolerances**. That result is the evidence. |
| **B. Output changes for valid input** | A bug fix that changes values, a precision change (for example float32 to float64 accumulation), a changed default | A scoped V&V update: add the new tests to the report's test inventory, add a Part 2 deviation entry, derive any changed expected value independently, and mark the Legacy comparison as needing a new run. A full recertification is not required. |
| **C. Release gate** | Every V&V'd filter whose source changed since the last release | Run the legacy comparison packages again on the release candidate, update the Legacy comparison rows, and confirm each Status. |

**The decision rule.** A change is Class B when it edits an oracle assertion, an expected value, an exemplar archive, or a comparison tolerance in a V&V'd filter's tests. Otherwise it is Class A. A reviewer can check this rule from the diff.

**What a failing V&V test means.**

- An existing oracle test fails, or its expected value must change: the V&V record was wrong for that case. Either the oracle was wrong or the record certified defective output. Correct the record, not only the code.
- A new test fails on the old code and no existing test covered the case: the oracle is still valid, but the V&V had a coverage gap. Add the test to the test inventory and add a Part 2 deviation entry.

## 2. One branch for V&V work (Agreed)

- All V&V work is on one branch and one draft PR until the V&V policy and the report template are stable: the policy and template changes, the V&V records, and the code fixes for defects in V&V'd filters.
- **Why:** a template change after a merge would make the merged records out of date and require another pass over all records. One branch prevents a series of merges that later changes would make irrelevant.
- **Commits:** each code fix is its own commit, one per filter, with tests that fail on the old code. A V&V record update is a separate commit. Keep code commits self-contained, so that an urgent fix (for example a crash) can move to its own PR by cherry-pick.
- **Rebase often.** Rebase onto `develop`; never merge `develop` into the branch. Let other large PRs that change the same filters merge first, and absorb their changes here.
- Other PRs (features, out-of-core work, general bug fixes) do not edit `vv/` files. When such a PR changes a V&V'd filter, record the gap on the V&V branch; do not expand the other PR.

## 3. Legacy comparison packages (Agreed)

A legacy comparison is **diff-explanation evidence for users who migrate from DREAM.3D v6.5.171**. It is not a correctness oracle; the analytical unit tests establish correctness (see `vv_policy.md`).

- **Three-way run.** Each package runs the same shared legacy-format input through stock DREAM.3D v6.5.171, a local proof version (DREAM.3D v6.5.171 with surgical patches), and DREAM3D-NX.
- **The proof.** Stock DREAM.3D v6.5.171 differs from DREAM3D-NX only where a Part 1 deviation predicts it. The local proof version, which contains a fix for each such deviation, matches DREAM3D-NX. Then the documented deviations fully explain the differences.
- **When no patch is needed.** A deviation that is a configuration difference, a DREAM3D-NX-only option, or a validation check does not change the calculation; it needs no proof patch. Write "Not applicable" and the reason, never "missing".
- **Proof patches.** One commit per filter on the proof branch (`v6_5_172` in the BlueQuartz GitLab DREAM3D repository). The commit message names the deviation IDs and says what is fixed. Patch only the documented causes; no cleanup.
- **Archive.** Each package is stored in the V&V working archive (OneDrive) as `vv_work:<FilterName>Filter/`. Keep only the primary package; do not add extra copies or older runs, because they confuse later readers. A filter with no legacy equivalent gets a ReadMe that says so.
- **Package contents.** A ReadMe (filter, runners and their versions, cases, results, deviation-to-case-to-patch mapping, how to run again), pipelines, inputs, the three outputs, logs, comparison results, and SHA512 checksums.

## 4. Report wording (Agreed)

The rules for report wording are in [`report_template.md`](./report_template.md) and [`deviation_template.md`](./deviation_template.md). In short: write for a user of `develop` or a DREAM3D-NX release; name release versions, not branches, commits, or PRs; refer to DREAM.3D v6.5.171 only; call a patched legacy build "a local proof version".
