# Read NIfTI File (Version 1)

## Group (Subgroup)

IO (Read)

## Description

This **Filter** reads a [NIfTI-1](https://nifti.nimh.nih.gov/nifti-1/) volume
(single-file format, `.nii` or `.nii.gz`) into an **Image Geometry** plus a
**Cell Data** attribute matrix that holds the voxel values.

The filter transparently handles both uncompressed (`.nii`) and gzipped
(`.nii.gz`) files via zlib, and detects / corrects for file byte order by
comparing `sizeof_hdr` against the expected value of 348.

### Supported voxel datatypes

| NIfTI code | C type | simplnx `DataType` | Component count |
|---:|---|---|---:|
| `NIFTI_TYPE_UINT8` (2) | `uint8` | `uint8` | 1 |
| `NIFTI_TYPE_INT8` (256) | `int8` | `int8` | 1 |
| `NIFTI_TYPE_UINT16` (512) | `uint16` | `uint16` | 1 |
| `NIFTI_TYPE_INT16` (4) | `int16` | `int16` | 1 |
| `NIFTI_TYPE_UINT32` (768) | `uint32` | `uint32` | 1 |
| `NIFTI_TYPE_INT32` (8) | `int32` | `int32` | 1 |
| `NIFTI_TYPE_UINT64` (1280) | `uint64` | `uint64` | 1 |
| `NIFTI_TYPE_INT64` (1024) | `int64` | `int64` | 1 |
| `NIFTI_TYPE_FLOAT32` (16) | `float32` | `float32` | 1 |
| `NIFTI_TYPE_FLOAT64` (64) | `float64` | `float64` | 1 |
| `NIFTI_TYPE_RGB24` (128) | 3 × `uint8` | `uint8` | 3 |
| `NIFTI_TYPE_RGBA32` (2304) | 4 × `uint8` | `uint8` | 4 |

Complex and 128-bit float types are not currently supported.

### Orientation

When *Use Stored Affine Transform* is enabled (the default), the filter uses
the NIfTI `sform` transform (if `sform_code > 0`) or `qform` transform (if
`qform_code > 0`) to set the **Image Geometry** origin and spacing. If neither
is set, the filter falls back to `pixdim[1..3]` for spacing and a zero origin.

simplnx Image Geometries are axis-aligned; if the stored transform contains a
non-trivial rotation, only the spacing (column magnitudes) and origin are
extracted and a warning is emitted. The voxel data itself is loaded in its
native storage order.

### Data scaling

When *Apply Scaling Transform* is enabled and the header specifies a
non-trivial scaling (`scl_slope != 0` and (`scl_slope != 1` or
`scl_inter != 0`)), the filter computes `y = scl_slope * x + scl_inter` at
read time and promotes the output array to `float32`. Per the NIfTI-1
specification, scaling is never applied to `RGB24` or `RGBA32` data and a
warning is emitted if a scaling transform is present for those types.

## Parameters

| Parameter | Type | Default | Description |
|---|---|---|---|
| Input NIfTI File | File path | — | Path to the `.nii` or `.nii.gz` file to read. |
| Use Stored Affine Transform | Bool | `true` | Use `sform`/`qform` to set origin + spacing when present. |
| Apply Scaling Transform | Bool | `true` | Apply `y = slope*x + inter` at read time; promotes to float32. |
| Image Geometry | Data Path | `NIfTI Image` | Path to the created Image Geometry. |
| Cell Attribute Matrix Name | String | `Cell Data` | Name of the attribute matrix holding voxel values. |
| Image Data Array Name | String | `ImageData` | Name of the array receiving voxel values. |

## Caveats

* Only the single-file NIfTI-1 format (magic `n+1`) is supported. The
  separate-file `.hdr` / `.img` pair (magic `ni1`) is not.
* Only 3D volumes (`dim[0] == 3`) are supported. 4D and higher files (fMRI
  time series, diffusion series, etc.) are rejected in this release.
* Non-trivial rotations in `sform` / `qform` are flattened — the Image
  Geometry is axis-aligned in simplnx.

% Auto generated parameter table will be inserted here                                                                    

## Reference

* [NIfTI-1 Data Format (official NIMH reference)](https://nifti.nimh.nih.gov/nifti-1/)
* [`nifti1.h` header (NITRC)](https://www.nitrc.org/docman/view.php/26/64/nifti1.h)

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
