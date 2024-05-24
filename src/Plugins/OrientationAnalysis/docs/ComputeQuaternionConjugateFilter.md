# Generate Quaternion Conjugate

## Group (Subgroup)

Processing (OrientationAnalysis)

## Description

This filter will generate the transpose of a [1x4] *Quaternion* laid out in memory such that < x, y, z >, w. This can be
handy when the user wants to convert the orientation transformation to an opposite effect. The algorihtm will calculate
the conjugate of each quaternion in the array of input quaternions

**NOTES**: Internally DREAM.3D assumes that the internal reference transformation is a **Sample to Crystal**
transformation. If the incoming data was collected in such a way that the orientation representation that is stored (
Quats, Eulers, Orientation Matrix, Rodrigues) was assumed to be a **Crystal to Sample** transformation then this filter
can be applied to a quaternion to convert into a reference frame that DREAM.3D assumes.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
