# Robust Automatic Threshold

## Group (Subgroup)

DREAM3D Review (Threshold)

## Description

This **Filter** automatically computes a threshold value for a scalar **Attribute Array** based on the array's gradient magnitude, producing a boolean array that is *false* where the input array is less than the threshold value and *true* otherwise.  The threshold value is computed using the following equation:

![\f[ T = \sum_{i = 1}^{n} \frac{a_{i} g_{i}}{g_{i}} \f]](Images/latex24.png)

where \f$ a \f$ is the input array, \f$ g \f$ is the gradient magnitude array, \f$ n \f$ is the length of the input array, and \f$ T \f$ is the computed threshold value.  Computing a threshold in this manner will generally partition the input array where its gradient is highest.  Gradients may be computed using the Find Derivatives **Filter**.  The gradient magnitude may then be found by computing the 2-norm of the gradient.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
