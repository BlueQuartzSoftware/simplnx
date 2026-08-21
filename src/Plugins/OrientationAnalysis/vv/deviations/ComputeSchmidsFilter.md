# Deviations from DREAM3D 6.5.171: ComputeSchmidsFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent.

Entries are referenced by stable ID (`ComputeSchmidsFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

---

## ComputeSchmidsFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeSchmidsFilter-D1` |
| **Filter UUID** | `b4681855-0a3d-4237-97f2-5aec509115c4` |
| **Status** | active |

**Symptom:** Every Schmid factor and both angle components produced by DREAM3D 6.5.171 on the default (auto) slip-system path are systematically too large by a fixed factor. The inflation is uniform, so it is invisible in any relative comparison and only shows up against an absolute reference — or against the physical bound. Concretely, for the maximizing loading direction of a cubic crystal 6.5.171 reports a Schmid factor of **0.500090176**, which is impossible: the Schmid factor cannot exceed 0.5. The reported `cos phi` and `cos lambda` can likewise exceed 1.0.

The three inflation factors, per output array:

| Array | 6.5.171 value / correct value | Exact expression |
|---|---|---|
| `SchmidPhis` (`Schmid_Phis`) | 1.0000293384785181 (+0.0029338 %) | `sqrt(3) / 1.732f` |
| `SchmidLambdas` (`Schmid_Lambdas`) | 1.0001510099261918 (+0.0151010 %) | `sqrt(2) / 1.414f` |
| `Schmids` | 1.0001803528351113 (+0.0180353 %) | `sqrt(6) / (1.732f * 1.414f)` |

Measured against 6.5.171 on hand-built fixtures, the observed ratios matched these predictions to better than `1.1e-8` relative.

`SlipSystems` and `Poles` are **not** affected. The inflation is a single positive scale factor applied to all twelve candidate slip systems, so the argmax cannot move; and `Poles` is a function of the crystal-frame loading direction, which the normalizers never touch. Both were bit-identical between 6.5.171 and SIMPLNX on every auto-path fixture.

**Root cause:** Library. `OrientationLib/LaueOps/CubicOps.cpp:850-868` in 6.5.171 normalizes the `{111}` plane-normal dot products by the literal `1.732f` and the `<110>` slip-direction dot products by `1.414f`, instead of by `sqrt(3)` and `sqrt(2)`. Both literals are four-significant-digit truncations and both are smaller than the constants they stand in for, so every direction cosine comes out too large. EbsdLib carried the identical defect through version 3.1.0, which means **SIMPLNX built against EbsdLib <= 3.1.0 reproduces this deviation exactly** — this is a shared library defect, not a 6.5.171-only one.

Fixed in EbsdLib on `topic/3_1_1_staging` (commit `4a56725`, `BUG: Use exact sqrt(3)/sqrt(2) normalizers in CubicOps::getSchmidFactorAndSS`) by replacing both literals with the full-precision `ebsdlib::constants::k_Sqrt3D` and `k_Sqrt2D`. The `double` type matches the surrounding arithmetic, which is entirely `double` — those two `float` literals were the only precision loss in the function.

Note that the *override* slip-system path (`Override Default Slip System` = on) was never affected in either implementation: it normalizes the user-supplied plane and direction with `std::sqrt` and never used the truncated literals. The A/B run confirms this — override-path floats agree between the two builds to `1.2e-7` relative with no systematic bias, which is the control that isolates the normalizer as the sole cause of the auto-path divergence.

**Affected users:** Everyone who has run Compute Schmid Factors, or legacy Find Schmid Factors, with `Override Default Slip System` off — that is, essentially every user of this filter. Absolute Schmid-factor values are wrong in the fourth decimal place. Relative comparisons *between* Features, rankings, histograms, and the reported slip system are all unaffected, so most downstream analyses reach the same conclusion.

**Recommendation:** **Trust SIMPLNX**, built against EbsdLib >= 3.1.1. The corrected values match an exact-arithmetic oracle to `1e-6` on all twelve value-asserting Class 1 fixtures and satisfy the physical bound `0 <= m <= 0.5`, which the pre-fix values provably violate.

No surgical patch to the legacy source was produced for this deviation, deliberately. The defective arithmetic lives in `OrientationLib`, not in `FindSchmids`, so a filter-level patch could not bring the two into alignment and a library-level patch would exceed the "smallest possible diff, one filter per patch" scope of the alignment-validation protocol. The alignment proof used instead is direct: fixed-EbsdLib SIMPLNX reproduces the exact-arithmetic oracle.

---

## ComputeSchmidsFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeSchmidsFilter-D2` |
| **Filter UUID** | `b4681855-0a3d-4237-97f2-5aec509115c4` |
| **Status** | active |

**Symptom:** For Features the filter does not compute — Feature 0, which is the conventional unassigned-Feature sentinel, and any Feature whose phase maps to a crystal structure with no enumerated slip systems — the two angle-component arrays read **`-301.0`** in 6.5.171 and **`0.0`** in SIMPLNX. `Schmids`, `SlipSystems` and `Poles` read `0` in both.

**Root cause:** Bug in 6.5.171. `FindSchmids.cpp:234` and `:242` create the `SchmidPhis` and `SchmidLambdas` arrays with `createNonPrereqArrayFromPath<DataArray<float>, AbstractFilter, float>(this, tempPath, -301, cDims)`. The third argument of that template is the array's `initValue`, not an error code; `-301` is an error code that was pasted into the wrong parameter slot. The same call for `Schmids`, `SlipSystems` and `Poles` correctly passes `0`. Because the execute loop writes the angle components only inside `if(xtal < LaueGroupEnd)`, uncomputed rows keep the `-301` sentinel.

SIMPLNX created all five arrays with `CreateArrayAction`'s default empty fill value, where 6.5.171 passed `initValue 0` for three of them. Preflight now passes an explicit `"0"` fill for all five arrays, and the algorithm additionally writes the Feature-0 row explicitly.

**How much exposure that actually carried, measured rather than assumed.** The obvious reading — that without a fill the uncomputed rows hold indeterminate memory — does **not** hold for the in-core store. `CoreDataIOManager::addDataStoreFnc()` constructs every in-core `DataStore<T>` with `static_cast<T>(0)` as its `initValue` irrespective of the action's `fillValue` string, and `DataStore`'s constructor unconditionally `std::fill_n`s the buffer with it (`DataStore.hpp:66-69`). Removing the explicit fill and re-running a 20 000-Feature fixture with same-size heap blocks poisoned to `0xAB` and freed immediately before the run still yields all zeros; a standalone probe confirms that on the same platform a 20 000-element `new float[]` after a same-size dirty free *does* return the poison, so the zeros are written by SIMPLNX and not inherited from the allocator. The explicit fill is therefore **defensive on a reachable path, not merely hypothetical**: `DataIOCollection::checkStoreDataFormat()` silently rewrites an empty `dataFormat` to the preferred large-data format whenever the array exceeds the large-data threshold and such a store factory is registered (e.g. the external `SimplnxOoc` plugin), and nothing guarantees that factory hard-codes a zero `initValue`. The fill restores what legacy stated explicitly and is the only in-repo guarantee on that path, which this cycle did not verify (logged open). The SIMPLNX side of this is a **latent port regression** — the `initValue` was genuinely dropped, but no in-core release ever produced indeterminate output because of it.

**Affected users:** Anyone whose data contains a phase whose crystal structure has no slip systems enumerated (anything other than Cubic-High, Cubic-Low, Hexagonal-High or Hexagonal-Low, or an unindexed/unknown phase), and anyone reading Feature 0. Under 6.5.171 those rows are a recognisable but undocumented `-301` sentinel. Under SIMPLNX they read `0` — before this cycle by way of the store factory's hard-coded zero rather than by the filter's own intent, and now by both.

**Recommendation:** **Trust SIMPLNX.** Zero is the meaningful "not computed" value here and is consistent with what the other three output arrays already do for the same rows. Filter uncomputed Features by phase or by `Schmids == 0`, not by looking for `-301`.

---

## ComputeSchmidsFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeSchmidsFilter-D3` |
| **Filter UUID** | `b4681855-0a3d-4237-97f2-5aec509115c4` |
| **Status** | active |

**Symptom:** Two classes of undefined output from the orientation library's Schmid-factor routines, present in both 6.5.171's `OrientationLib` and EbsdLib through 3.1.0.

1. `HexagonalLowOps::getSchmidFactorAndSS(load, schmidfactor, angleComps, slipsys)` **read** `schmidfactor` before writing it. The function declares its locals and then makes `if(schmid1 > schmidfactor)` the first statement that touches `schmidfactor`, so the entire comparison chain was driven by an indeterminate value. `slipsys` was likewise never initialized. Depending on the garbage, a Hexagonal-Low Feature could get a Schmid factor of zero with an arbitrary slip-system index, or could accept a candidate against a garbage incumbent.

2. `HexagonalOps` seeded `schmidfactor` but not `slipsys` or `angleComps`; and the seven Laue classes that enumerate no slip systems (`TrigonalOps`, `TrigonalLowOps`, `TetragonalOps`, `TetragonalLowOps`, `OrthoRhombicOps`, `MonoclinicOps`, `TriclinicOps`) set `schmidfactor` and `slipsys` but left `angleComps` **untouched**.

Case 2 is the more damaging in practice because it is silent and produces plausible-looking output. A caller that hoists one `angleComps[2]` buffer outside its per-Feature loop — the natural way to write the loop, and what both `FindSchmids` and `ComputeSchmids` did — gets the **previous Feature's** angle components attributed to the current Feature whenever the current Feature's Laue class is one of the seven stubs. The Schmid factor correctly reads 0 while the two angle columns read as real measurements carried over from an unrelated Feature.

**Root cause:** Bug, in the library. Fixed in EbsdLib on `topic/3_1_1_staging` (commit `2c84f2a`, `BUG: Define every getSchmidFactorAndSS output on all paths`) by seeding all four outputs at the top of every affected overload. That changes no value on any path that already computed a result; it only replaces indeterminate or stale output with a defined zero. `CubicOps` and `CubicLowOps` already wrote all four unconditionally and were left alone, as were all eleven plane/direction overloads, which already zeroed their outputs on entry.

Independently of the library fix, SIMPLNX now declares `schmid`, `angleComps` and `slipSystem` **inside** the per-Feature loop, so the stale-carryover half of this defect cannot occur regardless of which EbsdLib the filter is linked against.

One consequence of the fix, and three observations deliberately **not** changed, are recorded here so the decisions are on the record.

**A new `slipsys = 0` "no candidate" sentinel for the hexagonal classes.** `HexagonalOps` and `HexagonalLowOps` assign `slipsys` values **1 through 6** when a candidate wins the strict `>`. Seeding `slipsys = 0` therefore introduces a value *outside* that documented range, and it is now the reportable signal that no candidate ever beat the seeded `schmidfactor = 0.0` — the hexagonal analogue of the override path's `slipsys = 0`. Callers reading a hexagonal `SlipSystems` value must treat `0` as "no slip system found", not as a slip-system number. Before the fix the same situation produced whatever the caller had passed in. Recorded in `docs/ComputeSchmidsFilter.md`.

**KNOWN-OPEN: twelve further SC-2 instances in the hexagonal plane-normal geometry.** Six of the hexagonal slip-plane normals compute their `z` component by dividing by a truncated **sqrt-derived normalizer held as a bare decimal literal**. Structurally these are the *same* defect as D1 — a four-significant-digit stand-in for an irrational constant inside otherwise all-`double` arithmetic — not merely an approximate direction cosine:

| Literal | Constant it stands for | Relative error | Sites (EbsdLib `topic/3_1_1_staging`) |
|---|---|---|---|
| `0.8164` | `2/sqrt(6)` = 0.81649658 | −1.18e-4 | `HexagonalOps.cpp:758, 765, 772, 779`; `HexagonalLowOps.cpp:671, 678, 685, 692` |
| `1.154` | `2/sqrt(3)` = 1.15470054 | −6.07e-4 | `HexagonalOps.cpp:786, 793`; `HexagonalLowOps.cpp:699, 706` |

The `1.154` bias is **3.4x** the `sqrt(6)/(1.732f * 1.414f)` cubic bias that D1 was raised for. Status is **known-open, deferred**, not benign. The reason for deferring is specific: unlike D1's uniform positive scale factor, these literals perturb the *direction* of a plane normal — `l<i>nz` is one component of a vector that is renormalized by its own computed norm a few lines later, so the error tilts the normal instead of cancelling, and fixing it can move the argmax. A fix therefore changes both the reported hexagonal Schmid factor **and** the reported slip system, and needs its own hexagonal Class 1 oracle first — exactly the work that preceded the cubic fix. Follow-up work, with the twelve line numbers above.

**Genuinely benign truncated literals, kept — and why they are different.** The remaining hexagonal literals (`0.707`, `0.57735`, the `0.4472`/`0.8944` pair, and the `0.4082`/`0.8164` pair in the direction-cosine triples at `HexagonalOps.cpp:664-673` / `HexagonalLowOps.cpp:577-586` — distinct occurrences from the twelve divisor sites above, where `0.8164` also appears) appear only as *proportional triples* that are divided by their own computed norm a few lines later, and the truncations preserve the component **ratios** exactly: the `0.707` and `0.57735` triples are equal-magnitude, `0.4472 / 0.8944 = 0.5` exactly, which is the exact `(1/sqrt(5)) / (2/sqrt(5))`, and `0.4082 / 0.8164 = 0.5` exactly, matching `(1/sqrt(6)) / (2/sqrt(6))`. A common relative error cancels identically in the renormalization, so these change no output bit. That is precisely the distinction from the twelve above, where the literal is a **divisor applied to one component only** and so survives renormalization. Two further items are noted but out of scope: `0.866025` for `sqrt(3)/2` in the hexagonal-to-Cartesian basis transform (relative error −4.6e-7, three orders of magnitude below the twelve), and `caratio` hard-coded to `1.633` — a modelling limitation rather than a precision one, since it ignores the actual `c/a` of the phase.

**Slip-system numbering base.** `HexagonalOps` and `HexagonalLowOps` number their reported slip systems from **1**, while `CubicOps` numbers from **0**. Changing either would break existing user data. Documented, unfixed.

**Affected users:** Anyone whose data contains a Hexagonal-Low phase (defect 1), and anyone whose data mixes a Cubic or Hexagonal phase with any of Triclinic, Monoclinic, Orthorhombic, Tetragonal-Low/High or Trigonal-Low/High while `Store Angle Components of Schmid Factor` is on (defect 2).

**Recommendation:** **Trust SIMPLNX** built against EbsdLib >= 3.1.1. For SIMPLNX built against EbsdLib <= 3.1.0, the per-iteration reinitialization added in this cycle already removes the stale-carryover exposure; the `HexagonalLowOps` uninitialized read requires the library fix.

---

## ComputeSchmidsFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeSchmidsFilter-D4` |
| **Filter UUID** | `b4681855-0a3d-4237-97f2-5aec509115c4` |
| **Status** | active |

**Symptom:** Three interface-level differences that are stable across both implementations but easy to trip over.

1. **The angle-component arrays change units with a boolean toggle.** With `Override Default Slip System` **off**, `Phis` and `Lambdas` hold the direction **cosines** `cos phi` and `cos lambda` — dimensionless, in `[0, 1]`. With it **on**, the same two arrays hold the **angles** `phi` and `lambda` themselves, in **radians**. The two code paths call different overloads: the auto path stores the raw dot products, the override path stores `std::acos` of them. Both parameter descriptions call these "the angle between ...", which is only true in the second case. Present identically in 6.5.171 and SIMPLNX. Consequence: with the toggle off the Schmid factor equals the product of the two stored values; with it on it equals the product of their cosines.

2. **The default angle-array names differ between versions.** 6.5.171 defaults to `SchmidPhis` and `SchmidLambdas`; SIMPLNX defaults to `Schmid_Phis` and `Schmid_Lambdas`. Pipelines converted from SIMPL carry their explicit names across correctly, so this bites only when comparing outputs or hand-writing a downstream path.

3. **The preflight rejection code differs.** A slip direction that does not lie in the slip plane is rejected at preflight by both, with the same message text (`"Slip Plane and Slip Direction must be normal"`) but different codes: `-1001` in 6.5.171, `-13500` in SIMPLNX. Verified on a dedicated negative fixture.

**Root cause:** Algorithmic choice / order of operations, for (1); naming and error-code renumbering during the port for (2) and (3).

**Affected users:** For (1), anyone who toggles `Override Default Slip System` and consumes `Phis`/`Lambdas` — the values silently change units and any downstream arithmetic that assumed cosines is off by a `cos`. For (2) and (3), anyone diffing outputs across versions or matching on error codes.

**Recommendation:** **Either acceptable within tolerance** — the behaviour is the same in both versions and nothing is numerically wrong. The unit flip is documented in `docs/ComputeSchmidsFilter.md` with an explicit table, and unit tests assert both relationships (`m == Phis * Lambdas` on the auto path, `m == cos(Phis) * cos(Lambdas)` on the override path) so the distinction cannot silently regress. Convert explicitly rather than assuming a unit.

---

## ComputeSchmidsFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeSchmidsFilter-D5` |
| **Filter UUID** | `b4681855-0a3d-4237-97f2-5aec509115c4` |
| **Status** | active |

**Symptom:** With `Override Default Slip System` **on**, the `SlipSystems` output disagrees between the two versions for the same input and the same physical answer. On the `(001)[100]` system under `[1,2,3]` loading, 6.5.171 reports **8** and SIMPLNX reports **3**, while the Schmid factor (`3/7` exactly), `Phis`, `Lambdas` and `Poles` all agree.

This deviation was **not predicted** before the comparison ran. The pre-run prediction asserted `SlipSystems` would be identical on every fixture, reasoning that a uniform positive scale factor cannot move an argmax. That reasoning is correct and does hold on all five auto-path fixtures — the divergence has a different cause entirely, found by adjudicating the unexpected difference rather than by explaining it away.

**Root cause:** Algorithmic choice, specifically an indexing convention. On the override path the reported value is not a slip-system number at all: it is the index of the **symmetry operator**, within that library's 24-element cubic symmetry table, that maps the user-supplied plane and direction onto the maximizing variant. Two things follow.

- The two tables hold the same group in a **different order**. `OrientationLib`'s `CubicMatSym` and EbsdLib's `CubicHigh::k_MatSym` differ at **18 of their 24 positions**, but as *sets* they are identical: every one of EbsdLib's 24 matrices occurs exactly once in `OrientationLib`'s table, so there is a well-defined reordering permutation between them.
- **Six operators tie at the maximum, and they are the same six group elements in both libraries.** EbsdLib's tie set `{3, 6, 8, 9, 11, 14}` maps element-by-element through that permutation exactly onto `OrientationLib`'s `{8, 9, 10, 12, 14, 16}` — the pairing is `3->9`, `6->8`, `8->16`, `9->12`, `11->10`, `14->14`. Both implementations are therefore choosing among the *same* six maximizing operators, and each reports whichever member its own enumeration order reaches first under the strict `>`: EbsdLib reaches its index 3 first, `OrientationLib` reaches its index 8 first, and `OrientationLib`'s 8 *is* EbsdLib's 6. That is the whole mechanism — one tie set, two enumeration orders. Verified directly against both source tables in exact rational arithmetic.

The two *reported* matrices happen also to be transposes of one another, but that is a coincidence of which member each order reaches first and carries no weight here: transposition does not preserve the Schmid factor when the slip plane and the slip direction differ, so it could not by itself explain the equal maximum.

Neither answer is wrong. The value is a handle into a specific library's table, not a portable physical label, and it is arbitrary among equivalent maximizing operators.

This is distinct from the auto path, where `SlipSystems` **is** a genuine slip-system index into the crystal structure's built-in list (0-11 for Cubic-High) and **was** bit-identical between the two versions on every fixture.

**Affected users:** Anyone who uses `Override Default Slip System` and records, filters on, or compares the `SlipSystems` value — particularly anyone migrating stored results or scripts from DREAM3D 6.5.x.

**Recommendation:** **Either acceptable** — both values identify a correct maximizing symmetry operator. Where a choice is needed, **trust SIMPLNX**, because the SIMPLNX index is the one addressable through EbsdLib's own `getMatSymOp()` accessor, so a user can retrieve the actual matrix the index refers to. Do not treat this value as portable across versions, and do not treat it as uniquely determined even within one version. `docs/ComputeSchmidsFilter.md` now states both caveats, and the unit test that asserts index 3 carries a comment recording that it pins EbsdLib's table order rather than a physical label.
