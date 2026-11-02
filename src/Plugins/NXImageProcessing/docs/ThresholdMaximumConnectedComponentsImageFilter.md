# Threshold Maximum Connected Components Image Filter

Finds the threshold value of an image based on maximizing the number of objects in the image that are larger than a given minimal size.

## Group (Subgroup)

ImageProcessing (Segmentation)

## Description

This filter is an iterative auto-thresholding method: it bisection-searches for the threshold value that **maximizes** the number of connected components (objects) in the image that are at least **Minimum Object Size In Pixels** in size, then outputs the binary threshold at that value. This method is based on Topological Stable State Thresholding and is particularly effective when there are a large number of objects in a microscopy image.

At each step of the search, the image is binary-thresholded at a candidate threshold value (paired with the fixed **Upper Boundary**), the resulting foreground is labeled into connected components (face connectivity only), and the number of components at least **Minimum Object Size In Pixels** in size is counted. The bisection converges on the threshold value that maximizes this count.

The input array must be single-component (scalar) and may be **any scalar type** (integer or floating point). The output is a fixed **uint8** binary image (**Inside Value** / **Outside Value**) regardless of the input element type. This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Threshold Maximum Connected Components Image Filter, and matches it exactly (byte-identical binary output).

### Minimum Object Size In Pixels

Set the minimum pixel area used to count objects on the image. Only objects that have a pixel area greater than or equal to the minimum pixel area are counted as an object in the optimization portion of this filter. Essentially, it eliminates noise from being counted as an object. The default value is zero.

### Upper Boundary

This class automatically calculates the lower threshold boundary. The upper threshold boundary, inside value, and outside value can be defined by the user, however the standard values are used as default if not set by the user. Default is 65536.0.

### Inside Value

The value assigned to voxels at or above the calculated threshold (and at or below the Upper Boundary). Default is 1.

### Outside Value

The value assigned to voxels outside the calculated threshold range. Default is 0.

## Algorithm

The filter performs a bisection search over candidate lower thresholds. For each candidate it raster-scans the image, encodes foreground runs, joins connected runs with union-find, and counts components meeting the minimum-size requirement. The union-find and component-size tables contain one entry per provisional object run. After selecting the threshold, the filter writes the requested binary inside and outside values in bounded chunks.

For images with more than one Z plane, each connected-component scan reads one Z plane at a time and retains the current and previous plane's run descriptions. For a true two-dimensional image (`Z=1`), a checked scheduler uses at most 64 MiB of full-width row blocks while retaining only two run-encoded rows. Exceptionally wide rows use bounded X tiles and a provisional label image to reconnect neighboring tiles and rows. With an out-of-core input, that scratch uses the registered raw temporary-record store when available and otherwise falls back to the selected array storage format. This keeps cell-scale state disk-backed for out-of-core inputs while preserving the exact threshold search, connectivity, and component-count results.

% Auto generated parameter table will be inserted here

## References

1) Urish KL, August J, Huard J. "Unsupervised segmentation for myofiber counting in immunofluorescent microscopy images". Insight Journal. ISC/NA-MIC/MICCAI Workshop on Open-Source Software (2005) https://insight-journal.org/browse/publication/40
2) Pikaz A, Averbuch, A. "Digital image thresholding based on topological stable-state". Pattern Recognition, 29(5): 829-843, 1996.

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
