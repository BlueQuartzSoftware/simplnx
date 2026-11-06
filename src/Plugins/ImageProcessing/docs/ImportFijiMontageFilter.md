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

Each tile image must have 1, 3, or 4 components. Preflight rejects other
component counts, even when grayscale conversion is off.

Preflight fails if the filter cannot identify a tile image format or read its
metadata. After metadata is read, if pixel data cannot be read, the filter
reports a warning and continues with the other tiles. When grayscale conversion
is enabled for a uint8 RGB or RGBA tile, a tile with a pixel-read failure has a
single-component array filled with zeros. The filter fails if no tile can be read.
There is **no stitching or blending** — the tiles are imported side by side as
independent geometries positioned at their montage coordinates.

### Optional processing

- **Change Origin**: rebases the montage so its minimum corner lands on a
  user-supplied origin. `newOrigin = origin - (minCorner - userOrigin)`.
- **Parent Imported Images Under a DataGroup**: when enabled, all tile
  geometries are created under a single parent **DataGroup**; otherwise they are
  created at the root of the data structure.
- **Convert To GrayScale**: converts `uint8` RGB and RGBA tiles to single-channel
  grayscale using the supplied luminosity **Color Weighting**. RGBA conversion
  ignores alpha. Single-component tiles skip grayscale conversion.
  RGB and RGBA tiles with other data types are imported without grayscale
  conversion, and the filter reports a warning.
- **Set Image Data Type**: casts every imported tile array to `uint8`,
  `uint16`, or `uint32`.

## Parameters

| Name | Type | Description |
|------|------|-------------|
| Fiji Configuration File | Path | The `TileConfiguration[.registered].txt` file, alongside its tile images |
| Length Unit | Enum | Length unit set into every created Image Geometry |
| Change Origin | bool | Rebase the montage's minimum corner to a user origin |
| Origin | float (3) | New origin of the montage's minimum corner |
| Convert To GrayScale | bool | Convert eligible RGB and RGBA tiles to single-channel grayscale |
| Color Weighting | float (3) | Luminosity R/G/B weights for grayscale conversion |
| Set Image Data Type | bool | Cast every imported tile array to a chosen type |
| Output Data Type | Enum | 0 = uint8, 1 = uint16, 2 = uint32 |
| Parent Imported Images Under a DataGroup | bool | Group all tile geometries under one DataGroup |
| Name of Created DataGroup | String | Name of the parent DataGroup |
| Image Geometry Prefix | String | Prefix prepended to each tile's file-stem to name its Image Geometry |
| Cell Attribute Matrix Name | String | Name of the created cell Attribute Matrix in each tile geometry |
| Image DataArray Name | String | Name of the created image DataArray in each tile geometry |

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
