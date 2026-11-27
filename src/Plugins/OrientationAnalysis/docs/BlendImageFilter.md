# Blend Image

## Group (Subgroup)

Processing (Image)

## Description

This **Filter** modulates the brightness of an RGB color image using a per-**Cell** scalar *modifier* value, producing a new color image. After the modifier is normalized into the range [0, 1] across the image, each color channel of a **Cell** is multiplied by that **Cell**'s normalized value. **Cells** with the lowest modifier values darken toward black, while **Cells** with the highest values keep their full color.

A common use is to weight an IPF (crystallographic orientation) color map by a per-**Cell** quality metric such as Confidence Index or Image Quality, so low-quality points appear dim and high-quality points appear bright. The **Filter** is general, however: it accepts **any** 3-component `uint8` color **Data Array** and **any** 1-component `float32` modifier **Data Array**.

### Inputs and Output

- **Color Image Array** — the RGB colors to blend (3-component `uint8`).
- **Modifier Array** — the per-**Cell** brightness weight (1-component `float32`).
- **Blended Colors Array** (output) — the resulting blended RGB image (3-component `uint8`), created alongside the input color array.

The behavior is controlled by the *Blend Mode* selector. Each mode builds on the one before it, so the parameters available (and required) depend on the selected mode.

### Blend Mode: Basic

The generic blend, suitable for any color array:

1. The **Modifier Array** is scanned for its minimum and maximum values.
2. Each **Cell**'s modifier is normalized to [0, 1] using that range (minimum → 0, maximum → 1).
3. Each RGB channel is multiplied by the normalized value and stored as a byte.

If every modifier value is identical, all **Cells** are treated as fully weighted and the colors pass through unchanged. Basic mode requires only the **Color Image Array** and **Modifier Array**; no other inputs are used.

### Blend Mode: Advanced

Advanced mode performs the same brightness blend but with 8-bit–accurate intensity scaling: the normalized weight is quantized to 256 levels and the blended color is rounded to the nearest value (Basic mode truncates instead). Per-channel results therefore differ from Basic by at most one gray level. As in Basic mode, the normalization range is computed globally over the whole image.

Advanced mode adds two optional transforms:

- **Blackout Bad Points** (*on* by default) — any **Cell** whose value in the **Blackout Criterion Array** is less than the *Blackout Threshold* is forced to pure black (0, 0, 0), regardless of its color or brightness. This is used to hide unindexed or invalid measurement points. The criterion array is a separate 1-component `float32` input; it may be the same array used as the **Modifier Array** (for example a Confidence Index) or a different one. When this option is enabled, the **Blackout Criterion Array** is required and must have the same number of **Cells** as the color image.
- **Apply Final Rescale** (*off* by default) — after blending, the entire image is contrast-stretched so that its darkest channel value maps to 0 and its brightest maps to 255, maximizing use of the available brightness range. When off, the blended values are written as-is.

### Blend Mode: Advanced with Background Leveling

This mode does everything Advanced mode does and additionally removes smooth, large-scale brightness gradients from the modifier before blending. It is useful when the modifier (for example a quality map) has an uneven background — such as one side of a scan being systematically brighter than the other.

- A 2D polynomial surface of the chosen *Surface Fit Degree* is fit to the modifier values of each slice and subtracted from them, flattening the background while preserving local detail. A higher degree removes more complex gradients; the degree must be at least 1.
- Because leveling operates slice by slice, this mode also computes the normalization range and the optional final rescale **per slice**: each 2D slice of the volume is treated as an independent image. For a single-slice (2D) dataset, this produces the same result as Advanced mode's global normalization.
- This mode requires an **Input Image Geometry** so the **Filter** knows the X/Y/Z dimensions of each slice. The geometry's **Cell** count must equal the number of **Cells** in the color and modifier arrays.

% Auto generated parameter table will be inserted here

## Example Pipelines

None.

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
