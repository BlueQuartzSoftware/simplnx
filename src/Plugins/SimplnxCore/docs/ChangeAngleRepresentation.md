# Convert Angles to Degrees or Radians

## Group (Subgroup)

Orientation Analysis (Conversion)

## Description

This **Filter** will multiply the values of every **Element** by a factor to convert *degrees to radians* or *radians to degrees*.  The user needs to know the units of their data in order to use this **Filter** properly.

### Example Usage

2D data files that are in *HKL*'s .ctf format use degrees. 3D data files in *HKL*'s .ctf format use radians. All files in *TSL*'s .ang format use radians. If other file types are used, determine the units before running this **Filter**.

Generally, all **Filters** within DREAM.3D use radians as a default unit, but **Filters** may be added that require the Euler angles to be in degrees and this **Filter** will allow the user to move back-and-forth between the units.

If the wrong conversion is selected (e.g., the user chooses *degrees to radians*, but their data is already in radians), the **Filter** will apply the conversion anyway and will result in erroneous data being used for the rest of the pipeline. If unsure about the units, check with the software supplier.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ TxCopper_Exposed
+ TxCopper_Unexposed

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
