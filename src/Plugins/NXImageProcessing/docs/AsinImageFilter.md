# Asin Image Filter

Computes the arcsine of each pixel.

## Group (Subgroup)

NXImageProcessing (Pointwise)

## Description

Computes the principal arcsine of each input value. A dimensionless sine in the closed interval [−1, 1] gives an angle from −π/2 to π/2 radians. Finite inputs outside this interval, infinity and NaN all produce NaN (not a number).

| Input sine | Output in radians |
|---|---|
| −1 | −π/2 ≈ −1.57080 |
| 0 | 0 |
| 1 | π/2 ≈ 1.57080 |
| 2 | NaN |

### Output type

| Input type | Output type |
|---|---|
| Signed or unsigned integer | Float32 |
| Float32 | Float32 |
| Float64 | Float64 |

Integer input gives floating-point output so that fractional angles are kept. For example, an input of 1 gives about 1.57 radians. If a downstream filter expects a sine or cosine, check its input domain before passing it this angle.

## Algorithm

The filter evaluates each component in double precision and stores the angle in the output type. It preserves the tuple and component shapes of the input.

% Auto generated parameter table will be inserted here

## Example Pipelines

`16_Phase_and_Angle_Field_Transforms.d3dpipeline` uses this filter to convert *Normalized Response* to *Inverse Sine*.

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
