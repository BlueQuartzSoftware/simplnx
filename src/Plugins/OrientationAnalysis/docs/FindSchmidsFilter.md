# Find Schmid Factors

## Group (Subgroup)

Statistics (Crystallographic)

## Description

This **Filter** calculates the Schmid factor of each **Feature** given its average orientation and a user defined loading axis. The Schmid Factor is the combination of the component of the axial force *F* that lies parallel to the slip direction and the component that lies perpendicular to the slip plane.  The equation for the Schmid Factor is given as:

Schmid Factor = (cos &phi; cos &lambda;)

*The angle &phi; is the angle between the tensile axis and the slip plane normal, and &lambda; is the angle between the tensile axis and the slip direction in the slip plane.*

The **Filter** determines the Schmid factor for each **Feature** by using the above equation for all possible slip systems (given the **Feature's** crystal structure).  The largest Schmid factor from all of the slip systems is stored for the **Feature**. Only the Schmid factor is used in determining which slip system's Schmid factor to report.  The critical resolved shear stress for the different slip systems is not considered.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (05) SmallIN100 Crystallographic Statistics

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) GItHub site where the community of DREAM3D-NX users can help answer your questions.
