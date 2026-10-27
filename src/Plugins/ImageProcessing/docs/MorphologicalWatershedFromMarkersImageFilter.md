# Morphological Watershed From Markers Image Filter

Applies a marker-controlled (seeded) morphological watershed segmentation to a grayscale image.

## Group (Subgroup)

ImageProcessing (Watersheds / Segmentation)

## Description

Floods a grayscale image from user-supplied **markers**, which are integer seed labels. Each marked region grows over ascending gray levels until regions meet. The output labels identify the resulting basins. The markers control the initial regions; select them to represent the regions you want to separate. Marker IDs are dimensionless identities, not gray levels or array indexes.

Two flooding variants are provided via **Mark Watershed Line**:

- **On (Meyer's algorithm, the default):** watershed-line voxels where distinct basins meet are left as label 0, separating the basins.
- **Off (Beucher's algorithm):** reached voxels are assigned to a basin without a separating line. An image with no surviving nonzero seed remains unmarked.

**Fully Connected** selects the neighborhood used for flooding: face connectivity (6-connected in 3D, 4-connected in 2D) when off, or face+edge+vertex connectivity (26-connected in 3D, 8-connected in 2D) when on.

### Input Guidance

- **Input Cell Data (grayscale):** a single-component scalar image of any numeric type. The flood ascends this image's gray levels.
- **Marker Cell Data:** a single-component **integer** scalar image with the same number of tuples as the grayscale input. Nonzero voxels are markers/seeds; a value of 0 means "unmarked". Voxels sharing the same nonzero label belong to the same seed region, so two disjoint blobs with the same label flood into one basin.

### Output Interpretation

- A label image with the **same element type as the marker image**, the grayscale input's tuple and scalar component shapes, and the grayscale input's storage format. Watershed-line voxels (when Mark Watershed Line is on) are 0. Warn mode can change the original marker identities as described below.

### Marker ID Conversion Handling

The flood uses internal `uint32` labels. A **lossless round trip** means a native marker ID has the same value after conversion to `uint32` and back to its native type.

- All `int8`, `uint8`, `int16`, `uint16`, `int32`, and `uint32` values round-trip, including negative signed values and their minimum values.
- For `int64` and `uint64`, the lossless direct range is **0 through 4294967295, inclusive**. Negative `int64` IDs do not round-trip.
- Zero means unmarked. `UINT32_MAX`, or 4294967295, is a valid in-bounds seed. It is not a reserved marker identity.

**Marker ID Conversion Handling** has exactly two choices:

- **Error (default):** check values during the existing bounded marker-copy pass. If an ID changes, stop before writing its affected scratch chunk and before flooding starts. The error identifies the marker array, flat index, native ID, and restored value. Previously copied scratch chunks or allocated output storage can remain; this is not a general rollback guarantee.
- **Warn and Continue (Legacy Behavior):** keep the old conversion, flood, and conversion back. If any marker changes, return one warning with the number of changed occurrences observed and the first changed marker's context. Valid round-tripping markers do not warn. This choice does not preserve unsupported native IDs or suppress independent allocation and I/O errors.

For example, in Warn mode:

| Native marker | Internal and restored value | Consequence |
| --- | --- | --- |
| `uint64` 4294967296 | 0 | The seed disappears into background. |
| `uint64` 4294967297 | 1 | It can merge with an existing seed 1. |
| `int64` -1 | 4294967295 | The seed is renamed in the `int64` output. |

Seed loss, merging, or renaming can change basin boundaries and region sizes. Downstream feature counts and measurements can be incorrect for the original markers. Choose Warn only when you intentionally accept these consequences.

Existing pipelines that omit the new choice use Error. To retain old lossy behavior, explicitly select Warn. Converting the marker array first preserves IDs only when every value fits the destination type; arbitrary narrowing is not a lossless repair. Lossless remapping is future work and is not an available choice.

Copy, flood, and restore Results retain the conversion warning on normal completion and delivered later errors or cancellation. The generic filter wrapper can replace a successful cancelled execution Result and lose its warnings; warning retention at that inherited cancellation boundary is not guaranteed. Cancellation does not make a partial output a completed segmentation.

This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Morphological Watershed From Markers Image Filter.

## Algorithm

Resident input uses an in-memory hierarchical queue. The filter processes the lowest gray level first. It uses FIFO order for voxels at the same level.

The initial marker-copy pass checks the selected conversion policy using the same native values it copies. Transfers use at most 65536 marker values per chunk, with one native buffer and one `uint32` buffer. The check adds constant work per value and constant diagnostic state. It adds no extra image scan, remapping table, or resident copy of the full native marker volume. Internal marker/output scratch stores remain `uint32` and follow the grayscale storage format.

An out-of-core endpoint keeps the same processing order with bounded, file-backed state. The engine gets an adaptive working-memory grant from the shared cache budget. It uses only the useful part of that grant and keeps a disk-backed fallback. If a complete state fits, the engine can keep it resident. Otherwise, it keeps fixed record pages in memory. Large partial 3-D state uses 32 x 32 x 8 voxel tiles so face neighbors usually share one page.

Eight- and sixteen-bit gray types use ordered gray-level buckets. Each bucket stores 64-bit voxel indexes in file-backed FIFO blocks. Wider integers and floating-point types use a file-backed minimum heap keyed by flood level and insertion order. Marker values are streamed during initialization and are not kept in each voxel record. Input and output transfers use bounded bulk I/O.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
