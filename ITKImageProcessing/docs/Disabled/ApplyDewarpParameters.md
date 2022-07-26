# Apply Dewarp Parameters #


## Group (Subgroup) ##

Processing (Processing)

## Description ##

This **Filter** takes a set of dewarp parameters stored in a **DataArray** generated in the **CalcDewarpParameters** filter and uses them to dewarp tiles within a GridMontage.  The parameters are expected to be in the format

> x' = a<sub>0</sub> * x + a<sub>1</sub> * y + a<sub>2</sub> * x<sup>2</sup> + a<sub>3</sub> * y<sup>2</sup> + a<sub>4</sub> * xy + a<sub>5</sub> * x<sup>2</sup>y + a<sub>6</sub> * xy<sup>2</sup>
> 
> y' = b<sub>0</sub> * x + b<sub>1</sub> * y + b<sub>2</sub> * x<sup>2</sup> + b<sub>3</sub> * y<sup>2</sup> + b<sub>4</sub> * xy + b<sub>5</sub> * x<sup>2</sup>y + b<sub>6</sub> * xy<sup>2</sup>

where X and Y are offset by half of the target tile dimensions instead of starting at the tile's top-left corner.  The target tile dimensions are taken from the first or second row/column based on the number of rows and columns in the montage.  If there are more than two rows, the target height is taken from the second row.  Otherwise, the target height is taken from the first row.  If there are more than two columns, the target width is taken from the second column.  Otherwise, the target width is taken from the first column.  It is done this way because the last row and column are not guaranteed to be full size, while the first row and column may be larger than the others with said region containing possible junk data.  By finding the appropriate tile size, the correct offsets for the algorithm can be determined and junk data discarded.

Warping is done around the center of each tile using a set of global im_dim_x/y values for the montage.  These values are still used for the first and last row/col where tiles may be larger or smaller than the others in order to maintain consistent warping.  For the first row and column, the offset based on (im_dim_x, im_dim_y) tile sizes is extended with any additional width or height beyond the target size being added or subtracted accordingly.  For cases where the last row or column do not contain the entire (im_dim_x, im_dim_y) tile size, the data is still warped around the center point of an (im_dim_x, im_dim_y) sized tile

The value for each point in the transformed montage is determined by mapping each point in the new data to a point in the original data instead of mapping the old data to the new tiles and blending the overlaps.  The calculations for the original data points from the new is as follows:

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
|------|------|-------------|
| **GridMontage** | GridMontage | Montage to dewarp |
| **Attribute Matrix Name** | Text | Name of the AttributeMatrix that should be dewarped for each tile |
| **Transform Array** | DataArrayPath | DataArrayPath to the transformation parameters |

## Required Geometry ##
Not Applicable

## Required Objects ##
Not Applicable

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Data Container Prefix** | Transformed_ | N/A | N/A | Prefix to apply for dewarped DataContainers and GridMontage |
| **Attribute Array** | Mask | uint8_t | (1) | Name of the created Mask array for each dewarped DataContainer |


## Example Pipelines ##

None

## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists ##

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
[DREAM.3D Users Google group!](https://groups.google.com/forum/?hl=en#!forum/dream3d-users)