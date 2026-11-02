# Defect Hunt — Finding Bugs, Not Only Documenting Behavior

The other V&V phases show that a filter agrees with an oracle on the fixtures that the engineer chose. They do not show that the engineer chose fixtures that can find bugs. The engineer designs the oracle after reading the code, so the fixtures tend to exercise the cases that the code already handles. The defect hunt is an adversarial pass with one goal: find an input that makes the filter produce a wrong result.

**Motivating example — issue #1758.** In `AlignGeometries`, the Centroid branch computes `{t[0]-m[0], t[0]-m[0], t[0]-m[0]}`, so the X offset is applied on all three axes. The Origin branch next to it uses the correct indices. The tests run only the Origin mode. A fixture with the same offset on every axis also passes. Each of parts 1–4 below finds this defect independently.

## When

Do the defect hunt after `vv-discover` has listed the code paths and before the oracle fixtures are final. Its findings change the fixture design. Do the hunt again when a rebase or a fix changes the algorithm.

The defect hunt does not replace the second-engineer review of the oracle. It is the author's own attempt to make the filter fail.

## Part 1 — Bug-pattern review

Read every line of the algorithm, its helpers, and the filter's `preflightImpl()`/`executeImpl()`. For each pattern, look for a match. Then do a separate pass in which you compare sibling code side by side.

| Pattern | What to look for |
|---|---|
| **Copy-paste drift** | Per-axis or per-component expressions in which the index does not advance (`[0],[0],[0]`, `x, x, z`). Sibling branches (mode A vs. mode B, X vs. Y, in-core vs. OOC) that compute the same quantity in different ways. Put sibling branches side by side and compare them token by token. |
| **Index and stride** | X/Y/Z order. Tuple index vs. component index. Strides (`dims[0]*dims[1]`). An internal iteration order that differs from the physical order the user sees (for example, a slice loop that runs from the top down, then is indexed by a user "slice k"). |
| **Bounds and off-by-one** | `>` vs. `>=`. A loop that starts at 1 and skips element 0 or the anchor. The last slice, row, or feature. Empty and single-element inputs. |
| **Numerics** | Truncation where the code must round. Signed/unsigned wrap. Narrowing casts. Division by zero. A NaN cast to an integer (undefined behavior). Accumulation in `float` where the code needs `double`. |
| **Uninitialized or stale state** | Output tuple 0. Buffers that the code reuses between features or slices and does not reset. Accumulators declared outside the loop. |
| **Wrong object** | The wrong array, variable, or path among similar names. A parameter that the code reads but does not use. A result written to the wrong output. |
| **Discarded results** | A `Result<>` or warning that the code ignores. An error code that two different failures use. |
| **Mode gating** | A parameter that has no effect in a mode where it must have an effect. A mode branch that no test executes. |
| **Component shape not declared** | The algorithm reads an input DataArray with a fixed number of components (`a[2*i+1]`, `a[3*i+k]`, `getComponent(i, 2)`), but the `ArraySelectionParameter` or `MultiArraySelectionParameter` that selects it declares no `AllowedComponentShapes`, or declares a different one. The fix belongs in the parameter (`AllowedComponentShapes{{N}}`), because parameter validation then rejects other shapes (−208) before preflight runs. When N depends on another parameter or has variable dimensions, preflight must check `getComponentShape()` instead. Example: WriteStlFile's phases parameter declared `{{1}}` while the algorithm read 2 components. |
| **Parent Attribute Matrix not checked** | An element-level input DataArray (one value per cell, vertex, edge or face) is not checked to be a child of the geometry's element Attribute Matrix. A selection parameter accepts a DataPath into **any** Attribute Matrix, so a DataArray with the wrong number of tuples is a reachable input, not corrupt data. The fix is a preflight check with `IsChildOfAttributeMatrix` against `getCellData()` (Image or Rectilinear Grid Geometry) or the Vertex, Edge, Face or Polyhedra Attribute Matrix of a node geometry. `AttributeMatrix` only accepts children whose tuple count matches its own. Use the accessors that return a pointer; the `*Ref()` accessors and `getCellDataPath()` throw when the Attribute Matrix is missing. |
| **Feature-level bound** | A Feature Attribute Matrix has largest Feature Id + 1 tuples, because id 0 is reserved. The largest Feature Id depends on the array values, so preflight cannot check it: execute must check it, or size internal buffers from the Feature Attribute Matrix tuple count. Look for buffers sized by the largest Feature Id but read by the Feature Attribute Matrix tuple count, and the reverse. Also look for sparse or negative ids used directly as indices. |

**Where each check belongs.** Structure (component shape, DataType) goes in the parameter: parameter validation runs before the filter's preflight, so a repeated check in preflight or execute is dead code, and an existing one is removed. Relationships between inputs (tuple count against a sibling DataArray, parent Attribute Matrix) go in preflight. Only value-dependent bounds go in execute.

### SIMPLNX vocabulary for findings

State every finding in SIMPLNX terms, so that the reader can see how the input is reachable.

- **Attribute Matrix kinds:** *element* Attribute Matrices (*Vertex Data*, *Edge Data*, *Face Data*, *Cell Data*), whose tuple count equals the number of those elements; *Feature* Attribute Matrices (for example *Cell Feature Data*), with largest Feature Id + 1 tuples; *Ensemble* Attribute Matrices (for example *Cell Ensemble Data*).
- **Data objects:** DataArray, StringArray, NeighborList, DataPath. Say "number of tuples", "tuple shape", "number of components", "component shape" and DataType.
- **Geometries:** Image, Rectilinear Grid, Vertex, Edge, Triangle, Quad, Tetrahedral and Hexahedral Geometry. Say "number of cells" or "number of faces".
- **Parameters:** name a parameter by its display name in bold, for example **Create Bounding Box Geometries**, and say whether the failure is at parameter validation, preflight or execute.

Write "The Feature Ids DataArray is not checked to be a child of the Image Geometry's Cell Data Attribute Matrix, so its number of tuples can differ from the number of cells". Do not write "Feature Ids may not have one value per cell".

For each suspected defect, record `file:line`, the input that triggers it, and the expected and actual results. A suspicion is not a finding. Confirm it with a failing test or a hand calculation, or discard it. Every confirmed defect becomes a `<FilterName>-D<N>` deviation entry and a regression test. A defect is a deviation even when 6.5.171 has the same defect.

## Part 2 — Mode coverage

Make a list of every value of every `ChoicesParameter`, every `BoolParameter` state that changes the output, and every dispatched algorithm variant. For each item, cite a test that **asserts output values** in that mode. A mode that only "runs without error" is not covered. Put each uncovered mode in the Code path coverage table as a gap with its reason. Do not omit it.

## Part 3 — Symmetry-breaking fixtures

A fixture can detect a swap of two axes, components, or features only if the swap changes the expected output. Design each oracle fixture so that:

- each axis has a different value: offsets, dimensions (`X ≠ Y ≠ Z`), spacing, and a non-zero origin;
- each component of a multi-component value is different (for example, a translation of (1, 2, 3), not (1, 1, 1));
- there are at least two features or phases, and their values are different;
- masks, feature shapes, and positions are not symmetric about the center of the geometry; and
- the fixture contains values that make truncation and rounding give different results, and values that have a sign that the filter must keep.

If a fixture cannot break a symmetry (for example, the filter supports only cubic inputs), write the reason in the Oracle section.

## Part 4 — Mutation check

This part tests the tests. For each row in the Code path coverage table, make at least one small mutant of the source in that path:

- swap an index (`[1]` → `[0]`);
- reverse a comparison (`<` → `<=`, `>` → `>=`);
- change a bound by one;
- remove a guard or branch; or
- change a constant.

Apply one mutant at a time. Rebuild, run the filter's tests, then restore the source. A mutant is **killed** when at least one test fails. A **surviving** mutant shows a path that the tests do not check. Add an assertion that kills it, or record why the mutant is equivalent (it cannot change the output).

Record the result as `K of N mutants killed` and list each survivor with its disposition. The `vv-tests` skill packages a helper that applies, builds, tests, and restores each mutant.

## Part 5 — Metamorphic relations (recommended)

A metamorphic relation connects the outputs of two runs. It does not need a hand-calculated expected value. Examples:

- **Translation:** move the input by (a, b, c) → the output positions move by (a, b, c), and the per-feature values stay the same.
- **Axis permutation:** exchange X and Y in the input → the output exchanges X and Y.
- **Relabel:** permute the feature IDs → the per-feature outputs are permuted in the same way.
- **Scale:** multiply the spacing by s → lengths multiply by s, areas by s², and volumes by s³.

Relations of this type find axis and index defects, and do not need a new oracle. Record them as Class 4 companions (see [`oracle_classes.md`](./oracle_classes.md)).

## Report

Fill the `## Defect hunt` section of the report:

- the patterns reviewed and the reviewer;
- a findings table with columns ID, Pattern, `file:line`, Trigger, Disposition (`fixed — <FilterName>-D<N>`, `not a defect — <reason>`, or `deferred — <issue link>`);
- mode coverage as `M of M modes value-asserted`, with the gaps;
- the mutation result as `K of N mutants killed`, with each survivor and its disposition; and
- the metamorphic relations used, or `None`.

"No defects found" is a valid result only when the section shows the patterns that were reviewed and the mutation result.
