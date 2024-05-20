# Compute Feature Reference C-Axis Misalignments

## Group (Subgroup)

Statistics (Crystallographic)

## Description

This **Filter** calculates the misorientation angle between the C-axis of each **Cell** within a **Feature** and the average C-axis for that **Feature** and stores that value for each **Cell**.  The average and standard deviation of those values for all **Cells** belonging to the same **Feature** is also stored for each **Feature**.

This filter requires at least one Hexagonal crystal structure phase (Hexagonal-Low 6/m or Hexagonal-High 6/mmm). Although it is not recommended, you can give input data with mixed phase types and all non hexagonal phases will be skipped in the calculations.

% Auto generated parameter table will be inserted here

## Example Pipelines

EBSD_Hexagonal_Data_Analysis

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
