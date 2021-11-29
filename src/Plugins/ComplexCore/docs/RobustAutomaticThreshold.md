# Robust Automatic Threshold #

## Group (Subgroup) ##

DREAM3D Review (Threshold)

## Description ##

This **Filter** automatically computes a threshold value for a scalar **Attribute Array** based on the array's gradient magnitude, producing a boolean array that is _false_ where the input array is less than the threshold value and _true_ otherwise.  The threshold value is computed using the following equation:

\f[ T = \sum_{i = 1}^{n} \frac{a_{i} g_{i}}{g_{i}} \f]

where \f$ a \f$ is the input array, \f$ g \f$ is the gradient magnitude array, \f$ n \f$ is the length of the input array, and \f$ T \f$ is the computed threshold value.  Computing a threshold in this manner will generally partition the input array where its gradient is highest.  Gradients may be computed using the [Find Derivatives](@ref findderivatives) **Filter**.  The gradient magnitude may then be found by computing the [2-norm of the gradient](@ref findnorm).

## Parameters ##

None

## Required Geometry ###

None

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| Any **Attribute Array** | None | Any except bool | (1) | **Attribute Array** to threshold |
| **Attribute Array** | None | float | (1) | Gradient magnitude of input **Attribute Array** |

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Attribute Array** | Mask | bool | (1) | Mask array produced after application of the computed robust threshold to the input **Attribute Array** |

## Example Pipelines ##



## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists ##

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
