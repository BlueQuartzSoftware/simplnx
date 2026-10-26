# Asin Image Filter

## Group (Subgroup)

ImageProcessing (Pointwise)

## Description

Computes the **principal arcsine**, `asin(x)`, for each value in the input image. The input is a dimensionless sine value. The output is an angle in **radians**, from -pi/2 to pi/2 for valid input.

Values in the closed interval `[-1, 1]` have a real angle. Values outside this interval, including positive and negative infinity, produce NaN (not a number). Incoming NaN also produces NaN. The filter does not clamp, replace or reject these values.

### Output Type and Precision

| Input type | Output type | Output bytes per value |
|---|---|---|
| int8, uint8, int16, uint16, int32, uint32, int64, uint64 | float32 | 4 |
| float32 | float32 | 4 |
| float64 | float64 | 8 |

Integer inputs need floating output because their valid values `-1`, `0` and `1` produce -pi/2, 0 and pi/2 radians. Integer output would lose the fractional part of these angles and could not store NaN. With unsigned integer input, only `0` and `1` are in the valid domain.

The calculation uses double precision, then stores the result in the output type. For valid integer inputs, float32 introduces at most about `4.37e-8` radians of representation error. The filter preserves the input tuple shape and component shape. It processes each component separately.

For integer input, output storage is four times the size of an 8-bit input, twice the size of a 16-bit input, equal to a 32-bit input, and half the size of a 64-bit input. Floating input keeps its existing storage width.

### Migration and Downstream Use

Existing pipelines with integer input now receive float32 angles. Previously, integer output discarded fractional angles: for example, `asin(1)` became 1 rather than pi/2 radians. Check downstream filters that require integer input and thresholds that relied on that truncation. For valid integer inputs, direct zero/nonzero status is unchanged.

For example, an Asin result passed to [Acos Image Filter](AcosImageFilter.md) can change from the old quantized `1` to pi/2. Acos then produces NaN because pi/2 is outside its input domain `[-1, 1]`. This is a constructed composition example; it does not represent a reproduced user pipeline. Check the units and domain before using a radian angle as a dimensionless trigonometric input.

Before converting the result to an integer type later in a pipeline, handle NaN and check the destination range. An unchecked conversion can reintroduce unsafe behavior.

## Algorithm

The filter evaluates each value independently. Resident arrays use direct memory views. Out-of-core arrays use serial bulk reads and writes through working batches; parallel calculation uses owned buffers. Batch sizes follow the pointwise engine's memory target and store layout. A small image can fit in one batch. Output uses the input storage format and preserves its geometry.

% Auto generated parameter table will be inserted here

## Example Pipelines

The following scalar values illustrate the result. These are examples, not named pipeline files.

| Input sine | Output angle (radians) |
|---|---|
| -1 | -pi/2, approximately -1.57079633 |
| 0 | 0 |
| 1 | pi/2, approximately 1.57079633 |
| 2 | NaN |

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
