# Deviations from DREAM3D 6.5.171: CopyFeatureArrayToElementArrayFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent (`CopyFeatureArrayToElementArray`, SIMPL UUID `99836b75-144b-5126-b261-b411133b5e8a`).

Entries are referenced by stable ID (`CopyFeatureArrayToElementArrayFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

**Numeric output on valid input is bit-identical** — A/B comparison run 2026-07-23 on a 4×3×1 / 4-feature fixture with float32 (1-comp), int32 (3-comp), and bool arrays showed all output arrays bit-identical between 6.5.171 and SIMPLNX. All deviations below concern *naming* and *input-validation* behavior, not numeric results.

---

## CopyFeatureArrayToElementArrayFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `CopyFeatureArrayToElementArrayFilter-D1` |
| **Filter UUID** | `4c8c976a-993d-438b-bd8e-99f71114b9a1` |
| **Status** | active |

**Symptom:** Pipelines converted from legacy DREAM3D produce output arrays with different *names*: legacy names the created array exactly what the user typed (e.g., `MyOutput`); SIMPLNX names it `<sourceArrayName><suffix>` (the legacy name string is converted into the suffix, so a legacy pipeline copying `EquivalentDiameters` to `MyOutput` produces `EquivalentDiametersMyOutput` in SIMPLNX).

**Root cause:** Algorithmic choice. The SIMPLNX filter was redesigned from single-array-with-explicit-name to multi-array-with-shared-suffix (`MultiArraySelectionParameter` + `created_array_suffix`). `FromSIMPLJson()` maps legacy `CreatedArrayName` onto the suffix (`CopyFeatureArrayToElementArrayFilter.cpp`, `FromSIMPLJson`), which cannot reproduce the legacy naming for a non-empty name.

**Affected users:** Anyone importing a legacy `.json` pipeline containing this filter. Downstream filters in the converted pipeline that reference the legacy output-array name will fail preflight until the user updates the reference (or renames via a Rename Data Object filter).

**Recommendation:** Trust SIMPLNX. Numeric content is identical; after conversion, update downstream array references to the new `<source><suffix>` name.

---

## CopyFeatureArrayToElementArrayFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `CopyFeatureArrayToElementArrayFilter-D2` |
| **Filter UUID** | `4c8c976a-993d-438b-bd8e-99f71114b9a1` |
| **Status** | active |

**Symptom:** When the selected Feature array has more tuples than `largestFeatureId + 1` (an over-provisioned Feature AttributeMatrix), DREAM3D 6.5.171 aborts with error -5555 ("The number of Features in the InArray array (N) does not match the largest Feature Id"); SIMPLNX runs successfully.

**Root cause:** Algorithmic choice. Legacy `execute()` required `largestFeatureId == numFeatures - 1` exactly (SIMPL `CopyFeatureArrayToElementArray.cpp:235-241`). SIMPLNX (`ValidateFeatureIdsToFeatureAttributeMatrixIndexing`, `DataArrayUtilities.cpp:160-198`) only rejects ids that would index *past* the array (`maxFeatureId >= numFeatures`, error -5351). Extra unused feature tuples are legal in SIMPLNX — they are simply never read.

Confirmed by A/B probe 2026-07-23: 6-cell fixture with max id 2 and an 8-tuple feature array — 6.5.171 errored -5555; SIMPLNX produced correct output for all referenced tuples (verified against hand-derived values; also pinned by unit test `Over-provisioned Feature array accepted`).

**Affected users:** Users whose Feature AttributeMatrix retains tuples for features no longer present in the FeatureIds map (e.g., after feature removal/cropping). Their legacy pipelines failed; the same data now processes in SIMPLNX.

**Recommendation:** Trust SIMPLNX. The legacy strict-equality check rejected valid input; the SIMPLNX output for all referenced feature ids is well-defined and verified.

---

## CopyFeatureArrayToElementArrayFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `CopyFeatureArrayToElementArrayFilter-D3` |
| **Filter UUID** | `4c8c976a-993d-438b-bd8e-99f71114b9a1` |
| **Status** | active |

**Symptom:** When the FeatureIds array contains a negative value, DREAM3D 6.5.171 **completes without any error** and writes an undefined value at the affected cells; SIMPLNX stops with error -5355 ("Feature Ids array … has negative values").

**Root cause:** Bug in 6.5.171. Legacy `copyData<T>()` computes `fPtr + (numComp * featureIdx)` with no lower-bound check (SIMPL `CopyFeatureArrayToElementArray.cpp:178-189`); a negative id reads out of bounds before the feature array's allocation — undefined behavior whose result depends on adjacent heap contents. The legacy max-id scan (`m_FeatureIds[i] > largestFeature`) only tracks the maximum, so negatives pass validation silently. SIMPLNX validates the minimum feature id before copying and refuses to run.

Confirmed by A/B probe 2026-07-23: fixture with `FeatureIds[5] = -1` — 6.5.171 reported "Pipeline Complete" and wrote 0.0 (whatever bytes preceded the allocation) at cell 5; SIMPLNX errored -5355 and produced no output.

**Affected users:** Anyone whose FeatureIds contains negative sentinel values (some workflows use -1 for "unassigned"). Legacy results at those cells were silent garbage.

**Recommendation:** Trust SIMPLNX. The legacy behavior is undefined and platform-dependent; SIMPLNX's hard error is the correct response. Users with -1 sentinels must clean/reassign them before copying feature data down to cells.
