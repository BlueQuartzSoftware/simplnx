# Align Sections (List)

## Group (Subgroup)

Reconstruction (Alignment)

## Description

This **Filter** will apply the precalculated cell shifts to each section of an Image Geometry. It allows for both *relative* or *cumulative* cell shifts. The difference between the two being the former is dependent on the previous slice's position. Under the covers, relative is translated to cumulative before applying shifts to the cells themselves. Previously, the only accepted input was utilizing relative shifts, so use those for backwards compatibility. See the **Handling User Created Shifts File** and **Example Pipelines** sections of this documentation for further hints.

### Input Array Type

The *Input Array Type* parameter controls how the shift values in the input arrays are interpreted:

- **Relative [0]**: Each shift value is a relative offset between adjacent sections. The shift for section N describes how far section N has moved compared to section N-1. These values are internally converted to cumulative shifts before being applied.
- **Cumulative [1]**: Each shift value is an absolute (cumulative) offset measured from a fixed reference point. The shift for section N describes how far section N has moved from that reference.

## Handling User Created Shifts File

In this section we will be covering how to get a user defined shift file into the algorithm. This has been included in the documentation for posterity, as the capability was removed in order to offer a less rigid file structuring and to centralize the I/O paradigm.

### File Information

**Requirements:**

- The number of shifts (`x` and `y` both) must be equivalent to the number of slices (Z dimension of the geometry)
- The slices must be ordered from the bottom of the sample to the top of the sample.

If using relative shifts for the algorithm, the first line of shifts should be the second slice relative to the first, then the next line is the third slice relative to the second, and so on

**Recommendations:**

The **user** created alignment file is recommended have the format as follows:

```console
xshift yshift
xshift yshift
xshift yshift
etc...
```

**Backwards Compatibility Information:**

For versions of 7.0.3 and below, the DREAM3D and DREAM3DNX produced had the following format:

```console
slice_m slice_m+1 newxshift newyshift xshifts yshifts
slice_n slice_n+1 newxshift newyshift xshifts yshifts
etc...
```

Where:

- `newxshift` and `newyshift` were the relative shifts
- `xshifts` and `yshifts` were the cumulative shifts.
- `xcentroid` and `ycentroid` were appended to the end for `AlignSectionsFeatureCentroid`

**Note: Previous implementations of `AlignSectionsFeatureCentroid` were printing zeros for the relative shifts, so if you are using a file produced by that you must use cumulative shifts.**

### Importing

Use "Read CSV File" (`ReadCSVFileFilter`) or "Read Text Data Array" (`ReadTextDataArrayFilter`) to get your data into an array.

**Recommendations:**

If you have a file matching the recommended **user** created alignement file above, the process can be expidited with "Read Text Data Array" (`ReadTextDataArrayFilter`).

To do so:

1. supply your file in the "Input Text File" parameter
2. set "Input Numeric Type" parameter to `signed int 64 bit`
3. set "Number of Components" parameter to `2`
4. change the "Delimiter" parameter to ` (space)` (index number `2` for python)
5. in the "Created Array Path" parameter, leave the path empty, and name it according to the type of shifts ("relative_shifts" or "cumulative_shifts")
6. get the tuple dims of the Z dimension of your geometry, and put it into the "Data Array Dimensions (Slowest to Fastest)" parameter

**Backwards Compatibility Information:**

For alignment shifts files from the previous version, you **must** use the "Read CSV File" (`ReadCSVFileFilter`). This is a complicated filter parameter wise, so see the documentation provided with the filter. It is recommended to store these in an Attribute Matrix with dimensions equivalent to that of the Z dimension of the target geometry.

### Conclusion

The imported data should be prepared and compatible with the Align Sections List input by now. If you have further questions see one of the example pipelines below.

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
