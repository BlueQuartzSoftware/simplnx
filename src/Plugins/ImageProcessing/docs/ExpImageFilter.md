# Exp Image Filter

## Group (Subgroup)

ImageProcessing (Pointwise)

## Description

Computes the natural exponential, `exp(x) = e^x`, for each input value. The input and output are dimensionless. Positive inputs describe growth. Negative inputs give positive decay values below 1. Each component is processed separately. The output keeps the input tuple and component shapes.

### Output type and precision

| Input type | Output type | Output bytes per value |
|---|---|---|
| int8, uint8, int16, uint16, int32, uint32, int64, uint64 | float64 | 8 |
| float32 | float32 | 4 |
| float64 | float64 | 8 |

Integer input uses float64 because an exponential can have fractional decay or large growth. For example, integer input -1 gives about 0.3678794412. Integer input 100 gives the finite float64 value about 2.6881171418 × 10^43. Float32 input 100 keeps float32 output and gives positive infinity. Output promotion depends on the input type, not its value.

Float32 has about 7 significant decimal digits; float64 has about 16. Floating results are rounded to the output type. Float64 covers the full int8 and uint8 exponential range, but no finite output type covers all possible input values. Integer input 255 gives about 5.5602316477 × 10^110, which exceeds the float32 range.

### Numeric limits

- NaN (not a number) produces NaN.
- Positive infinity produces positive infinity. Negative infinity produces positive zero.
- Positive and negative zero both produce 1.
- Growth beyond the finite output range produces positive infinity. Finite double results above the float32 range are handled explicitly before conversion.
- Very small decay results can be **subnormal** values, which are positive values below the smallest normal floating value. Further underflow produces zero. Float32 input -128 produces zero; float64 or integer input -128 gives about 2.5722093726 × 10^-56.

### Storage and downstream use

For integer inputs, the 8-byte output uses 8, 4, 2, or 1 times the bytes of an 8-, 16-, 32-, or 64-bit input value. One billion uint8 input values require 1 GB, and their output values require 8 GB, before other storage. Floating input keeps its existing output width.

Earlier integer output truncated positive decay values below 1 to zero. These values now remain positive until floating underflow occurs. Review thresholds and zero/nonzero masks that depended on that integer truncation. Downstream filters that accept only integers can reject the float64 output. An unchecked conversion back to integer can again lose decay values or exceed the integer range.

## Algorithm

The filter applies the same exponential operation to resident and disk-backed arrays. Buffered processing targets 64 MiB of transfer scratch by default, but each batch must contain at least one complete storage slab formed by the trailing tuple dimensions, such as an XY slice for Z/Y/X storage. A slab can exceed that target or cover the entire array. Bulk input and output transfers are serial. The calculation runs in parallel on owned buffers. Exp adds no separate full-volume scratch array beyond those transfer buffers. Output storage still requires the bytes shown above.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
