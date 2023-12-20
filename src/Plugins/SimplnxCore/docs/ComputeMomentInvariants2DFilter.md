# Compute MomentInvariants (2D)

## Group (Subgroup)

Statistics (Statistics)

## Description

This **Filter** computes the 2D Omega-1 and Omega 2 values from the *Central Moments* matrix and optionally will normalize the values to a unit circle and also optionally save the *Central Moments* matrix as a DataArray to the *Cell Feature Attribute Matrix*. Based off the paper by MacSleyne et. al [1], the algorithm will calculate the 2D central moments for each feature starting at *feature id = 1*. Because *feature id 0* is of special significance and typically is a matrix or background it is ignored in this filter. If any feature id has a Z Delta of > 1, the feature will be skipped. This algorithm works strictly in the XY plane and is meant to be applied to a 2D image. Using the research from the cited paper certain shapes can be detected using the Omega-1 and Omega-2 values. An example usage is finding elliptical shapes in an image:

See below figure from [1] that can help the user classify objects.

![Example appllication of filter to identify elliptical particales (red) which are differentiated from non-elliptical particals (purple)](Images/ComputeMomentInvariants_Fig1.png)

![Example appllication of filter to identify elliptical particales (red) which are differentiated from non-elliptical particals (purple)](Images/ComputeMomentInvariants2D.png)

% Auto generated parameter table will be inserted here

# Citations

[1] J.P. MacSleyne, J.P. Simmons, M. De Graef, *On the use of 2-D moment invariants for the automated classification of particle shapes*, Acta Materialia, Volume 56, Issue 3, February 2008, Pages 427-437, ISSN 1359-6454, [http://dx.doi.org/10.1016/j.actamat.2007.09.039.](http://dx.doi.org/10.1016/j.actamat.2007.09.039.)
[http://www.sciencedirect.com/science/article/pii/S1359645407006702](http://www.sciencedirect.com/science/article/pii/S1359645407006702)

# Acknowledgements

The authors would like to thank Dr. Marc De Graef from Carnegie Mellon University for enlightening discussions and a concrete implementation from which to start this filter.
## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) GItHub site where the community of DREAM3D-NX users can help answer your questions.
