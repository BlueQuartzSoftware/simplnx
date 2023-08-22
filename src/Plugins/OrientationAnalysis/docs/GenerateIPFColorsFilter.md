# Generate IPF Colors


## Group (Subgroup) ##

Processing (Crystallography)

## Description ##

This **Filter** will generate _inverse pole figure_ (IPF) colors for cubic, hexagonal or trigonal crystal structures. The user can enter the _Reference Direction_, which defaults to [001]. The **Filter** also has the option to apply a black color to all "bad" **Elements**, as defined by a boolean _mask_ array, which can be generated using the [Threshold Objects](@ref multithresholdobjects) **Filter**.

### Originating Data Notes ###

+ TSL (.ang file)
    - If the data originates from a TSL .ang file, then **Elements** that the TSL software could not reliably identify the Euler angles for will have a "Fit of Solution" = 180 and/or an "Image Quality" = 0.0.
    - This means that when the user runs some sort of [threshold](@ref multithresholdobjects) **Filter** the _mask_ will be those **Elements** that have an Image Quality > 0 and/or Fit < 180.0
+ HKL (.ctf file)
    - If the data originates from an HKL (or Bruker) system (.ctf file) then bad voxels can typically be found by setting "Error" > 0
    - This means that when the user runs some sort of [threshold](@ref multithresholdobjects) **Filter** the _mask_ will be those **Elements** that have an Error = 0


-----

![IPF Color Triangle](Images/IPFFilterLegend.png)

-----

![Example Data Set](Images/IPFColor_1.png)

-----

## Parameters ##

| Name | Type | Description |
|------|------| ----------- |
| Reference Direction | float (3x) | The reference axis with respect to compute the IPF colors |
| Apply to Good Elements Only (Bad Elements Will Be Black) | bool | Whether to assign a black color to "bad" **Elements** |

## Required Geometry ##

Not Applicable

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Element Data Array** | Euler Angles | float | (3)  | Three angles defining the orientation of the **Element** in Bunge convention (Z-X-Z) |
| **Element Data Array** | Phases | int32 | (1) | Phase Id specifying the phase of the **Element** |
| **Element Data Array** | Mask | bool | (1) | Used to define **Elements** as *good* or *bad*. Only required if _Apply to Good Elements Only (Bad Elements Will Be Black)_ is checked |
| **Ensemble Data Array** | Crystal Structures | uint32 | (1) | Enumeration representing the crystal structure for each **Ensemble** |

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Element Data Array** | IPFColor |  uint8 | (3) | The RGB colors encoded as unsigned chars for each **Element** |

## Example Pipelines ##

+ (10) SmallIN100 Full Reconstruction
+ (04) SmallIN100 Presegmentation Processing
+ (02) Single Hexagonal Phase Equiaxed
+ (03) Single Cubic Phase Rolled
+ INL Export
+ TxCopper_Exposed
+ TxCopper_Unexposed
+ MassifPipeline
+ InsertTransformationPhase
+ Edax IPF Colors
+ (01) Single Cubic Phase Equiaxed
+ (04) Two Phase Cubic Hexagonal Particles Equiaxed
+ (03) SmallIN100 Alignment
+ (06) SmallIN100 Synthetic

## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM.3D Mailing Lists ##

If you need more help with a **Filter**, please consider asking your question on the [DREAM.3D Users Google group!](https://groups.google.com/forum/?hl=en#!forum/dream3d-users)



## Python Filter Arguments

+ module: OrientationAnalysis
+ Class Name: GenerateIPFColorsFilter
+ Displayed Name: Generate IPF Colors

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| cell_euler_angles_array_path | Euler Angles | Three angles defining the orientation of the Element in Bunge convention (Z-X-Z) | complex.ArraySelectionParameter |
| cell_ip_fcolors_array_name | IPF Colors | The name of the array containing the RGB colors encoded as unsigned chars for each Element | complex.DataObjectNameParameter |
| cell_phases_array_path | Phases | Specifies to which Ensemble each cell belongs | complex.ArraySelectionParameter |
| crystal_structures_array_path | Crystal Structures | Enumeration representing the crystal structure for each Ensemble | complex.ArraySelectionParameter |
| good_voxels_array_path | Mask | Path to the data array used to define Elements as good or bad. | complex.ArraySelectionParameter |
| reference_dir | Reference Direction | The reference axis with respect to compute the IPF colors | complex.VectorFloat32Parameter |
| use_good_voxels | Use Mask Array | Whether to assign a black color to 'bad' Elements | complex.BoolParameter |

