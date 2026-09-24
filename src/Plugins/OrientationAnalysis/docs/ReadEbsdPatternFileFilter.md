# Read EBSD Pattern File

## Group (Subgroup)

IO (Input)

## Description

This filter reads raw electron backscatter diffraction (**EBSD**) detector patterns from an
EDAX `.up1` or `.up2` file into a new **Data Array**. Each tuple contains one complete detector
pattern. The pattern pixels form the component shape of the array.

The filename extension determines the output pixel type:

| Extension | Output type | Bytes per pixel |
|---|---|---:|
| `.up1` | `uint8` | 1 |
| `.up2` | `uint16` | 2 |

The filter reads the pixel type, pattern width, pattern height, payload position, and available
scan geometry from the file. Users do not specify these values. Pattern pixels remain in file
order. The component shape is `{pattern height, pattern width}`, from slowest to fastest
dimension.

### Output Tuple Shape

The output location controls the tuple shape:

- If the output is in an existing **Attribute Matrix**, the filter uses the Attribute Matrix
  shape. The total number of Attribute Matrix tuples must equal the number of imported patterns.
- For a version 3 UP file outside an Attribute Matrix, the filter uses the stored
  `{number of rows, number of columns}` scan shape.
- For a version 1 UP file outside an Attribute Matrix, the default tuple shape is
  `{number of patterns}` because version 1 does not store scan geometry.
- For a version 1 UP file, enable *Set Version 1 Scan Dimensions* to supply the number of rows
  and columns. Their product must equal the number of patterns in the file.

### Extra Patterns

A version 3 header can declare patterns beyond the rectangular scan grid. The meaning of these
patterns is not sufficiently documented. The filter imports the complete rectangular grid,
skips the declared extra patterns, and reports a warning. If the declared count does not agree
with the trailing payload size, the filter reports an additional warning and still imports the
complete grid.

### File Validation

The UP format has no magic number or checksum. The filter therefore validates the extension,
header version, pattern dimensions, data offset, checked payload size, and complete-pattern
count before it creates the output. Pattern width and height must each be from 12 through 4096
pixels. Header version 2 is rejected because it has no released layout. Header versions above
3 use the version 3 layout and produce a warning.

All serialized multibyte values are read as little-endian values. File offsets and payload sizes
use 64-bit arithmetic, which permits files larger than 2 GiB.

## Required Input Sources

None. This filter reads directly from an EDAX `.up1` or `.up2` file on disk.

## Created Outputs

The filter creates one numeric **Data Array** at the selected output path.

| Output | Type | Tuple shape | Component shape | Units |
|---|---|---|---|---|
| Output Pattern Data Array | `uint8` for `.up1`; `uint16` for `.up2` | Attribute Matrix shape, stored version 3 scan shape, supplied version 1 scan shape, or flat pattern count | `{pattern height, pattern width}` | Detector intensity counts |

The version 3 x and y scan steps are reported during preflight in micrometers. This filter does
not create coordinate arrays or a geometry from those values.

## Limitations

- Only EDAX `.up1` and `.up2` files are supported initially.
- Version 1 files do not contain scan rows, scan columns, or scan step values.
- The files do not contain absolute stage position, tile position, montage layout, detector
  geometry, accelerating voltage, camera settings, or timestamps.
- Hexagonal-grid metadata is reported, but this filter creates only the pattern array. It does
  not create offset scan coordinates.
- The filter reads the complete grid. Pattern subsets and rectangular read windows are not
  supported.

% Auto generated parameter table will be inserted here

## Example Pipelines

None.

## References

- U.S. Naval Research Laboratory,
  [`PyEBSDIndex/pyebsdindex/ebsd_pattern.py`](https://github.com/USNavalResearchLaboratory/PyEBSDIndex/blob/main/pyebsdindex/ebsd_pattern.py),
  `EBSDPatternFile` and `UPFile` reader implementations.

## License & Copyright

Please see the description file distributed with this **Plugin**.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over
to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions)
GitHub site where the community of DREAM3D-NX users can help answer your questions.
