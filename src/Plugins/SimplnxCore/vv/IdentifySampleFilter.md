# V&V Report: IdentifySampleFilter

|        |              |
|--------|--------------|
| Plugin | SimplnxCore |
| SIMPLNX UUID | `94d47495-5a89-4c7f-a0ee-5ff20e6bd273` |
| SIMPLNX Human Name | Isolate Largest Feature (Identify Sample) |
| DREAM.3D v6.5.171 equivalent | `IdentifySample` (Processing plugin, UUID `0e8c0818-a3fb-57d4-a5c8-7cb8ae54a40a`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | READY FOR REVIEW |
| Sign-off | Nathan Young, 2026-06-17; second engineer: pending |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Rewrite of `IdentifySample` in DREAM.3D v6.5.171. BFS retains the two-pass search; CCL uses component labels and replay, and SIMPLNX adds slice processing. |
| Oracle (confirmed) | Class 1 masks in the three non-square cases in `test/IdentifySampleTest.cpp` check largest-component selection. All pass in CI. |
| Code paths enumerated | 21 of 44 exercised. Gaps include flat-image hole filling, ties, cancellation, and storage failures. |
| Tests today | 6 test cases: 1 archive case with 12 sections, 3 analytical cases, 1 conversion case, and 1 hidden HDF5 case. The five ordinary cases all pass in CI. |
| Exemplar archive | `identify_sample_v2.tar.gz` supplies inputs and SIMPLNX-generated regression outputs. Inline analytical masks provide the independent oracle. |
| Legacy comparison | Not run — comparison artifacts are not available. |
| Bug flags | `IdentifySampleFilter-D6` |
| V&V phase | Source review, analytical fixtures, and archive regression are complete. Second-engineer oracle review and a current legacy comparison remain open. |

## Summary

`IdentifySampleFilter` keeps the largest face-connected mask component and can fill enclosed holes, either for the whole image or for each selected slice. Class 1 masks check connectivity, and twelve archive sections check regression behavior. The fixed defects are listed below; CCL still leaves enclosed holes unfilled on flat images (`IdentifySampleFilter-D6`).

## Algorithm Relationship

*Classification:* Rewrite

*Evidence:* `SimplnxCoreLegacyUUIDMapping.hpp` maps the legacy filter to this filter; BFS retains the two-pass search, while `IdentifySampleCCL.cpp` replaces it with label equivalences and replay.

## Oracle

*Class:* 1 (Analytical).

*Applied:* A hand-built 3×4 mask has components of four and two cells; only indices 0, 1, 3, and 4 must remain true.

*Encoded:* `src/Plugins/SimplnxCore/test/IdentifySampleTest.cpp::SimplnxCore::IdentifySampleFilter: 2D Empty Z Non-Square {3,4,1}` and the Empty Y and Empty X companion cases encode three rotated fixtures. All pass in CI.

*Second-engineer review:* Pending.

## Bugs found and fixed

All defects in this table are fixed. DREAM3D-NX v7.4.2 contains the BFS fix for `IdentifySample-D1` and the fixes for `IdentifySampleFilter-D2` and `IdentifySampleFilter-D3`. DREAM3D-NX v7.4.1 contains the fixes for `IdentifySampleFilter-D4` and `IdentifySampleFilter-D5`.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `IdentifySample-D1` | Whole-image hole filling treated a flat axis as a boundary at every cell. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | BFS ignores flat axes when it checks enclosure. The separate CCL issue is `IdentifySampleFilter-D6`. |
| `IdentifySampleFilter-D2` | The dimensional BFS filled a lone false voxel because its neighbor loop was empty. | None. DREAM3D-NX v7.0.0 through v7.4.2 were not affected. DREAM.3D v6.5.171 was not affected. | A single voxel starts with its boundary flag set and stays false. |
| `IdentifySampleFilter-D3` | Slice processing kept the first of two equally large components, unlike whole-image BFS. | DREAM3D-NX v7.0.2 through v7.4.1. DREAM.3D v6.5.171 was not affected. | The slice BFS uses `>=` and keeps the later component on a tie. |
| `IdentifySampleFilter-D4` | Slice hole filling used a local plane index as a global index for XZ and YZ planes. | DREAM3D-NX v7.0.2 through v7.4.0. DREAM.3D v6.5.171 was not affected. | Plane coordinates and strides place each filled voxel in the selected slice. |
| `IdentifySampleFilter-D5` | Slice hole filling reused visited flags from component selection and skipped removed internal islands. | DREAM3D-NX v7.0.2 through v7.4.0. DREAM.3D v6.5.171 was not affected. | Slice BFS clears its visited flags before hole filling. CCL labels the false components in a separate pass. |

## Code path coverage

21 of 44 paths exercised. The table separates test assertions from paths that have no direct fixture.

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/IdentifySample.cpp` (28 lines); `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/IdentifySampleBFS.cpp` (295 lines); `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/IdentifySampleCCL.cpp` (1055 lines); `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/IdentifySampleCommon.hpp` (517 lines); `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/IdentifySampleFilter.cpp` (139 lines).

`Archive` below means `SimplnxCore::IdentifySampleFilter`. Other names omit the `SimplnxCore::IdentifySampleFilter: ` prefix. The ordinary mask cases select BFS and CCL on resident stores through the scenario harness.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | BFS dispatch | Whole image, uint8 mask, all dimensions greater than one. | Archive — `whole_*`. |
| 2 | BFS dispatch | Boolean mask. | All three `2D Empty * Non-Square` cases. |
| 3 | BFS dispatch | Flat Z dimension. | `2D Empty Z Non-Square {3,4,1}`. |
| 4 | BFS dispatch | Flat Y dimension. | `2D Empty Y Non-Square {3,1,4}`. |
| 5 | BFS dispatch | Flat X dimension. | `2D Empty X Non-Square {1,3,4}`. |
| 6 | BFS dispatch | X-only line. | *Not directly tested. No one-dimensional fixture exists.* |
| 7 | BFS dispatch | Y-only line. | *Not directly tested. No one-dimensional fixture exists.* |
| 8 | BFS dispatch | Z-only line. | *Not directly tested. No one-dimensional fixture exists.* |
| 9 | BFS dispatch | Single voxel, including its hole-fill boundary guard. | *Not directly tested. No single-voxel fixture exists.* |
| 10 | BFS selection | Expand through valid true neighbors; skip invalid, false, and visited neighbors. | Archive; all three non-square cases. |
| 11 | BFS selection | Keep the largest component and clear smaller components. | Archive; all three non-square cases. |
| 12 | BFS filling | `FillHoles=false`. | Archive — `whole_*_nofill`; all three non-square cases. |
| 13 | BFS filling | Fill an enclosed 3D false component after resetting visited flags. | Archive — `whole_*_fill`, including the internal island. |
| 14 | BFS filling | Leave a boundary-connected false component unchanged. | Archive — `whole_*_fill`. |
| 15 | BFS filling | Fill an enclosed hole in a flat image. | *Not directly tested. The non-square fixtures disable hole filling.* |
| 16 | BFS selection | Equal-size whole-image components select the later component. | *Not directly tested. No fixture isolates an equal-size largest-component tie.* |
| 17 | Slice BFS | XY plane, filling disabled and enabled; contiguous plane transfers. | Archive — `sliced_xy_nofill`, `sliced_xy_fill`. |
| 18 | Slice BFS | XZ plane, filling disabled and enabled; contiguous row transfers. | Archive — `sliced_xz_nofill`, `sliced_xz_fill`. |
| 19 | Slice BFS | uint8 YZ plane, filling disabled and enabled; column batch transfers. | Archive — `sliced_yz_nofill`, `sliced_yz_fill`. |
| 20 | Slice BFS | Boolean YZ plane uses scalar transfers. | *Not directly tested. Slice fixtures use uint8 masks.* |
| 21 | Slice BFS | A full 64-column YZ batch followed by another batch. | *Not directly tested. The archive has only 16 X cells.* |
| 22 | Slice BFS | Reset visited flags and fill a removed internal island. | Archive — `sliced_*_fill`. |
| 23 | Slice BFS | Equal-size components select the later component. | *Not directly tested. No fixture isolates an equal-size largest-component tie.* |
| 24 | CCL dispatch | Whole-image labeling on resident bool and uint8 masks. | Archive — `whole_*`; all three non-square cases. |
| 25 | Slice CCL | XY planes with filling disabled and enabled. | Archive — `sliced_xy_nofill`, `sliced_xy_fill`. |
| 26 | Slice CCL | XZ planes with filling disabled and enabled. | Archive — `sliced_xz_nofill`, `sliced_xz_fill`. |
| 27 | Slice CCL | YZ planes with filling disabled and enabled. | Archive — `sliced_yz_nofill`, `sliced_yz_fill`. |
| 28 | CCL selection | Whole image has no true component; return without replay. | *Not directly tested. No all-false whole-image fixture exists.* |
| 29 | Slice CCL | An empty plane returns without component removal. | Archive — slices outside all documented features. |
| 30 | CCL selection | Create labels, merge connected labels, and replay to remove smaller roots. | Archive; all three non-square cases. |
| 31 | CCL filling | Label false components, mark boundary roots, and fill enclosed roots. | Archive — `whole_*_fill` and `sliced_*_fill`. |
| 32 | CCL filling | A flat whole image marks every false root as boundary-connected. | *Not directly tested. Flat fixtures disable hole filling; see `IdentifySampleFilter-D6`.* |
| 33 | CCL storage | HDF5 mask and equivalences cross a 4096-record page boundary. | *Not directly tested. The hidden `genuine HDF5 equivalence-page and slice oracle` requires a real HDF5 store; the supplied CI evidence covers in-core builds.* |
| 34 | CCL storage | A failed temporary-store request falls back to resident records. | *Not directly tested. No fixture isolates provider failure and fallback.* |
| 35 | CCL storage | Provider failure without fallback, or null store (`-45460`). | *Not directly tested. No fixture injects these provider responses.* |
| 36 | CCL storage | Maximum label count overflows the sentinel slot (`-45461`). | *Not directly tested. Requires an image count of UINT64_MAX.* |
| 37 | CCL storage | Boundary-root cache allocation fails (`-45462`). | *Not directly tested. No fixture injects allocation failure.* |
| 38 | Storage | Mask reads or writes fail in BFS slice transfers or CCL scans and replay. | *Not directly tested. No mask store injects an I/O failure.* |
| 39 | CCL storage | Equivalence or root-flag creation, access, or flush fails. | *Not directly tested. No temporary store injects these failures.* |
| 40 | Cancellation | Return during BFS scanning, slice processing, CCL labeling, or replay. | *Not directly tested. No fixture injects cancellation.* |
| 41 | CCL selection | Equal-size roots select the larger root label. | *Not directly tested. No fixture isolates a tie between largest roots.* |
| 42 | CCL dispatch | One-dimensional or single-voxel whole image. | *Not directly tested. The smallest flat fixture is a 3×4 plane.* |
| 43 | Slice CCL | Boolean masks in XY, XZ, or YZ planes. | *Not directly tested. All slice fixtures use uint8 masks.* |
| 44 | Slice BFS | Boolean masks in XY or XZ planes. | *Not directly tested. All slice fixtures use uint8 masks.* |

## Test inventory

The five ordinary cases in `src/Plugins/SimplnxCore/test/IdentifySampleTest.cpp` run in the supplied in-core CI configurations. All pass in CI.

| Test case | Status | Notes |
|-----------|--------|-------|
| `SimplnxCore::IdentifySampleFilter` | kept | Compares every uint8 mask value with `Mask Exemplar` in 12 archive sections: whole/sliced × XY/XZ/YZ × fill/nofill. Checks tuple dimensions. |
| `SimplnxCore::IdentifySampleFilter: genuine HDF5 equivalence-page and slice oracle` | new-for-V&V | Hidden. Builds a 129×129×3 mask and checks every value with filling off/on, removal of disconnected components, and the CCL dispatch counter. |
| `SimplnxCore::IdentifySampleFilter: SIMPL Backwards Compatibility` | kept | Dynamic variants `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` check identity, comments, and three converted arguments. |
| `SimplnxCore::IdentifySampleFilter: 2D Empty Z Non-Square {3,4,1}` | kept | Checks all 12 bool values against the four-cell analytical component in the XY plane. |
| `SimplnxCore::IdentifySampleFilter: 2D Empty Y Non-Square {3,1,4}` | kept | Checks the same analytical component in the XZ plane. |
| `SimplnxCore::IdentifySampleFilter: 2D Empty X Non-Square {1,3,4}` | kept | Checks the same analytical component in the YZ plane. |

## Exemplar archive

- **Archive:** `identify_sample_v2.tar.gz`
- **SHA512:** `a7ffac3eaad479c07215c1dd16274c45a52466708a9d27b5f85a29b0eba3b6705b627e1052a7a27e9bfe89cd6e7df673beb7a1e98b262b6c52ea383b4848ac31`
- **Provenance:** `src/Plugins/SimplnxCore/vv/provenance/IdentifySampleFilter.md`

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/IdentifySampleFilter.md` for the root cause, affected users, and recommendation. Part 1 contains the following source-confirmed difference.

| Deviation | Observed difference |
|-----------|---------------------|
| `IdentifySample-D1` | Whole-image BFS fills enclosed holes on 2D images; DREAM.3D v6.5.171 treats every cell on a flat axis as a boundary cell. |
