# Compute Feature Boundary Strength Metrics

## Group (Subgroup)

Statistics (Crystallographic)

## Description

This **Filter** calculates the same metrics as in the Find Neighbor Slip Transmission Metrics **Filter**.  However, this **Filter** stores the values in the **Face Attribute Matrix** of a **Triangle Geometry**.  The algorithm the **Filter** uses is as follows:

1. Find the two **Features** that are separated by a **Face** in the **Triangle Geometry**
2. Get the average orientation of both of the **Features**
3. Calculate the transmission metrics from both directions (i.e. **Feature** 1 to **Feature** 2 and **Feature** 2 to **Feature** 1)
4. Store the metrics in the **Face** map
5. Repeat for all **Faces**

*Note:* Each metric is calculated twice for the two different directions slip could approach the boundary.  The values are stored on each **Face** in the **Face** map in a way that notes the direction (i.e., when **Feature** 1 has neighbor **Feature** 2 and when **Feature** 2 has neighbor **Feature** 1) because the direction across the boundary between the **Features** affects the value of the metric.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
