# Align Sections (List)

## Group (Subgroup)

Reconstruction (Alignment)

## Description

This **Filter** applies precomputed cell shifts to each section of an **Image Geometry**. Unlike the other alignment filters (which calculate shifts automatically), this filter takes a set of known X and Y shifts as input and applies them directly. This is useful for:

- Applying shifts that were computed by another alignment filter in a previous run
- Applying user-measured or externally-calculated alignment corrections
- Re-applying a known good alignment to a dataset

### Shift Units

All shift values are in **cell (voxel) units**, not physical units. A shift of 3 means move the section by 3 cells, regardless of the cell's physical size (spacing). The shifts must be integers.

### Relative vs. Cumulative Shifts

The input shifts can be provided in either format:

- **Relative [0]**: Each shift describes the offset of section N relative to section N-1. The filter internally converts these to cumulative shifts before applying them.
- **Cumulative [1]**: Each shift describes the absolute offset of section N from a fixed reference point.

### Preparing Shift Data from a File

If your shifts are stored in an external file, you need to import them into a DataArray before using this filter:

**Requirements:**
- The number of shift entries must equal the number of slices (Z dimension of the geometry)
- Slices must be ordered from bottom to top

**Recommended file format** (space-delimited, two values per line):

```console
xshift yshift
xshift yshift
xshift yshift
...
```

**To import this format:**

1. Use the [Read Text Data Array](ReadTextDataArrayFilter.md) filter
2. Set *Input Numeric Type* to `signed int 64 bit`
3. Set *Number of Components* to `2`
4. Set *Delimiter* to space
5. Set the *Data Array Dimensions* to match the Z dimension of your geometry

For more complex file formats (e.g., legacy DREAM3D alignment files with 6 columns), use the [Read CSV File](ReadCSVFileFilter.md) filter instead. See the backwards compatibility example pipeline for details.

### Note on Legacy Files

Alignment shift files from DREAM3D/DREAM3DNX versions 7.0.3 and earlier used a 6-column format with both relative and cumulative shifts. If using these files, import via the [Read CSV File](ReadCSVFileFilter.md) filter and select the appropriate columns. Files produced by the legacy `AlignSectionsFeatureCentroid` contained zeros for relative shifts -- use cumulative shifts for those.

### Required Input Sources

- **Cell Shifts Array** -- a two-component int64 array with one entry per Z-slice. Import this from an external file using [Read Text Data Array](ReadTextDataArrayFilter.md) (simple format) or [Read CSV File](ReadCSVFileFilter.md) (complex/legacy format); or pass the *Relative Shifts* / *Cumulative Shifts* array produced by another alignment filter such as [Align Sections (Feature Centroid)](AlignSectionsFeatureCentroidFilter.md), [Align Sections (Misorientation)](../OrientationAnalysis/AlignSectionsMisorientationFilter.md), or [Align Sections (Mutual Information)](../OrientationAnalysis/AlignSectionsMutualInformationFilter.md).

% Auto generated parameter table will be inserted here

## Example Pipelines

- align_sections_user_input
- align_sections_no_file (meant for visualizing)
- align_sections_backwards_compatibility

Note the backwards compatibility also demonstrates how it a more complex user input could be done as well (Using the Read CSV Data Filter).

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
