# Exp Negative Image Filter

## Group (Subgroup)

ImageProcessing (Pointwise)

## Description

Computes the negative exponential, `exp(-x) = e^(-x)`, for each input value. The input and output are dimensionless. Positive inputs give positive decay values below 1. Negative inputs produce growth. Each component is processed separately. The output keeps the input tuple and component shapes.

### Output type and precision

| Input type | Output type | Output bytes per value |
|---|---|---|
| int8, uint8, int16, uint16, int32, uint32, int64, uint64 | float64 | 8 |
| float32 | float32 | 4 |
| float64 | float64 | 8 |

Integer input uses float64 to preserve quantitative decay and a wider growth range. Input 1 gives about 0.3678794412. Input 255 gives about 1.7984862203 × 10^-111, which is positive in float64 but below the float32 range. Input -128 gives about 3.8877084060 × 10^55, which also requires the float64 range. Output promotion depends on the input type, not its value.

Float32 has about 7 significant decimal digits; float64 has about 16. Results are rounded to the output type. Float64 represents every int8 and uint8 negative-exponential result as a finite positive value, but the larger integer types can still produce underflow or overflow. Converting a 64-bit integer argument to float64 can round that argument before the calculation. Floating input keeps its existing type: float32 input -128 produces positive infinity, while float64 or integer input -128 has the finite result above.

### Numeric limits

- NaN (not a number) produces NaN.
- Positive infinity produces positive zero. Negative infinity produces positive infinity.
- Positive and negative zero both produce 1.
- Growth beyond the finite output range produces positive infinity. Finite float64 results above the float32 range are handled explicitly before conversion.
- Very small decay results can be **subnormal** values, which are positive values below the smallest normal floating value. Further underflow produces positive zero. Float32 input 128 produces zero; float64 or integer input 128 gives about 2.5722093726 × 10^-56.

The argument is converted to float64 before its sign changes. A signed integer minimum is therefore not negated in its integer type.

### Storage and downstream use

For integer inputs, the 8-byte output uses 8, 4, 2, or 1 times the bytes of an 8-, 16-, 32-, or 64-bit input value. One billion uint8 input values require 1 GB, and their output values require 8 GB, before other storage. Floating input keeps its existing output width.

Earlier integer output truncated decay values to zero: integer input 1 or 255 gave zero. These results now remain positive until floating underflow occurs. Every uint8 input from 0 through 255 has a positive float64 output. A later output-not-equal-to-zero mask can therefore select every cell. Review masks and thresholds that depended on the earlier integer truncation.

Downstream filters that accept only integers can reject the float64 output. Adjust the pipeline to accept floating data and reconsider its thresholds. If an integer conversion is necessary, choose an explicit range and rounding policy. An unchecked conversion can lose decay values or exceed the integer range.

## Algorithm

The filter applies the same negative-exponential operation to resident and disk-backed arrays. It uses the existing pointwise traversal and bounded transfer buffers for disk-backed storage. It does not create a full-array resident temporary for the calculation. Output storage still requires the bytes shown above.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
