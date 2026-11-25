# Otsu Multiple Thresholds Image Filter

Threshold an image using multiple Otsu Thresholds.

## Group (Subgroup)

NXImageProcessing (Thresholding)

## Description

This filter creates a labeled image that separates the input image into various classes. The filter computes the thresholds using a multi-Otsu calculator and applies those thresholds to the input image to produce class labels. The **Number Of Histogram Bins** and **Number Of Thresholds** control the calculator. The **Label Offset** is added to every output label so labels can start from a value other than zero.

Each finite, selected voxel receives the number of thresholds that its intensity strictly exceeds, plus **Label Offset**. A value equal to a threshold belongs to the lower class. The input can use any scalar signed or unsigned integer type, `float32`, or `float64`. The output is always a scalar `uint8` label image with the input tuple shape. Threshold values use the input intensity units; output labels have no intensity units.

The configured class interval is **Label Offset** through **Label Offset + Number Of Thresholds**, including both endpoints. The sum must not exceed 255. The filter rejects an invalid interval instead of wrapping labels. It does not renumber classes or widen the output type.

### Input with NaN or infinity

Only selected finite samples contribute to the histogram. The filter can complete with NaN or infinity in the input. Labels at these pixels have no scientific interpretation. Clean invalid intensities upstream or supply a mask that excludes them before using the labels for measurements.

### Explicit mask and excluded labels

Enable **Use Mask Array** to select a **Mask Array**. The mask must be scalar `bool` or `uint8`. Its final tuple count must match the input when Otsu runs, after any upstream filters have resized their arrays. Tuples are paired in flat order, so the mask can have a different tuple shape or parent group. False or zero excludes a pixel. True or any nonzero `uint8` value selects it. The filter applies this selection before fitting the histogram.

Pixels excluded by the mask receive **Excluded Label**, which defaults to 255. When a mask is enabled, this label must be outside the configured class interval, leaving up to 255 ordinary classes. An all-false mask gives every pixel the excluded label. With the mask disabled, all 256 labels are available for classes.

A distinct label does not make downstream filters ignore excluded pixels. Supply the mask or an explicit background/exclusion rule to consumers that must ignore them. Changing histogram participation can change phase fractions, feature counts, and connectivity.

For example, use the mask `[1,1,1,1,0,0,0]` with the input `[0,0,4,4,NaN,-Infinity,+Infinity]`. Two bins have counts `[2,2]`. With Label Offset 7 and Excluded Label 255, the output is `[7,7,8,8,255,255,255]`.

For component labeling, first select the ordinary class of interest into a binary mask. For fractions measured over valid
pixels, exclude Excluded Label from both the selection and the denominator. Removing nonfinite samples from the histogram
can move the fitted thresholds and change the labels of finite pixels too.

### Precision and pipeline migration

The filter uses `float64` for histogram bounds, bin measurements, threshold fitting, and comparisons between voxel intensities and thresholds. Adjacent large 64-bit integers can round to the same `float64` value and receive the same label without a warning or error. For example, the first two values in the `uint64` input `[2^53, 2^53 + 1, 2^53 + 4]` round to the same value. With two bins, one threshold, **ReturnBinMidpoint** enabled, and **Label Offset** 0, the output is `[0, 0, 1]`.

The filter reports a numeric error when it detects an unusable histogram range, unusable bins, nonfinite threshold arithmetic, or objective underflow. For example, distinct selected minimum and maximum values can round to the same value, or a very small nonzero class mean can square to zero. Rescale the finite data or use fewer bins where suitable. The checks do not establish exact arbitrary-precision Otsu accuracy. True finite constants retain the lower-class tie behavior.

The finite tie tolerance, ordinary threshold search, and Valley Emphasis behavior are retained. **ReturnBinMidpoint** selects a bin midpoint instead of its maximum for the returned threshold. These checks add no automatic normalization or new scientific fitting rule.

By default, **Use Mask Array** is off. Ordinary finite pipelines with valid label intervals and usable histogram and solver arithmetic retain their labels. A finite pipeline can fail if the fitting arithmetic overflows or loses a required distinction between values. Replacing bad intensities upstream adds those replacement values to the histogram. Masking them leaves them out of the fit.

This filter also includes an option to use the valley emphasis algorithm from H.F. Ng, "Automatic thresholding for defect detection", Pattern Recognition Letters, (27): 1644-1649, 2006. The valley emphasis algorithm is particularly effective when the object to be thresholded is small. It is turned off by default; enable it with **Valley Emphasis**.

ITK-free, out-of-core-capable reimplementation of the legacy ITK Otsu Multiple Thresholds Image Filter. The histogram and input range are computed with bounded-memory streaming passes, so memory use stays bounded regardless of image size.

## Algorithm

For integral inputs whose selected native intensity span is at most 65,535, the filter can determine the input range and exact value frequencies in one scan using a fixed 65,536-entry frequency ring. It then fills the requested histogram without rereading the input. Wider integral ranges use a second bounded histogram scan. Buffered scans also use a second scan when the working-memory grant cannot hold the ring after reserving the input and any mask buffers. Floating-point inputs use a selected finite range/count scan followed by the histogram scan. A true constant has an analytic tied-threshold path.

The histogram uses the existing marginal upper bound `max + (max - min) / (numberOfBins * 100)`. An upper-bound extension that rounds back to `max` is permitted when the bin geometry remains usable; endpoint clamping keeps minimum/maximum samples inside their bins before an integer index conversion. Native full-span 64-bit inputs remain valid for a two-bin split. No native maximum is incremented.

Thresholds are calculated from the histogram and applied independently to each selected voxel. Resident scans use direct spans. Other scans use serial bulk input/mask transfers with bounded owned buffers and working-memory reservations. Floating-point range scans reduce fixed blocks in parallel when multicore support is enabled and a batch contains more than one block. Workers read only resident spans or owned buffers. The main thread merges finite bounds and selected counts in block order. The summary table belongs to the current bounded batch and does not grow with the full image.

The default scratch target for the input, mask and floating-range summaries is 64 MiB. A developer working-memory override can change the target for buffered scans. The amount of useful data and the process-wide memory budget limit the actual grant. A low grant can reduce a buffered scan to one tuple. The masked output pass reserves combined input, mask, and label buffers. Boolean masks use contiguous owned storage. The optional ring and histogram/solver arrays depend on the fixed ring size and requested bin/class counts, not on image volume. The unmasked output pass retains the existing resident/buffered pointwise routes. No full-volume finite-only copy is made.

Cancellation is checked during scans and threshold search and before label writes. Transfer failures remain errors. Completed earlier label batches are not rolled back if a later output transfer fails.

![Otsu multiple-threshold label map.](Images/OtsuThreshold.png)

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
