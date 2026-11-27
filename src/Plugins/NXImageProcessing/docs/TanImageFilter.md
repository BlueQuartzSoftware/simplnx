# Tan Image Filter

Computes the tangent of each pixel.

## Group (Subgroup)

NXImageProcessing (Pointwise)

## Description

Computes the tangent of each input angle. Supply angles in radians. The output is a dimensionless ratio that can be negative, even for unsigned input. Near a pole at π/2 plus any integer multiple of π, a small change in the stored angle can change the sign and magnitude of its tangent.

| Input in radians | Output tangent |
|---|---|
| −1 | ≈ −1.55741 |
| 0 | 0 |
| 1 | ≈ 1.55741 |
| 2 | ≈ −2.18504 |
| NaN or either infinity | NaN (not a number) |

### Output type

| Input type | Output type |
|---|---|
| Signed or unsigned integer | Float32 |
| Float32 | Float32 |
| Float64 | Float64 |

Integer input gives floating-point output so that signed, fractional results are kept. A zero/nonzero mask treats a small result as nonzero. For example, the tangent of 3 radians is about −0.14255.

## Algorithm

The filter evaluates each component in double precision and rounds the result to the output type. Integer angles beyond 2⁵³ in magnitude can round during input conversion, changing the angle used for the calculation. The output retains the input tuple and component shapes.

% Auto generated parameter table will be inserted here

## Example Pipelines

`16_Phase_and_Angle_Field_Transforms.d3dpipeline` uses this filter to convert *Phase Radians* to *Tangent*.

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
