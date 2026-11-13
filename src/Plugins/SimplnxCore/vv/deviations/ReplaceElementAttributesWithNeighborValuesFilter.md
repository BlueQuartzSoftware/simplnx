# Deviations from DREAM3D 6.5.171: ReplaceElementAttributesWithNeighborValuesFilter

| Field | Value |
|-------|-------|
| SIMPLNX UUID | `65128c53-d3be-4a69-a559-32a48d603884` |
| SIMPL UUID   | `17410178-4e5f-58b9-900e-8194c69200ab` |
| Comparison run | 2026-07-21 (initial); re-run 2026-07-21 after algorithm refactor |
| Full comparison details | `replace_element_attributes_vv/comparison_report.md` |

## Historical comparisons

A/B run confirmed **bit-identical output** between SIMPLNX and DREAM3D 6.5.171 on all tested configurations. The comparison was re-run after the algorithm was refactored (`T best` replacing `float32 best`, `bestNeighbor` reset between iterations, precomputed `neighborAMArrays` replacing per-copy `dynamic_cast`) and produced the same results.

| Test case | Comparison | Result |
|-----------|------------|--------|
| LessThan (index=0), threshold=0.5, loop=true   | 6.5.171 vs NX    | Bit-identical (125/125 voxels, ConfidenceIndex + Marker) |
| GreaterThan (index=1), threshold=0.5, loop=true | 6.5.171 vs NX   | Bit-identical (125/125 voxels, ConfidenceIndex + Marker) |
| LessThan, threshold=0.5, loop=true              | 6.5.171 vs 6.5.172 | Bit-identical (125/125 voxels) |

The tested float32 inputs produced the same output after the refactor.

## Stalled-loop termination (#1806)

When *Loop Until Gone* is enabled, a complete pass with failing cells and no selected replacement returns error **-15002**. The former loop repeats unchanged passes for this input. The diagnostic names the comparison array, remaining count, and threshold, and suggests a parameter change. Earlier tuple writes remain applied. One-pass mode and the existing between-pass cancellation behavior retain their contracts.

The historical A/B comparisons above do not cover this termination deviation. Inline analytical regressions cover four cells without donors under both comparisons, repeated complete-tuple propagation, partial progress before a disconnected region stalls, and cancellation. The hidden real-store regression also requires actual out-of-core stores and backing files.
