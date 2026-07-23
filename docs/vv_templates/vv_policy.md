# DREAM3D-NX Filter V&V Policy (v2)

This is the short-form policy for filter Verification & Validation (V&V) in DREAM3D-NX. It exists to support MTR SBIR deliverables and the public DREAM3D → DREAM3D-NX migration guide.

Read this once. The deliverable is a one-page report per filter; the working details live in the templates and gates docs alongside this file.

---

## Why

DREAM3D 6.5.171 is trusted by reputation, not by proof. It has been in the field long enough that users rely on its numbers, but it was never subjected to V&V rigor. We therefore cannot treat "matches 6.5.171" as a correctness check — it is a **diff-explanation** check.

Each filter must first be shown correct **independently** of 6.5.171 against an oracle, and only then diffed against legacy to produce user-facing migration guidance.

## The one ordering rule

> **Pick the oracle before running any DREAM3D 6.5.171 comparison.**

Everything else — code-path enumeration, test inventory, exemplar provenance, algorithm review, documentation — can be worked in any order.

## Oracle classes

Every filter must be verified against at least one of the following. Classes 1–4 are preferred; Class 5 requires a documented justification for why no Class 1–4 oracle was feasible.

| # | Class | What it is | Drift risk |
|---|---|---|---|
| 1 | Analytical | Closed-form expected output on hand-built input (threshold, crop, rotate, color conversion, array arithmetic) | None |
| 2 | Reference implementation | Trusted external library produces the expected output (NumPy / SciPy / MTEX / EbsdLib upstream / Eigen) | **High** — library version drift |
| 3 | Paper-based | Filter reproduces a published figure, table, or equation from a named reference | Low–Medium |
| 4 | Invariant | Derivable properties the output must satisfy (FeatureIds start at 1 and contiguous; sum of phase fractions = 1; mass conservation) | None |
| 5 | Expert-visual | Domain expert signs off on output for canonical test cases. Last resort. | **High** — social drift |

**"Legacy 6.5.171 produced this output" is never a valid oracle for correctness.**

The oracle and its design **really should** be reviewed by a second engineer. A wrong oracle silently confirms buggy filters, and the filter author is the least likely person to notice — test cases unconsciously get designed around the code that already exists. If skipped, record the reason in the provenance sidecar.

For detailed explanations of each class — with examples, strengths and weaknesses, drift-risk analysis, common anti-patterns ("what is NOT an oracle"), and a decision tree for picking the right class for a given filter — see [`oracle_classes.md`](./oracle_classes.md).

## Deliverables per filter

Three artifacts live in the source tree at the filter's commit:

| Artifact | Location |
|---|---|
| **V&V report** (1 page) | `src/Plugins/<PluginName>/vv/<FilterName>.md` |
| **Deviations from 6.5.171** | `src/Plugins/<PluginName>/vv/deviations/<FilterName>.md` |
| **Exemplar provenance** (per archive) | `src/Plugins/<PluginName>/vv/provenance/<archive>.md` |

The verified state is pinned by **(commit hash, archive SHA512)**. The commit captures source + tests + `download_test_data()` declaration; the SHA512 captures the archive contents.

## Templates and gates

| File | What it is |
|---|---|
| [`report_template.md`](./report_template.md) | Empty report — copy into `src/Plugins/<P>/vv/<FilterName>.md` |
| [`report_gates.md`](./report_gates.md) | Per-section "Done when:" checklists — reference while filling in the report |
| [`deviation_template.md`](./deviation_template.md) | Empty deviation file — copy into `src/Plugins/<P>/vv/deviations/<FilterName>.md` |
| [`provenance_template.md`](./provenance_template.md) | Empty exemplar-provenance sidecar — copy per exemplar archive |
| [`commit_template.md`](./commit_template.md) | Standard commit message format for landing a completed V&V cycle — use at step 6 of the engineer workflow below |

## Engineer workflow

1. Read this policy doc once.
2. Decide the oracle class for this filter (write it down).
3. Run `python scripts/vv_init.py <FilterName>` to scaffold the report and deviation files in the plugin tree.
4. Open `report_gates.md` in a second tab.
5. Work each section in any order. A section is "done" when all its gates pass.
6. When all gates green, set `Status: READY FOR REVIEW`, push a `vv/<FilterName>` branch with a commit following [`commit_template.md`](./commit_template.md).
7. After sign-off, set `Status: COMPLETE`. Verified commit hash is filled in at SBIR deliverable assembly.

## Status tracking across filters

Each report's Status line gives a fleet-wide view via one grep:

```bash
grep -r '^| Status |' src/Plugins/*/vv/*.md | sort
```

## Algorithm Relationship — opening claim of every report

Every report's Algorithm Relationship section uses one of these classifications. This sets reader expectations and frames the Deviation entries.

- **Port** — line-by-line translation of the legacy algorithm. Differences should be minor and confined to type precision, library calls, or parallelization.
- **Minor changes** — same algorithm intent with small deliberate improvements (e.g., `float` → `double`, corrected boundary handling, Eigen instead of hand-rolled math).
- **Rewrite** — substantially different implementation under the same UUID. **A rewrite that produces materially different outputs is a red flag** — keeping the UUID is a claim of functional equivalence. The Deviations file must defend the claim.
- **New filter, no legacy equivalent** — legacy comparison is N/A.

## Root-cause categories for Deviation entries

When SIMPLNX and 6.5.171 differ, each Deviation entry names one root cause (or a short compound like "precision + library"):

- **Bug** — one implementation is mathematically wrong
- **Precision** — different floating-point width or intermediate-math type
- **Order of operations** — associativity differences in parallel reductions, different loop order, different accumulation
- **Library** — Eigen vs. hand-rolled, different EbsdLib version, different HDF5 versions
- **Algorithmic choice** — a deliberate difference in method or behavior (an added validation guard, a changed default, exposed randomization, a replacement algorithm). Any Algorithm Relationship may carry algorithmic-choice deviations — a **Port** or **Minor changes** filter that adds a guard legacy lacked is the common case. A **Rewrite** consists of algorithmic choices by definition, and a Rewrite whose outputs diverge must defend the shared UUID (see above)
