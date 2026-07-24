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

NIfTI-1 allows a variable-length extension block between the 348-byte
header and the voxel data. The filter honors `vox_offset` (the byte offset
in the file where the voxel data begins) and seeks past the extension
block before reading voxels — any custom extension metadata (DICOM
attributes, AFNI XML, etc.) is **skipped, not preserved**.

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

NIfTI stores up to two affine transforms that map voxel indices to physical
space: the `sform` ("standard" transform, a general affine matrix) and the
`qform` ("quaternion" transform, a rigid rotation plus translation). When
*Use Stored Affine Transform* is enabled (the default), the filter uses
the NIfTI `sform` transform (if `sform_code > 0`) or `qform` transform (if
`qform_code > 0`) to set the **Image Geometry** origin and spacing. If neither
is set, the filter falls back to `pixdim[1..3]` (the per-axis voxel size
stored in the header) for spacing and a zero origin.

The spacing units are whatever the source file recorded (most commonly
millimeters for medical NIfTI data) and are **not** converted by this
filter. Treat the resulting **Image Geometry** spacing as being in the
file's native physical units.

simplnx Image Geometries are axis-aligned; if the stored transform contains a
non-trivial rotation, only the spacing (column magnitudes) and origin are
extracted and a warning is emitted. The voxel data itself is loaded in its
native storage order.

### Data scaling

NIfTI can store a linear intensity rescaling defined by two header fields:
`scl_slope` (the multiplicative slope) and `scl_inter` (the additive
intercept). When *Apply Scaling Transform* is enabled and the header
specifies a non-trivial scaling (`scl_slope != 0` and (`scl_slope != 1` or
`scl_inter != 0`)), the filter computes `y = scl_slope * x + scl_inter` at
read time and promotes the output array to `float32`. Per the NIfTI-1
specification, scaling is never applied to `RGB24` or `RGBA32` data and a
warning is emitted if a scaling transform is present for those types.

### Cropping on read

When the *Cropping Options* parameter is set to anything other than
*NoCropping*, only the selected sub-volume is **retained** in the
output DataArray. The rest of the file is still read (and, for
`.nii.gz`, still decompressed) but is never stored, so peak memory is
proportional to the cropped region, not the full source volume. See
the **Memory vs. wall-clock** note below before assuming cropping
speeds up I/O.

Two cropping modes are supported:

* **Voxel Subvolume** — pick an inclusive `[start..end]` voxel index range
  per axis. The range is zero-based and uses the source file's voxel
  grid.
* **Physical Subvolume** — pick an inclusive physical coordinate range per
  axis. The filter builds a temporary axis-aligned ImageGeom from the
  source file's origin and spacing (the same values the Orientation
  section would use) and converts the requested bounds to voxel indices
  via `ImageGeom::getIndex()`. Bounds that fall outside the volume
  produce a descriptive error rather than silently clamping.

Each axis can be toggled on or off independently via the *Crop X / Y / Z*
flags. An axis whose flag is off is *not* cropped — its full extent from
the source file is used, even if the physical or voxel bound values are
populated.

#### How the cropped ImageGeom relates to the source

* **Dimensions** equal the cropped voxel count on each axis.
* **Spacing** is the same as the source file's spacing (cropping never
  resamples).
* **Origin** is shifted so that the center of the first cropped voxel
  lands at its correct physical position, i.e. `new_origin[i] =
  source_origin[i] + start_voxel[i] * spacing[i]`. This is produced by
  running `CropImageGeometryFilter::preflight` on a temp geometry, so the
  behavior matches what you'd get from piping a standalone read +
  `CropImageGeometry` sequence.

#### Interaction with other options

* **Scaling** — if *Apply Scaling Transform* is on, `y = slope*x + inter`
  is applied only to the retained voxels. Voxels outside the crop region
  are discarded before scaling.
* **RGB24 / RGBA32** — all per-voxel components are preserved. The crop
  operates on the voxel grid, not on individual color bytes.
* **Orientation** — cropping runs on the axis-aligned ImageGeom that was
  already constructed from the sform / qform / pixdim fallback chain; a
  stored rotation warning (if any) is emitted independently of cropping.

#### Memory vs. wall-clock

Cropping on read primarily saves **memory**, not I/O time, in this
version:

* For `.nii.gz` the file is not seekable without linear decompression, so
  the full compressed stream is still read and decompressed even when
  the cropped region is small. Peak resident memory, however, is
  proportional to the cropped region, not the full uncompressed volume.
  This matters when the source file scaled up to `float32` would not fit
  in RAM.
* For plain `.nii` this version also reads linearly (scan-line by
  scan-line, discarding out-of-range rows in place of a true
  seek-skip). A seek-skip optimization for uncompressed files may come
  later if real-world timings warrant it.

In short: use cropping to keep the output DataArray small, not to make
large compressed files read faster.

## Caveats

* Only the single-file NIfTI-1 format (magic `n+1`) is supported. The
  separate-file `.hdr` / `.img` pair (magic `ni1`) is not.
* Only 3D volumes (`dim[0] == 3`) are supported. 4D and higher files (fMRI
  time series, diffusion series, etc.) are rejected in this release.
* Any of `dim[1]`, `dim[2]`, `dim[3]` being `<= 0` is rejected.
* Complex (`complex64`, `complex128`, `complex256`) and 128-bit float
  voxel datatypes are rejected.
* Extension blocks between the header and voxel data are skipped over
  (via `vox_offset`) but not preserved.
* Non-trivial rotations in `sform` / `qform` are flattened — the Image
  Geometry is axis-aligned in simplnx.
* Cropping saves memory, not wall-clock read time. A `.nii.gz` file must
  still be fully decompressed regardless of how small the cropped region
  is; plain `.nii` files are also read linearly in this version.
* Physical-subvolume crop bounds must map to voxels inside the source
  volume. Bounds outside the extent produce an error rather than
  silently clamping.
* `start <= end` is required on each cropped axis; reversed ranges are
  rejected at parameter-validation time.

## Required Input Sources

None — this filter reads directly from a `.nii` or `.nii.gz` file on disk.

% Auto generated parameter table will be inserted here

## Reference

* [NIfTI-1 Data Format (official NIMH reference)](https://nifti.nimh.nih.gov/nifti-1/)
* [`nifti1.h` header (NITRC)](https://www.nitrc.org/docman/view.php/26/64/nifti1.h)

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
