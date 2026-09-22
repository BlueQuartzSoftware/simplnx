# Read MHA/MetaImage File

## Group (Subgroup)

IO (Read)

## Description

This **Filter** reads a [MetaImage](https://itk.org/Wiki/ITK/MetaIO/Documentation)
image (attached single-file `.mha`, or detached `.mhd` + external `.raw`/`.zraw`)
into an **Image Geometry** plus a **Cell Data** attribute matrix that holds the
voxel values. It is ITK-free (zlib only).

Supported header features:

- `CompressedData = True` (zlib) and uncompressed (`False`).
- `BinaryDataByteOrderMSB` / `ElementByteOrderMSB` (multi-byte types are
  byte-swapped to host order).
- Scalar images and interleaved component images (`ElementNumberOfChannels > 1`,
  e.g. displacement fields / RGB).
- `ElementSpacing` (or `ElementSize`) and `Offset` / `Position` / `Origin`.
- 2D images are loaded with `Z = 1`; 3D images load natively.

### Transformation Matrix

If **Apply Image Transformation To Geometry** is enabled, the header
`TransformMatrix` (2x2 for 2D, 3x3 for 3D) is assembled into a 4x4 matrix and
applied to the created geometry via the *Apply Transformation To Geometry*
filter, using the chosen **Interpolation Type**. **Transpose Stored
Transformation Matrix** transposes it first (requires a pure rotation:
`|1 - determinant| <= 1e-4`, else an error). **Save Image Transformation As
Array** writes the 16 floats (row-major 4x4) to a `float32[16]` array.

#### Known limitation

When **Apply Image Transformation To Geometry** is enabled, preflight surfaces
any errors/warnings from the *Apply Transformation To Geometry* step, but it does
**not** propagate that step's geometry-mutating output actions. As a result,
pipeline preflight reports the *pre-transform* Image Geometry dimensions, while
execute actually resamples the geometry to its post-transform extents. This is
deliberate parity with the legacy ITK reader. Downstream filters should not rely
on the transformed geometry's dimensions being known at preflight when Apply
Transformation is enabled.

### Unsupported (explicit errors)

- `ObjectType` other than `Image`, `NDims` other than 2/3, `BinaryData = False`
  (ASCII pixel data), unrecognized `ElementType`, and `ElementDataFile`
  `LIST` / `%d` / multi-file forms.

## Parameters

| Name | Type | Description |
|------|------|-------------|
| Input MHA/MetaImage File | Path | `.mha` or `.mhd` file to read |
| Cropping Options | Enum + bounds | Optional voxel/physical sub-volume streamed on read |
| Apply Image Transformation To Geometry | bool | Apply the header matrix to the geometry |
| Interpolation Type | Enum | 0 = Nearest Neighbor, 1 = Linear |
| Transpose Stored Transformation Matrix | bool | Transpose before use (pure rotation only) |
| Save Image Transformation As Array | bool | Save the 4x4 as a `float32[16]` array |
| Transformation Matrix | DataPath | Path of the saved matrix array |
| Image Geometry | DataPath | Created Image Geometry |
| Cell Attribute Matrix Name | String | Attribute matrix for the voxel data |
| Image Data Array Name | String | Array that holds the voxel values |

## Required Geometry

Not Applicable (this Filter creates a geometry).

## Created Outputs

- An **Image Geometry** with dimensions `[X, Y, Z]`, plus a **Cell Attribute
  Matrix** and a voxel **DataArray** of the file's native type (multi-component
  images use the component count as the last dimension). Optionally a
  `float32[16]` transformation-matrix array.

## References

- MetaIO / MetaImage documentation: <https://itk.org/Wiki/ITK/MetaIO/Documentation>
