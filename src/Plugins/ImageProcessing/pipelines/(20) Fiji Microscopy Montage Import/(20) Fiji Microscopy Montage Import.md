# Fiji Microscopy Montage Import

Executable pipeline: `(20) Fiji Microscopy Montage Import.d3dpipeline`

> **Validation warning:** Do not treat this example as a validated acceptance procedure. Adapt and validate it for the scanner, material, resolution, and quality requirement.

## Purpose and real-world setting

A microscopist acquires a large field as overlapping high-magnification tiles and registers them in Fiji. The pipeline brings the registered tile layout into DREAM3D-NX without flattening the tiles into one irreversible image.

Imports a Fiji registered tile configuration and its image tiles while preserving tile positions and metadata in a DREAM3D-NX data group.

## When to use this workflow

- Import a Fiji Grid/Collection Stitching result
- Preserve tile positions for later montage analysis
- Transfer a registered large-area microscopy acquisition into DREAM3D-NX


## Input data and assumptions

- The Fiji configuration paths resolve relative to the included tile files.
- The configuration uses translations that match the imported image coordinate system.
- Registration quality was reviewed in Fiji before import.

<!-- INPUT-DATA-STRATEGY:BEGIN -->
### Input source strategy

Strategy: `measured-compatible-fixture`.

Paper visual target: Overlapping microscopy tiles with a registered Fiji tile configuration and preserved tile positions.

Target traits:

- measured microscopy tiles
- overlap
- registered translations
- tile metadata

Known differences: The paper's exact confocal acquisition is unavailable. Existing measured BlueQuartz tiles are retained because they exercise the same Fiji registration contract.

See `Data/ImageProcessing_Examples/Provenance.json` for per-file transformations, checksums, dimensions, spacing, and licensing.
<!-- INPUT-DATA-STRATEGY:END -->

## Data flow

Fiji registered tile configuration plus image tiles -> positioned tile geometries and metadata group -> DREAM3D file.

The `.d3dpipeline` file is authoritative for executable arguments. The matching `.yaml` file repeats the ordered steps in a structured form for software, LLM, and MCP use.

## Step-by-step filter explanation and parameter rationale

### 1. `ImportFijiMontageFilter`

Import the registered microscopy tiles and preserve their positions and metadata.

Spacing and unit settings control the physical meaning of distance, size, and position.

Key parameters:

- `cell_attribute_matrix_name`: `"Cell Data"`
- `change_image_data_type`: `false`
- `change_origin`: `false`
- `color_weights`: `[0.21250000596046448,0.715399980545044,0.07209999859333038]`
- `convert_to_gray_scale`: `false`
- `data_container_path`: `"Tile"`
- `data_group_name`: `"Fiji Montage"`
- `image_data_array_name`: `"Image"`
- `image_data_type_index`: `0`
- `input_file`: `"Data/ImageProcessing_Examples/io/fiji/TileConfiguration.registered.txt"`
- `length_unit_index`: `6`
- `origin`: `[0.0,0.0,0.0]`
- `parent_data_group`: `true`

Resolved inputs:

- `input_file` -> `"Data/ImageProcessing_Examples/io/fiji/TileConfiguration.registered.txt"`

### 2. `WriteDREAM3DFilter`

Save the complete workflow result.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `compression_level`: `5`
- `export_file_path`: `"Data/Output/ImageProcessing_Examples/16_Fiji_Montage/fiji_montage.dream3d"`
- `use_compression`: `true`
- `write_xdmf_file`: `false`

Declared outputs:

- `export_file_path` -> `"Data/Output/ImageProcessing_Examples/16_Fiji_Montage/fiji_montage.dream3d"`

## Outputs and how to interpret them

- `Data/Output/ImageProcessing_Examples/16_Fiji_Montage/fiji_montage.dream3d`

Intermediate arrays remain in the final DREAM3D file. Compare them in order. A plausible final mask does not prove that every upstream assumption is correct.

## How to adapt the pipeline

- Keep the configuration file and tile names synchronized.
- Select grayscale conversion only when color channels are not scientifically meaningful.
- Check overlap seams and landmark continuity after import.

Keep input and output paths relative when the pipeline must run from a DREAM3D-NX Anaconda environment or an installed application bundle.

## Failure modes and quality checks

- Renamed or moved tiles break configuration references.
- Incorrect stage units or scale create wrong physical placement.
- A montage can load correctly even when the upstream registration is poor.
- Review intermediate arrays, not only the final writer output.
- Compare measurements with an independent reference before production use.

## Scientific basis and annotated references

Workflow reproduction status: `method_reproduction`.

The example imports a tested Fiji tile configuration and reproduces the registered-montage data flow, not a named paper figure.

Paper dataset availability: The paper describes microscopy acquisitions but does not provide one canonical tile set for exact reproduction.

Dataset license: No separate canonical dataset license is stated.

Original or paper-linked dataset included in the example archive: `no`.

1. Stephan Preibisch, Stephan Saalfeld, Pavel Tomancak. “Globally optimal stitching of tiled 3D microscopic image acquisitions.” *Bioinformatics* (2009). [10.1093/bioinformatics/btp184](https://doi.org/10.1093/bioinformatics/btp184). Primary scientific basis for Fiji tile registration and global montage placement.
2. Johannes Schindelin, Ignacio Arganda-Carreras, Erwin Frise, Verena Kaynig, Mark Longair, Tobias Pietzsch, Stephan Preibisch, Curtis Rueden, Stephan Saalfeld, Benjamin Schmid, Jean-Yves Tinevez, Daniel J. White, Volker Hartenstein, Kevin Eliceiri, Pavel Tomancak, Albert Cardona. “Fiji: An open-source platform for biological-image analysis.” *Nature Methods* (2012). [10.1038/nmeth.2019](https://doi.org/10.1038/nmeth.2019). Scientific reference for the Fiji ecosystem that produces the montage configuration.

## Guidance for an LLM or MCP assistant

Use this document to explain intent and tradeoffs. Use the `.yaml` sidecar to retrieve exact structured metadata. Use the `.d3dpipeline` file as the executable source of truth, and use the `.py` companion for Anaconda execution.

Before adapting this workflow, ask:

- What imaging system, material, and acquisition mode produced the data?
- What are the voxel or pixel spacing and physical units?
- What is the smallest feature that must be retained or reported?
- Which independent measurement or labeled dataset will validate the result?

Constraints:

- The Fiji configuration paths resolve relative to the included tile files.
- The configuration uses translations that match the imported image coordinate system.
- Registration quality was reviewed in Fiji before import.
- Keep all file paths relative to the DREAM3D-NX application binary directory.
- Run preflight and execute the complete pipeline after any parameter or DataPath change.
- Treat example parameter values as starting points, not production acceptance limits.

Related examples:

- `(19) Serial-Section Stack Reconstruction.d3dpipeline`

When a user asks for a new pipeline, preserve DataPath consistency between steps, validate every mathematical domain, and run the result end to end with representative data.
