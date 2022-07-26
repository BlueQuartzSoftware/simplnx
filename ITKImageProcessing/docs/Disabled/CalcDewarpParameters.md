# Calculate Dewarp Parameters #


## Group (Subgroup) ##

Processing (Processing)

## Description ##

This **Filter** uses a derivative of **itk::AmoebaOptimizer** to calculate the parameters required for dewarping a specified montage.  It does this by performing an **itk::FFTConvolutionImageFilter** over the overlaps in a **GridMontage** for each set of parameters checked by the amoeba filter.  The amoeba filter then tries to find the set of parameters that maximizes the sum of the max values from each overlap.  These max values are used to determine how similar the overlapping region is.  Maximizing the summation is done to find the set of dewarp parameters resulting in the best-matching overlaps.

Once the amoeba optimizer is completed, a new **DataContainer**, **AttributeMatrix**, and **DataArray** are generated to store the dewarp parameters.  The actual application of dewarping is not performed in this filter.

The process for finding the parameters works centering the tile around **(0,0)** and warping around that location each tile using components from a 3rd degree polynomial as described below to find the location of the corresponding pixel in the original data for each set of parameters tested using an amoeba optimizer.

> x'<sub>n</sub> = x<sub>new</sub> - im_dim_x / 2
>
> y'<sub>n</sub> = y<sub>new</sub> - im_dim_y / 2

> x'<sub>old</sub> = a<sub>0</sub> * x'<sub>n</sub> + a<sub>1</sub> * y'<sub>n</sub> + a<sub>2</sub> * x'<sub>n</sub><sup>2</sup> + a<sub>3</sub> * y'<sub>n</sub><sup>2</sup> + a<sub>4</sub> * x'<sub>n</sub>y'<sub>n</sub> + a<sub>5</sub> * x'<sub>n</sub><sup>2</sup>y'<sub>n</sub> + a<sub>6</sub> * x'<sub>n</sub>y'<sub>n</sub><sup>2</sup>
> 
> y'<sub>old</sub> = b<sub>0</sub> * x'<sub>n</sub> + b<sub>1</sub> * y'<sub>n</sub> + b<sub>2</sub> * x'<sub>n</sub><sup>2</sup> + b<sub>3</sub> * y'<sub>n</sub><sup>2</sup> + b<sub>4</sub> * x'<sub>n</sub>y'<sub>n</sub> + b<sub>5</sub> * x'<sub>n</sub><sup>2</sup>y'<sub>n</sub> + b<sub>6</sub> * x'<sub>n</sub>y'<sub>n</sub><sup>2</sup>

> x<sub>old</sub> = x'<sub>old</sub> + im_dim_x / 2
>
> y<sub>old</sub> = y'<sub>old</sub> + im_dim_y / 2



## Parameters ##

| Name | Type | Description |
|------|------|------|
| Parameter Name | Parameter Type | Description of parameter... |
| **Montage Name** | GridMontage | **GridMontage** to dewarp |
| **Max Iterations** | Integer | Maximum number of iterations to perform |
| **Delta** | Integer | Maximum offset in cells when calculating the initial step size |
| **Fractional Convergence Tolerance** | Float | Fractional difference between min/max values for convergence |
| **Specify Initial Simplex** | LinkedBoolean | Enables or disables **X Factors** and **Y Factors** |
| **X Factors** | FloatVec7Type | `a` parameters for calculating `x'` |
| **Y Factors** | FloatVec7Type | `b` parameters for calculating `y'` |
| **Attribute Matrix Name** | Text | **AttributeMatrix** name containing the **IPFColors** |
| **IPF Colors Array Name** | Text | **IPFColors** array name within the target **AttributeMatrix** |

## Required Geometry ##
Not Applicable

## Required Objects ##
Not Applicable

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|-------------|---------|-----|
| **Data Container** | Dewarp Data | N/A | N/A | DataContainer holding dewarp parameters |
| **Attribute Matrix** | Transform Matrix | Generic | N/A | AttributeMatrix holding dewarp parameters |
| **Element Attribute Array** | Transform | Double | (14) | DataArray holding dewarp parameters |


## Example Pipelines ##

None

## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists ##

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
[DREAM.3D Users Google group!](https://groups.google.com/forum/?hl=en#!forum/dream3d-users)