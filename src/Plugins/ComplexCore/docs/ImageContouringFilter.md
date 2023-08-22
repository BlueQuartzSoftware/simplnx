# Contour (Image Contouring)

## Group (Subgroup)

Visual Analysis

## Description

This filter will draw a 3 dimensional contouring line through an Image Geometry based on an input value.

Here's what the results look like:

![3D-Contouring](Images/3D-contouring.png)

## Parameters

| Name | Type | Description |
|------|------| ----------- |
| Contour value | float64 | This is the thresholding value that will be used to create the *Contouring Geometry* |

## Required Geometry

Image

## Required Objects

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Data Array** | Data Array to Contour | Any | (1) | This is the array that will be parsed in order to create the contouring geometry, this is what the *Contour Value* will be compared against |

## Created Objects

| Kind | Default Name | Type | Description |
|------|--------------|------|-------------|
| Geometry | Contouring Geometry | TriangleGeom | This stores the 3D contouring line and the normals for the vertices |

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM.3D Interaction

If you need more help with a **Filter**, please consider asking your question on the [DREAM.3D Help Forum!](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues)


## Python Filter Arguments

+ module: complex
+ Class Name: ImageContouringFilter
+ Displayed Name: Contour

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| iso_val_geometry | Contour Value | The value to contour on | complex.Float64Parameter |
| new_triangle_geometry_name | Name of Output Triangle Geometry | This is where the contouring line will be stored | complex.DataObjectNameParameter |
| selected_data_array | Data Array to Contour | This is the data that will be checked for the contouring iso value | complex.ArraySelectionParameter |
| selected_image_geometry | Selected Image Geometry | The target geometry | complex.GeometrySelectionParameter |

