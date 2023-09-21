# Find GBCD 


## Group (Subgroup) ##

Statistics (Crystallographic)

## Description ##

This **Filter** computes the 5D grain boundary character distribution (GBCD) for a **Triangle Geometry**, which is the relative area of grain boundary for a given misorientation and normal. The GBCD can be visualized by using either the **Write GBCD Pole Figure (GMT)** or the **Write GBCD Pole Figure (VTK)** **Filters**.

## Parameters ##

| Name | Type | Description |
|------|------| ----------- |
| GBCD Resolution (Degrees) | float | The resolution in degrees for the GBCD calculation |

## Required Geometry ##

Image + Triangle

## Required Objects (From Triangle Geometry) ##

| Kind                      | Default Name | Type     | Comp. Dims | Description                                 |
|---------------------------|--------------|----------|------------|---------------------------------------------|
| Face Attribute Array | FaceLabels | int32_t | (2) | Specifies which **Features** are on either side of each **Face |
| Face Attribute Array | FaceNormals | double | (3) | Specifies the normal of each **Face |
| Face Attribute Array | FaceAreas | double | (1) | Specifies the area of each **Face |

## Required Objects (From Image Geometry) ##

| Kind                      | Default Name | Type     | Comp. Dims | Description                                 |
|---------------------------|--------------|----------|------------|---------------------------------------------|
| Feature Attribute Array | AvgEulerAngles | float | (3) | Three angles defining the orientation of the **Feature** in Bunge convention (Z-X-Z) |
| Feature Attribute Array | Phases | int32_t | (1) | Specifies to which phase each **Feature** belongs |
| Ensemble Attribute Array | CrystalStructures | uint32_t | (1) | Enumeration representing the crystal structure for each **Ensemble |

## Created Objects ##

| Kind                      | Default Name | Type     | Comp. Dims | Description                                 |
|---------------------------|--------------|----------|------------|---------------------------------------------|
| Ensemble Attribute Array | GBDC | Double | (n,m,l,o,q,r) | 5 parameter GBCD data. The 6<sup>th</sup> component is used internally to track the northern vs. southern hemisphere of the Lambert sphere |


## Example Pipelines ##

+ (04) SmallIN100 GBCD

## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.


