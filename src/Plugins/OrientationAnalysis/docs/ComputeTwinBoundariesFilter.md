# Compute Twin Boundaries

## Group (Subgroup)

Statistics (Crystallography)

## Description

This **Filter** identifies all **Triangles** between neighboring **Features** that have a &sigma; = 3 twin relationship.  The **Filter** uses the average orientation of the **Features** on either side of the **Triangle** to determine the *misorientation* between the **Features**.  If the *axis-angle* that describes the *misorientation* is within both the axis and angle user-defined tolerance, then the **Triangle** is flagged as being a twin.  After the **Triangle** is flagged as a twin, the crystal direction parallel to the **Face** normal is determined and compared with the *misorientation axis* if *Compute Coherence* is selected.  The misalignment of these two crystal directions is stored as the incoherence value for the **Triangle** (in degrees). Note that this **Filter** will only extract twin boundaries if the twin **Feature** is the same phase as the parent **Feature**.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
