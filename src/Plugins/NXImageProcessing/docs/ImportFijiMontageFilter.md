# Read Fiji Montage

## Group (Subgroup)

IO (Read)

## Description

This **Filter** reads a [Fiji](https://imagej.net/software/fiji/) montage
`TileConfiguration[.registered].txt` file and imports each tile it references as
its **own** 2D **Image Geometry**. It is an ITK-free replacement for the legacy
*ITK Import Fiji Montage* filter and produces byte-for-byte the same result,
minus ITK.

Each non-comment line of a `TileConfiguration` file lists one tile in the form:

```
<file>; ; (x, y)
```

For every parsed tile the Filter:

1. Resolves `<file>` relative to the configuration file's directory.
2. Reads the tile's dimensions and pixel type through the same image-decode path
   used by the *Read Image* filter (libtiff for TIFF, stb for PNG/BMP/JPEG).
3. Creates a 2D **Image Geometry** (`Z = 1`) named `<prefix><file-stem>`,
   positioned at the parsed `(x, y)` origin, holding a **Cell Attribute Matrix**
   and an image **DataArray** filled with the tile's pixels.

When **Convert To GrayScale** is on, each tile image must have 1, 3, or 4
components. When this option is off, any component count is accepted.

Preflight fails if the filter cannot identify a tile image format or read its
metadata. If the filter cannot read the pixels of a tile, it reports a warning and continues.
When grayscale conversion applies to that tile, its array has one component and contains zeros.
The filter fails if no tile can be read.

There is **no stitching or blending** — the tiles are imported side by side as
independent geometries positioned at their montage coordinates.

### Optional Processing

- **Change Origin**: rebases the montage so its minimum corner lands on a
  user-supplied origin. `newOrigin = origin - (minCorner - userOrigin)`.
- **Parent Imported Images Under a DataGroup**: when enabled, all tile
  geometries are created under a single parent **DataGroup**; otherwise they are
  created at the root of the data structure.
- **Convert To GrayScale**: converts `uint8` RGB and RGBA tiles to single-channel
  grayscale using the supplied luminosity **Color Weighting**. RGBA conversion
  ignores alpha. The `uint8` check uses the type after **Set Image Data Type**.
  A `uint16` RGB tile cast to `uint8` is converted.
  Single-component tiles skip grayscale conversion.
  RGB and RGBA tiles with other data types are imported without grayscale
  conversion, and the filter reports a warning.
- **Set Image Data Type**: casts every imported tile array to `uint8`,
  `uint16`, or `uint32`.

### Tile Coordinates

Each tile coordinate must be finite. If the file contains a nonfinite coordinate, the error identifies the TileConfiguration file and line. Rebasing the montage origin still requires valid tile coordinates.

## Required Geometry

Not Applicable (this Filter creates one geometry per tile).

## Required Inputs

None (all inputs are read from the configuration file and the tile images it
references).

## Created Outputs

- One **Image Geometry** per tile (optionally under a parent **DataGroup**),
  each with dimensions `[width, height, 1]` positioned at the tile's origin, a
  **Cell Attribute Matrix**, and an image **DataArray** holding the tile's
  pixels (native type, or the chosen output type; multi-component tiles use the
  component count as the last dimension).

## References

- Fiji / ImageJ: <https://imagej.net/software/fiji/>

% Auto generated parameter table will be inserted here

## Example Pipelines

+ 20_Fiji_Microscopy_Montage_Import.d3dpipeline
