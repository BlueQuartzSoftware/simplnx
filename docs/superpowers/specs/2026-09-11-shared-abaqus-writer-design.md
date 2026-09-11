# Shared Abaqus Writer Design

**Date:** 2026-09-11
**Status:** Approved design, pre-implementation
**Filters:** `SimplnxCore::WriteAbaqusHexahedronFilter`, `SimplnxCore::WriteAbaqusCrystalPlasticityFilter`

## Purpose

The two Abaqus filters independently implement the same voxel-mesh export: node generation, hexahedral connectivity, grain bucketing, element sets, solid sections, master-file includes, progress reporting, cancellation, and atomic file commits. Their output formats have drifted even though they represent the same mesh.

This change replaces both algorithm implementations with one internal Abaqus input-deck writer. The two public filters remain separate because they serve different workflows: the Hexahedron filter exports a mesh whose material definitions are completed externally, while the Crystal Plasticity filter writes per-grain user-material cards.

## Decisions

- Keep both public filters, names, UUIDs, and legacy JSON converters.
- Use the current Crystal Plasticity mesh representation as the canonical format.
- Require scalar `int32` Cell Phases in both filters.
- Keep Write Dummy Node as a Hexahedron-only option.
- Change the Hexahedron default element type from `C3D8` to reduced-integration `C3D8R`.
- Permit the resulting Hexahedron output and pipeline compatibility breaks.
- Keep the Crystal Plasticity output contract unchanged.
- Record the intentional Hexahedron changes in `src/Plugins/SimplnxCore/vv/deviations/WriteAbaqusHexahedronFilter.md`.

## Architecture

### Shared algorithm

Add one internal `WriteAbaqusInputDeck` algorithm under `SimplnxCore/Filters/Algorithms`. Both filter `executeImpl()` methods construct its input values and invoke it directly. Remove the two independent writer algorithm implementations after their filter adapters have migrated.

The common input contains:

- output directory, file prefix, and job name;
- integration type and hourglass stiffness;
- Image Geometry, Cell Feature IDs, and Cell Phases paths;
- the optional dummy-node setting;
- an optional crystal-plasticity material configuration.

The crystal-plasticity configuration contains the Cell Euler Angles path, dependent-variable count, user-output-variable count, and material constants. Its presence causes the master writer to append one user-material card per grain. Its absence produces the Hexahedron master file with geometry includes only.

### Shared validation

A shared preflight helper validates that Cell Feature IDs and Cell Phases have one tuple per Image Geometry cell. Crystal Plasticity performs its additional Euler-angle tuple-count and material-parameter validation after the common validation.

The Hexahedron filter adds a required `Cell Phases` array-selection parameter and increments `parametersVersion()`. Its legacy JSON converter cannot infer this new path. Converted legacy pipelines therefore retain an empty phase selection and fail preflight until the user selects an array. No phase value is invented or derived from Feature IDs.

### Grain data

The shared algorithm uses the current Crystal Plasticity two-pass contiguous bucketing implementation. Positive Feature IDs are grouped by grain without rescanning every cell for every grain. Feature ID zero and negative IDs are excluded from grain element sets. All cells remain present as mesh elements and therefore remain members of `ALLELEMENTS`.

For each positive grain, the last cell in linear index order supplies its phase ID. Missing IDs in the range from one through the maximum positive Feature ID produce empty sets and phase-zero section/material names, with one warning. Inputs with no positive Feature IDs return an error before any destination file is committed.

## Canonical file contract

Both filters write the same five paths:

- `<prefix>_nodes.inp`
- `<prefix>_elems.inp`
- `<prefix>_sects.inp`
- `<prefix>_elset.inp`
- `<prefix>.inp`

The common output uses:

- `*NODE, NSET=ALLNODES` and coordinates with three fractional digits;
- conventional Abaqus C3D8 local node ordering;
- `*ELEMENT, TYPE=C3D8R|C3D8, ELSET=ALLELEMENTS`;
- `Grain<id>_Phase<phase>_set` element-set names;
- `Grain<id>_Phase<phase>_mat` material references;
- nodes, elements, sections, and element sets in that master-file include order;
- `*Hourglass Stiffness` only for `C3D8R` sections.

When Hexahedron's Write Dummy Node is enabled, the shared node writer appends one unconnected node at `(0, 0, 0)`. This is the only intentional difference in the common mesh files when both filters otherwise receive equivalent inputs and settings.

The Crystal Plasticity master file additionally writes the existing `*Material`, `*Depvar`, `*User Material`, and `*User Output Variables` records. Euler angles remain converted from radians to degrees, and the material-constant wrapping rules remain unchanged.

## Failure and transaction behavior

The shared implementation retains the Crystal Plasticity writer's behavior:

- create all five outputs through `AtomicFile`;
- remove temporary files on cancellation or failure;
- distinguish open failures from write failures;
- commit destination files only after all five temporary files are complete;
- check cancellation during grain preparation and each file-writing phase;
- use throttled progress messages for long-running phases.

The output directory must exist before execution.

## Testing strategy

Implementation follows test-driven development. Before production changes, add a cross-filter parity test that runs both filters on the same small Image Geometry with identical Feature IDs, Cell Phases, integration type, stiffness, and dummy-node disabled. It compares `_nodes.inp`, `_elems.inp`, `_sects.inp`, and `_elset.inp` byte-for-byte. This test fails against the current independent writers and passes only after canonicalization.

Update the Hexahedron tests to:

- provide Cell Phases for successful execution;
- pin canonical node ordering, formatting, set names, section names, and `C3D8R` default output;
- verify the dummy node remains optional;
- verify mismatched Cell Phases tuple counts fail preflight;
- verify converted legacy pipelines have no inferred phase path and fail preflight until it is supplied.

Keep the existing Crystal Plasticity byte-level tests as regression pins. They must remain unchanged unless a test-only path adjustment is necessary for the cross-filter fixture. Run the focused Abaqus tests serially, then the relevant SimplnxCore test target. Format every touched C++ file with the repository-pinned clang-format and verify with `--dry-run --Werror`.

## Documentation and deviations

Update `WriteAbaqusHexahedronFilter.md` for the required Cell Phases input, canonical file syntax, phase-qualified names, reduced-integration default, and retained dummy-node behavior.

Create `vv/deviations/WriteAbaqusHexahedronFilter.md` from the repository deviation template with these stable entries:

- `WriteAbaqusHexahedronFilter-D1`: Cell Phases is now required; converted legacy pipelines fail preflight until a phase array is selected.
- `WriteAbaqusHexahedronFilter-D2`: the default element type changes from `C3D8` to `C3D8R` and reduced-integration sections include hourglass stiffness.
- `WriteAbaqusHexahedronFilter-D3`: nodes, element connectivity, set names, precision, headers, and include order use the canonical shared format and are not byte-compatible with earlier Hexahedron output.

This task creates the deviation sidecar requested for migration guidance. A complete V&V report and exemplar provenance package are outside this refactor's scope.

## Files expected to change

- Create the shared writer header and implementation under `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/`.
- Modify both Abaqus filter headers and implementations to use the shared writer.
- Remove the superseded Hexahedron and Crystal Plasticity algorithm files.
- Update `src/Plugins/SimplnxCore/CMakeLists.txt` for the shared source.
- Update both Abaqus test files as needed for the parity fixture and new Hexahedron contract.
- Update the Hexahedron filter documentation.
- Create the Hexahedron deviation sidecar.

## Out of scope

- Combining the two public filters into one UI filter.
- Changing either filter UUID or legacy filter mapping.
- Adding crystal-plasticity material parameters to the Hexahedron UI.
- Defining Hexahedron material cards automatically.
- Creating a complete three-artifact V&V package.
- Repackaging or publishing exemplar archives unless an existing Hexahedron test proves that an archive update is unavoidable.

## Definition of done

- Both filters invoke one shared Abaqus writer implementation.
- Equivalent inputs produce byte-identical common mesh files when dummy-node output is disabled.
- Crystal Plasticity's existing byte-level output remains unchanged.
- Hexahedron requires Cell Phases and emits the canonical phase-qualified format.
- Dummy-node output still works.
- The new parity and migration tests pass with the focused Abaqus suites.
- Hexahedron documentation and deviation entries describe every intentional user-visible break.
- All touched C++ files pass the pinned clang-format verification.
