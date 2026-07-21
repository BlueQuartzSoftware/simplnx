# Deviations from DREAM3D 6.5.171: ReplaceElementAttributesWithNeighborValuesFilter

| Field | Value |
|-------|-------|
| SIMPLNX UUID | `65128c53-d3be-4a69-a559-32a48d603884` |
| SIMPL UUID   | `17410178-4e5f-58b9-900e-8194c69200ab` |
| Comparison run | 2026-07-21 (initial); re-run 2026-07-21 after algorithm refactor |
| Full comparison details | `replace_element_attributes_vv/comparison_report.md` |

## No deviations observed

A/B run confirmed **bit-identical output** between SIMPLNX and DREAM3D 6.5.171 on all tested configurations. The comparison was re-run after the algorithm was refactored (`T best` replacing `float32 best`, `bestNeighbor` reset between iterations, precomputed `neighborAMArrays` replacing per-copy `dynamic_cast`) and produced the same results.

| Test case | Comparison | Result |
|-----------|------------|--------|
| LessThan (index=0), threshold=0.5, loop=true   | 6.5.171 vs NX    | Bit-identical (125/125 voxels, ConfidenceIndex + Marker) |
| GreaterThan (index=1), threshold=0.5, loop=true | 6.5.171 vs NX   | Bit-identical (125/125 voxels, ConfidenceIndex + Marker) |
| LessThan, threshold=0.5, loop=true              | 6.5.171 vs 6.5.172 | Bit-identical (125/125 voxels) |

The NX implementation is a direct port. The refactor changed internal mechanics only — output is invariant to those changes for float32 arrays.
