# Smoothing Recursive Gaussian Image Filter

Smooth (blur) an image using a separable recursive-Gaussian (Deriche) convolution with a per-axis standard deviation.

## Group (Subgroup)

NXImageProcessing (Smoothing)

## Description

Smooths an image with a recursive approximation of a Gaussian filter. Smoothing reduces fine detail and noise.

Select a single-component array of type int8, int16, int32, int64, float32, or float64. The output has the input type and
array shape. Each filtered axis needs at least four pixels. For a single Z slice, the filter uses only X and Y.

### Sigma

*Sigma* is the Gaussian standard deviation in image-spacing units. It defaults to 1 along each axis. A larger value
produces more smoothing along that axis.

Preflight checks that X and Y Sigma are finite and positive. If the final image has more than one Z slice, execution also
requires a finite, positive Z Sigma. Execution checks the spacing on each filtered axis. The spacing magnitude must be finite and greater than
`1e-8` in image-spacing units. The filter checks that Sigma divided by the spacing magnitude gives a finite positive
scale and that the calculation can form finite filter coefficients. Accuracy can decrease at extreme scales.

### Normalize Across Scale

This choice defaults to Off. It affects Gaussian derivatives. Smoothing uses order zero and has the same result with
either choice.

## Algorithm

The filter smooths one axis at a time. It calculates in double precision and stores intermediate values as
**float32** between axes. An output of type float64 cannot recover detail lost in these intermediate values.
Large int64 values can also lose detail.

The in-memory path filters lines in parallel and holds a float32 working image. The three-dimensional out-of-core path
uses a disk-backed working image and processes bounded groups of rows and planes. The two-dimensional out-of-core path
works in bounded blocks and may use disk-backed intermediate data. Both out-of-core paths read and write stores
serially.

### Final values and diagnostics

For integer output, the filter truncates each finite final value toward zero. It clips the result to the output type's
range if needed. For example, an int8 result of 127.75 becomes 127 without clipping, while 128 clips to 127. The filter
reports the total number of clipped values in one warning.

If the calculation produces NaN or infinity, integer output fails. One error gives the number of nonfinite results,
the output type, and the flat index if exactly one value was found. If conversion stops early, the diagnostics identify
partial counts. A block containing a rejected result is skipped. Completed blocks can remain in the output.

Floating-point output retains NaN and infinity. A double value just above the largest finite float32 magnitude can
round back to that magnitude. At or beyond the overflow midpoint, it rounds to infinity with the same sign.

### Migration and downstream use

Review clipping warnings after updating a saved pipeline. Earlier versions could produce an invalid integer result
when the smoothed value exceeded the output type's range. Clipping can change later thresholding and segmentation.
Use floating-point input if you need fractional or nonfinite output. Intermediate values still have float32 precision.


% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
