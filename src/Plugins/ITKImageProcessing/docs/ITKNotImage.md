# ITK Not Image Filter

Implements the NOT logical operator pixel-wise on an image.

## Group (Subgroup)

ITKImageIntensity (ImageIntensity)

## Description

This class is templated over the type of an input image and the type of the output image. Numeric conversions (castings) are done by the C++ defaults.

Since the logical NOT operation operates only on boolean types, the input type must be implicitly convertible to bool, which is only defined in C++ for integer types, the images passed to this filter must comply with the requirement of using integer pixel type.

The total operation over one pixel will be

```
if( !A )

 {

 return this->m_ForegroundValue;

 }

return this->m_BackgroundValue;

```

Where "!" is the unary Logical NOT operator in C++.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) GItHub site where the community of DREAM3D-NX users can help answer your questions.
