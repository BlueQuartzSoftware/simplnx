# Convert Quaternion Order

## Group (Subgroup)

OrientationAnalysis (Conversions)

## Description

Internally DREAM.3D assumes that a quaternion is laid out in the order such that < x, y, z >, w or Vector-Scalar ordering. Codes and algorithms external to DREAM.3D may store quaternions in the opposite or Scalar-Vector order (w < x,y,z >). This filter will allow the user to easily convert imported Quaternions into the representation that DREAM.3D expects.

For Example if the user has imported quaternion data in the form of Scalar-Vector then they would run this filter using a conversion type of **ToVectorScalar** and using the generated quaternions in subsequent filters. If the user wanted to then write out the Quaternions in the Scalar-Vector form then could add this filter again to the end of the pipeline (but before writing out data) to convert the Vector-Scalar quaternions array (assuming something modified the array).

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
