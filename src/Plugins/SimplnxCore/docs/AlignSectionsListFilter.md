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
- Shift rows must be ordered from the top section to the bottom section

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

Legacy DREAM3D 6.5.171 shift files have six tab-separated columns: section, next section, relative X, relative Y, cumulative X, and cumulative Y. All shifts are in cells. These files have one row fewer than the number of sections. They have no row for the top section.

For *Cumulative* input, add a zero row at the start of the file, before importing it. Row 0 is the first data row, not a header. Set its cumulative X and Y shifts to zero. For the 117-section example, the added row is:

```text
116	117	0	0	0	0
```

Import all 117 rows with [Read CSV File](ReadCSVFileFilter.md). Use custom column names and start at the first data row. Combine the cumulative X and Y columns, in that order, into one two-component int64 array.

For a volume with N sections, array row 0 represents the top section, N - 1, which stays fixed. Row i supplies the cumulative shift for section N - 1 - i. The last row supplies the shift for section 0. Without the added zero row, the first legacy row is unused and the remaining shifts are assigned to the wrong sections.

See `Align_Sections_Backwards_Compatibility.d3dpipeline` in `Align_Sections_Examples` for the complete example. Its input file already includes the zero row. Do not add another row to that file.

Legacy `AlignSectionsFeatureCentroid` files contain zeros in the relative shift columns. Use the cumulative shift columns for those files.

### Required Input Sources

- **Cell Shifts Array** -- a two-component int64 array with one entry per Z-slice. Import this from an external file using [Read Text Data Array](ReadTextDataArrayFilter.md) (simple format) or [Read CSV File](ReadCSVFileFilter.md) (complex/legacy format); or pass the *Relative Shifts* / *Cumulative Shifts* array produced by another alignment filter such as [Align Sections (Feature Centroid)](AlignSectionsFeatureCentroidFilter.md), [Align Sections (Misorientation)](../OrientationAnalysis/AlignSectionsMisorientationFilter.md), or [Align Sections (Mutual Information)](../OrientationAnalysis/AlignSectionsMutualInformationFilter.md).

% Auto generated parameter table will be inserted here

## Example Pipelines

- align_sections_user_input
- align_sections_no_file (meant for visualizing)
- align_sections_backwards_compatibility

The backwards compatibility example also shows how to import a more complex input file with Read CSV File.

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
