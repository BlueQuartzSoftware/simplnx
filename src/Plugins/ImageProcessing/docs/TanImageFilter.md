# Tan Image Filter

## Group (Subgroup)

ImageProcessing (Pointwise)

## Description

Computes the **tangent**, `tan(x)`, for each input value. The input is an angle in **radians**. The output is a signed, dimensionless response. Each component is processed separately. The output keeps the input tuple and component shapes.

### Output Type and Precision

| Input type | Output type | Output bytes per value |
|---|---|---|
| int8, uint8, int16, uint16, int32, uint32, int64, uint64 | float32 | 4 |
| float32 | float32 | 4 |
| float64 | float64 | 8 |

Integer input needs floating output because an integer angle can produce a signed, fractional tangent. For example, `tan(1)` is about 1.55740772 and `tan(2)` is about -2.18503986. An unsigned input does not imply a positive tangent. Integer output would discard fractions and cannot safely store all negative responses.

The input is converted to double precision before the tangent calculation. The result is then rounded to the output type. Float32 has about 7 significant decimal digits; float64 has about 16. The selected float32 output for integer input uses half the bytes of float64. A representation study across all int8 and uint8 arguments found a worst relative rounding error of about `5.63e-8`. This measures output rounding only. It is not a bound on the tangent calculation or wider integer inputs.

Integer arguments above `2^53` in magnitude can lose whole integer units when converted to double. For example, `9007199254740993` becomes `9007199254740992` before evaluation. This can change the tangent's phase and result. Float64 output would not recover that lost input precision.

### Numeric Limits

- Positive and negative zero keep their signs.
- NaN (not a number), positive infinity, and negative infinity produce NaN.
- Finite angles near a **pole**, where the mathematical tangent is undefined, are evaluated without a cutoff or clamp. Poles occur at `pi/2 + k*pi` radians for integer `k`.
- The nearest float32 and float64 values to `pi/2` lie on opposite sides of that pole. Their results are about `-2.2877332e7` and `1.6331239353e16`, respectively. Both are finite. Small changes in a stored angle near a pole can change the result's sign and magnitude.
- If a finite double result exceeds the float32 finite range, it is stored as infinity with the result's sign. NaN, infinity, and signed zero keep their floating behavior during output conversion.

### Storage and Downstream Use

For integer input, the 4-byte output uses 4, 2, 1, or 0.5 times the bytes of an 8-, 16-, 32-, or 64-bit input value. Floating input keeps its existing output width.

Existing pipelines with integer input now receive float32 responses. Earlier integer output discarded fractions: for example, `tan(3)` became zero in signed integer output instead of about -0.14254654. Review thresholds and zero/nonzero masks that depended on this truncation. Filters that accept only integer arrays can reject the new output type. Near-pole responses can dominate extrema and normalization. Before converting output back to integers, handle nonfinite values and check the destination range; an unchecked conversion can reintroduce unsafe behavior.

## Algorithm

The filter evaluates each value independently through the existing pointwise traversal. Resident arrays use direct memory views. Disk-backed arrays use serial bulk reads and writes through working batches. Transfer buffers target a combined 64 MiB, but each batch holds at least one complete trailing-dimensional slab, such as an XY slice for a Z/Y/X tuple shape. A slab can exceed this target or contain the whole array. Parallel calculation uses owned buffers. The filter adds no separate Tan-specific full-array calculation buffer. Output uses the input storage format and preserves its geometry.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
