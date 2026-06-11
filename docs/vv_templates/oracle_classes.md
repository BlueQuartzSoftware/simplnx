# Oracle Classes — Detailed Reference

The DREAM3D-NX V&V policy (see [`vv_policy.md`](./vv_policy.md)) names five oracle classes — five ways to establish what the *correct* output of a filter should be, independently of any DREAM3D implementation. The policy's table is a quick lookup. This document is the long-form explanation: what each class actually means in practice, what kinds of bugs it catches, what its weaknesses are, and how to choose between them when designing the oracle for a new (or retroactive) V&V pass.

## What an oracle is, and why this matters

An **oracle** is the source of truth you compare the filter against. The policy's "one ordering rule" is to pick the oracle *before* running any DREAM3D 6.5.172 comparison — because if the oracle is wrong, you will happily confirm a buggy filter. The oracle must be independent of the implementation it validates: it cannot be the filter's own output, and it cannot be the legacy DREAM3D's output (which is "trusted by reputation, not by proof").

The five classes differ in **what they trust** (math, a library, a paper, properties, or a person) and **how durable** that trust is over time. The policy ranks them by trust durability, and prefers oracles that are stable, reproducible, and external to the code being tested.

## Class 1 — Analytical

**What it is:** You can write down, on paper, the *exact* expected output as a closed-form function of the input. The math is small enough to do by hand or in a spreadsheet on a hand-built fixture.

**Where the trust comes from:** Mathematics itself. Arithmetic doesn't drift. `2 + 2 == 4` today, tomorrow, and forever.

**Examples:**

- Thresholding: `output[i] = (input[i] > T)`. Hand-pick a few inputs above and below `T`; the expected output is obvious.
- Cropping: `output[i,j,k] = input[i+oₓ, j+oᵧ, k+oᵤ]`. Pick a 5×5 input and a 2×2 crop; you know exactly which input cells should appear in which output cells.
- Color conversion (e.g., RGB→HSV): the formulas are in any reference; apply them by hand to a few colors.
- Indirection lookups: `output[i] = input[map[i]]`. Trivial to verify on a 6-element example.
- Grouping density: `density[i] = ParentVolumes[i] / Σ Volumes[k]` where `k` ranges over a set you compute by hand from the neighbor lists. Set-union sums + division — exactly the kind of thing that fits Class 1.

**Strengths:**

- Zero drift. The expected output is a function of the input, not of any library or external state.
- No external dependencies. The test runs in any environment.
- The derivation can be embedded as a comment next to the `REQUIRE(result == X)`, so future readers see both the truth and the assertion.
- Provable: if someone disputes the answer, they can re-derive it.

**Weaknesses:**

- Only works when the algorithm has a tractable closed form on small inputs. Many real algorithms don't (e.g., segmentation, optimization, iterative convergence).
- Hand-built fixtures may not exercise all code paths in a complex filter.

**How it's encoded in tests:** Usually inline `REQUIRE(result == 0.6428571)` with a comment showing the hand derivation. Or a tiny exemplar `.dream3d` whose values were hand-computed and the `.dream3d` is just a cache of the computation.

### Naming convention: `AnalyticalFixtures`

The C++ namespace used to hold Class 1 test scaffolding (helpers, struct, fixture builders) is `AnalyticalFixtures` — e.g., `AnalyticalFixtures::CreateScaffold`, `AnalyticalFixtures::FixtureData`.

**Why not `ToyFixtures`?** In CS / ML / testing terminology, "toy example" is the established term for a minimal hand-crafted dataset with a closed-form expected output. It is a precise technical signal — not dismissive — meaning *"this is an analytical oracle, not a regression-on-real-data circular oracle."* The original V&V scaffolding used `ToyFixtures` on those grounds.

**Why `AnalyticalFixtures` then?** SIMPLNX V&V deliverables read to a non-CS-academic audience (SBIR program managers, MTR reviewers, materials engineers). To that audience, "toy" can read as informal or unrigorous — the exact perception V&V is meant to dispel. `AnalyticalFixtures` preserves the load-bearing signal that "Toy" was carrying (*Class 1 closed-form oracle data*) without the dismissive connotation.

The rename was applied 2026-06-10 across all test files, V&V reports, deviations, provenance docs, and templates. The decision is documented here so it doesn't get re-litigated.

## Class 2 — Reference implementation

**What it is:** A trusted external library — one with broader user base, more testing, and longer history than your filter — computes the same operation on the same input. Its output becomes your expected output.

**Where the trust comes from:** The library's reputation. NumPy has millions of users; if its `np.linalg.inv()` is wrong, the world notices.

**Examples:**

- NumPy / SciPy for general array operations (matrix multiply, FFT, sort, statistics).
- MTEX (MATLAB) for orientation math and texture analysis.
- EbsdLib upstream for crystallographic operations (the simplnx filter might use a *different version* of EbsdLib than the standalone library does).
- Eigen for linear algebra in C++ tests.
- A reference Python implementation of an algorithm published as a research paper companion.

**Workflow:** Write a small Python (or other) script that takes the hand-built input, runs the reference library, and saves the expected output. That script + the library version + any random seed used become part of the V&V archive. Future tests load the saved expected output and compare bit-for-bit (or with appropriate float tolerance).

**Strengths:**

- External cross-check: you're not just testing your filter against your own arithmetic.
- For libraries that implement the same algorithm (e.g., the simplnx filter calls EbsdLib for some orientation math, and you can run EbsdLib's standalone test on the same input), this can catch real implementation drift between versions.

**Weaknesses:**

- **HIGH library-version drift.** If NumPy 1.24 produces output X and NumPy 2.0 produces output Y (different rounding, different algorithm internals), your "expected output" silently changes. You must record the exact library version (and seed for any random parts) in the archive — and accept that re-running the script years later may produce a different "expected output" unless you've pinned the library.
- The library may have its own bugs. Trusting it transitively trusts every commit in its history.
- The library might solve a slightly *different* problem than your filter (e.g., "convex hull" can mean different things). Subtle semantics mismatches are a frequent source of false-positive deviations.

**How it's encoded in tests:** A cached exemplar `.dream3d` file in the archive, generated by the Python/MATLAB script. The script and its version pin live in the archive too. Test compares filter output against the cached arrays.

## Class 3 — Paper-based

**What it is:** The filter implements an algorithm published in a peer-reviewed paper. The paper contains a worked example, figure, or equation showing the expected output. You reproduce that exact example.

**Where the trust comes from:** Peer review. The paper survived a referee process and (typically) replication by others.

**Examples:**

- **Rowenhorst 2015** ("Consistent representations of and conversions between 3D rotations"): provides explicit worked examples of orientation-representation conversions (Euler → quaternion → axis-angle → rotation matrix → Rodrigues vector). simplnx's `ConvertOrientations` filter implements this — the paper's worked examples are direct Class 3 oracle fixtures.
- **Bunge texture analysis** for ODF (orientation distribution function) calculations.
- **Hoshen-Kopelman 1976** for connected-component labeling — the paper has small worked examples with labels you can verify against.
- Whatever paper your filter's algorithm cites in its header comment.

**Strengths:**

- The most authoritative source. The author of the algorithm isn't writing the test, eliminating the "tests unconsciously designed around the existing code" risk.
- Captures the algorithm's *intent* precisely (the paper says what it should do; the filter is supposed to do that).
- Often comes with multiple test cases (figures, tables, edge cases).

**Weaknesses:**

- Low-to-medium drift. Papers don't update, but later papers may publish errata or alternate formulations. You need to pin to the exact paper revision (DOI + edition).
- Extracting numbers from PDF figures/tables is tedious and error-prone (re-read the text twice; pixel-pick figure values).
- Only applicable to filters that implement a published algorithm. Custom internal algorithms don't have one.

**How it's encoded in tests:** Inline `REQUIRE(result == X)` with a citation comment, e.g. `// Per Rowenhorst 2015, Eq. 7, p. 12: q = [0.7071, 0, 0.7071, 0]`. The paper PDF is embedded in the V&V archive for traceability.

## Class 4 — Invariant-based

**What it is:** The output must satisfy certain mathematical properties (invariants), regardless of input. You assert these properties as predicates instead of asserting specific values.

**Where the trust comes from:** Logical derivation from the algorithm's specification. The invariants follow from what the filter is *supposed to do*.

**Examples:**

- **Bounds:** `0.0 ≤ density ≤ 1.0` (when no sentinel is involved).
- **Sentinel signaling:** `density == -1.0f` exactly when no features touched the parent (no in-between values, no other negative numbers).
- **Range membership:** `FeatureIds[k] ∈ {0, 1, …, numFeatures-1}`.
- **Contiguity / completeness:** `FeatureIds` start at 1, no gaps (no orphan IDs from a segmentation filter).
- **Conservation:** total cell count out == total cell count in (no cells dropped silently).
- **Sum-to-one:** `Σ phase_fraction[p] == 1.0` for any cell.
- **Symmetry under transformation:** `Filter(Rotate(input)) == Rotate(Filter(input))` for rotation-equivariant filters.
- **Idempotence:** `Filter(Filter(input)) == Filter(input)` for filters that should converge in one pass.

**Strengths:**

- Cheap. No expected-output computation; you just write the property as a predicate.
- Zero drift. The invariants don't change unless the filter's specification changes.
- Catches whole *classes* of bugs at once. A boundary-handling regression that produces `density = 1.7` is caught by a single `REQUIRE(density ≤ 1.0)` regardless of what the actual correct value was.
- Works for *any* input, not just hand-picked fixtures. Especially powerful when paired with property-based testing (random input generation).

**Weaknesses:**

- **Does not fully specify behavior.** A buggy filter could satisfy every invariant and still be wrong. Example: a filter that always outputs `density = 0.5` would satisfy `0 ≤ density ≤ 1` but be obviously wrong.
- Best used as a **companion** to a stronger class (1, 2, or 3). On its own, Class 4 is necessary but not sufficient.
- Requires reasoning about what properties *must* hold, which is its own design skill.

**How it's encoded in tests:** Inline `REQUIRE(predicate)` assertions, often in a loop over output indices. No exemplar file needed.

## Class 5 — Expert-visual

**What it is:** A named domain expert visually inspects the output on canonical test cases and signs off that it looks correct. No formal computation of expected output; trust is placed in the expert's judgment.

**Where the trust comes from:** A person's experience and reputation.

**When it's used:** Last resort. Used when:

- The output is genuinely subjective (e.g., "does this segmentation look reasonable?", "is this rendering visually pleasing?").
- The algorithm is more art than math (heuristic image processing, aesthetic decisions).
- All other classes have been considered and rejected with documented reason.

**Strengths:**

- Captures expert intuition that's hard to formalize.
- Sometimes it's the only realistic option for visualization filters.

**Weaknesses:**

- **HIGHEST drift risk.** The expert disappears, changes their mind, or is replaced. Future maintainers can't reproduce the judgment.
- **Social drift.** Over time, the bar for "good output" shifts as the team adapts to what the filter actually produces — exactly the opposite of what an oracle should do.
- Reproducibility is poor: a screenshot signed off in 2024 may not be regenerable from the same input on different hardware/drivers in 2027.
- The policy **requires documented justification** for using Class 5 — you must explain why no Class 1–4 was feasible.

**How it's encoded in tests:** A cached exemplar `.dream3d` (or screenshot folder) signed off by named expert + date. Comparison via `CompareDataArrays` against the cached output.

## The drift-risk concept

Every oracle answers the question: *what's the right answer for this input?* **Drift** is the risk that, over time, the oracle's answer changes without the filter's specification changing.

| Class | Name | Drift source | Drift severity |
|---|---|---|---|
| 1 | Analytical | Math itself doesn't drift; only the *derivation* can be miscopied | None (if derivation is preserved as a comment) |
| 4 | Invariant | Properties don't drift; only the *specification* could change | None |
| 3 | Paper-based | Papers don't update, but related papers might publish errata | Low–Medium |
| 2 | Reference impl | Library version changes silently | **High** |
| 5 | Expert-visual | The expert leaves; the team's standards shift | **High (social drift)** |

The policy's preference order (1, 4 > 3 > 2 > 5) is essentially *prefer oracles with low drift, plus prefer oracles whose computation is transparent and reproducible.*

## Combining classes (encouraged)

Classes 1 and 4 are cheap — they cost almost nothing once you've done the work for a stronger class. The recommended pattern is:

- **Class 1 + Class 4** — hand-derive specific expected values *and* assert the invariants. The invariants act as a cheap sanity check against derivation typos; the specific values act as a tight bound on correctness.
- **Class 3 + Class 4** — cite the paper for specific values *and* assert the algorithm's invariants. The paper might have a transcription error in one figure; the invariants catch what slipped.
- **Class 2 + Class 4** — cross-check against a library *and* assert algorithm invariants. If the library output and the invariants disagree, you've found either a library bug or a misunderstanding of the algorithm.

You almost never use Class 5 alone. If you must use Class 5, pair it with Class 4 to limit damage.

## Decision tree

```
Is the output computable in closed form on a small input?
   YES → Class 1 (Analytical).        Cheapest, no drift. Default choice.
   NO ↓

Are there mathematical properties the output MUST satisfy?
   YES → Class 4 (Invariant).         Add these regardless of which other class you pick.

Does the filter implement a published algorithm?
   YES → Class 3 (Paper-based).       Strong external authority.
   NO ↓

Is there a trusted external library that solves the same problem?
   YES → Class 2 (Reference impl).    Watch for library version drift.
   NO ↓

Is there a named domain expert willing to sign off, and can you document
why no Class 1-4 was feasible?
   YES → Class 5 (Expert-visual).     Last resort. Pair with Class 4.
   NO ↓

Stop. You don't have an oracle. You cannot do V&V on this filter
until you find one.
```

## What is NOT an oracle

A common trap is to mistake one of the following for an oracle. None of these qualify:

- **The filter's own output**, captured on a previous date and saved as a "golden" exemplar. This is circular: any bug present at capture time becomes the new "correct" answer, and the test then confirms the bug forever.
- **The legacy DREAM3D 6.5.172's output.** Treated by reputation, not by proof. Useful for diff explanation (see [`vv_policy.md`](./vv_policy.md)), not for correctness.
- **Another simplnx filter's output**, when both filters might share the same bug (e.g., both call into a buggy shared utility).
- **A previous version of the same filter** ("my SIMPL implementation works, so I'll just diff against that"). Identical to the legacy DREAM3D trap.

If the existing exemplar in the data archive was generated from one of the above, it should be regenerated from a real oracle as part of the V&V pass — this is the "circular oracle" pattern called out in the retroactive audit's cross-cutting findings ([`docs/vv_retroactive_reports/INDEX.md`](../vv_retroactive_reports/INDEX.md)).

## Quick reference: when each class shines

| Class | Name | Best for | Default choice when |
|---|---|---|---|
| 1 | Analytical | Tight algorithms with closed-form output, hand-pickable hand-built inputs, small outputs | The math is short enough to do on paper |
| 2 | Reference impl | Algorithms where a trusted library exists and you trust it more than your own implementation | You're porting an algorithm that has a well-known library implementation elsewhere |
| 3 | Paper-based | Implementations of published algorithms (orientation math, classical algorithms with named authors) | The filter's header cites a paper |
| 4 | Invariant | Anything with conservation laws, range bounds, or structural constraints | Always — add Class 4 alongside whatever other class you pick |
| 5 | Expert-visual | Visualization quality, subjective image processing, rendering decisions | All Class 1–4 options have been ruled out with written justification |

## Encoding the choice in the V&V report

The per-filter V&V report's `## Oracle` section (per [`report_template.md`](./report_template.md)) requires four fields:

- *Class* — one or more class numbers (e.g., "1 primary, 4 companion").
- *Applied* — a one-paragraph description of how the oracle generates expected output for *this* filter.
- *Encoded* — a citation of the specific `TEST_CASE` or fixture in the codebase that runs the oracle, with the number of fixtures.
- *Second-engineer review* — named reviewer + date, or "Skipped — &lt;documented reason&gt;".

See [`report_gates.md`](./report_gates.md) for the exit criteria the Oracle section must satisfy before the V&V report can move from DRAFT to READY FOR REVIEW.
