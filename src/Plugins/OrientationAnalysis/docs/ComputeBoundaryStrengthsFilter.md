# Compute Feature Boundary Strength Metrics

## Group (Subgroup)

Statistics (Crystallographic)

## Description

**THIS FILTER ONLY WORKS ON Cubic m-3m LAUE CLASSES**

This **Filter** calculates the same metrics as in the Find Neighbor Slip Transmission Metrics **Filter**.  However, this **Filter** stores the values in the **Face Attribute Matrix** of a **Triangle Geometry**.  The algorithm the **Filter** uses is as follows:

1. Find the two **Features** that are separated by a **Face** in the **Triangle Geometry**
2. Get the average orientation of both of the **Features**
3. Calculate the transmission metrics from both directions (i.e. **Feature** 1 to **Feature** 2 and **Feature** 2 to **Feature** 1)
4. Store the metrics in the **Face** map
5. Repeat for all **Faces**

*Note:* Each metric is calculated twice for the two different directions slip could approach the boundary.  The values are stored on each **Face** in the **Face** map in a way that notes the direction (i.e., when **Feature** 1 has neighbor **Feature** 2 and when **Feature** 2 has neighbor **Feature** 1) because the direction across the boundary between the **Features** affects the value of the metric.

## Luster-Morris Parameter (M' Values)

The values are calculated as presented in [1].

## Fracture Initiation Parameter (fip)

Three (3) *fip* values are computed as outputs to this filter. These are best explained in [2], see page 021012-4

![Fracture Initiation Parameter F1](Images/ComputeNeighborSlipTransmission_F1.png)

![Fracture Initiation Parameter F1spt](Images/ComputeNeighborSlipTransmission_F1spt.png)

![Fracture Initiation Parameter F7.png](Images/ComputeNeighborSlipTransmission_F7.png)


## Citations

[1] [Luster, J., Morris, M.A., 1995. *Compatibility Of Deformation In Two-Phase Ti-Al Alloys: Dependence On Microstructure And Orientation Relationships*. **Metallurgical and Materials Transactions A 26, 1745**](https://link.springer.com/article/10.1007/BF02670762)

[2] [D. Kumar, T. R. Bieler, P. Eisenlohr, D. E. Mason, M. A. Crimp, F. Roters, and D. Raabe. On Predicting Nucleation of Microcracks Due to Slip Twin Interactions at Grain Boundaries in Duplex Near γ-TiAl. Journal of Engineering Materials and Technology, 130(2):021012–12, 2008. doi:10.1115/1.2841620.](https://doi.org/10.1115/1.2841620)

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
