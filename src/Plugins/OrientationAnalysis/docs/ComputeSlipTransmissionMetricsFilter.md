# Compute Neighbor Slip Transmission Metrics

## Group (Subgroup)

Statistics (Crystallography)

## Description

This **Filter** calculates a suite of *slip transmission metrics* for each pair of neighboring **Features** (grains). These metrics quantify how easily plastic deformation (slip) can transfer across a grain boundary from one grain to another, based on the geometric alignment of their slip systems.

When a material deforms, slip occurs along specific crystallographic planes and directions within each grain. At grain boundaries, slip in one grain must either transmit into the neighboring grain, be blocked, or nucleate new slip. The degree of geometric compatibility between the slip systems on each side of a boundary is a key factor in determining whether transmission occurs or whether stress concentrations develop that can lead to crack initiation.

### Cubic Materials Only

This filter only works on **cubic m-3m Laue classes**.

### How This Filter Works

1. For each **Feature**, the filter retrieves its average orientation and list of neighboring **Features**
2. For each neighbor pair, the filter evaluates all possible combinations of slip systems across the boundary
3. The transmission metrics are computed in both directions (Feature 1 → Feature 2 and Feature 2 → Feature 1) because the directionality affects the result
4. Results are stored as lists per **Feature**, one value per neighbor

### Note

The metrics are calculated using the **average orientations** of neighboring grains, not the local orientations near the boundary.

### Luster-Morris Parameter (M')

The Luster-Morris parameter (M') measures the geometric compatibility between two slip systems across a boundary. A value of 1.0 indicates perfect alignment; lower values indicate poorer compatibility. Calculated as presented in [1].

### Fracture Initiation Parameters (F1, F1spt, F7)

Three fracture initiation parameter (fip) values are computed, which relate to the likelihood of crack nucleation at grain boundaries due to slip incompatibility. These are defined in [2] (see page 021012-4):

![Fracture Initiation Parameter F1](Images/ComputeNeighborSlipTransmission_F1.png)

![Fracture Initiation Parameter F1spt](Images/ComputeNeighborSlipTransmission_F1spt.png)

![Fracture Initiation Parameter F7](Images/ComputeNeighborSlipTransmission_F7.png)

### Comparison with Compute Feature Boundary Strength Metrics

This filter stores results per **Feature** (in neighbor lists). The [Compute Feature Boundary Strength Metrics](ComputeBoundaryStrengthsFilter.md) filter computes the same metrics but stores results per **Face** on a **Triangle Geometry**, which is useful for visualization on the boundary mesh.

### Required Input Sources

- **Neighbor List** -- produced by [Compute Feature Neighbors](../SimplnxCore/ComputeFeatureNeighborsFilter.md) or [Compute Feature Neighborhoods](../SimplnxCore/ComputeNeighborhoodsFilter.md).
- **Average Quaternions** -- produced by [Compute Average Orientations](ComputeAvgOrientationsFilter.md).
- **Feature Phases** -- produced by [Compute Feature Phases](../SimplnxCore/ComputeFeaturePhasesFilter.md).
- **Crystal Structures** -- ensemble-level array read from EBSD data or created by [Create Ensemble Info](CreateEnsembleInfoFilter.md).

## References

[1] [Luster, J., Morris, M.A., 1995. *Compatibility Of Deformation In Two-Phase Ti-Al Alloys: Dependence On Microstructure And Orientation Relationships*. **Metallurgical and Materials Transactions A 26, 1745**](https://link.springer.com/article/10.1007/BF02670762)

[2] [D. Kumar, T. R. Bieler, P. Eisenlohr, D. E. Mason, M. A. Crimp, F. Roters, and D. Raabe. On Predicting Nucleation of Microcracks Due to Slip Twin Interactions at Grain Boundaries in Duplex Near γ-TiAl. Journal of Engineering Materials and Technology, 130(2):021012–12, 2008. doi:10.1115/1.2841620.](https://doi.org/10.1115/1.2841620)

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
