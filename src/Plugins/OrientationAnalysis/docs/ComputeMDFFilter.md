# Compute MDF

## Group (Subgroup)

Statistics (Crystallography)

## Description

This **Filter** computes the correlated Misorientation Distribution Function (MDF) for each **Ensemble**/Phase from a segmented microstructure. The MDF describes the relative frequency with which pairs of neighboring **Features** are misoriented from one another by a given angle, and is "correlated" because it is measured directly from **Feature**-boundary misorientations rather than assembled from an uncorrelated combination of single-orientation statistics.

The **Filter** scans every **Cell** face of an **Image Geometry** and identifies the boundary faces where the two neighboring **Cells** belong to different **Features** (both **Feature Ids** greater than 0) but the same **Phase**. At each such boundary face, the misorientation between the Euler angles of the two neighboring **Cells** is computed and accumulated into a per-phase kernel density estimate. Once every boundary face has been visited, each phase's estimate is evaluated to produce an MDF over the phase's Laue-symmetric misorientation bin grid, along with a smooth angle-distribution curve suitable for plotting or comparison to a random texture.

This **Filter** reproduces the default behavior of MTEX 6.1.0's `calcDensity` **as applied to grain-boundary correlated misorientations**:

- **Kernel**: A de la Vallee Poussin kernel is used for the density estimate, matching the MTEX default kernel shape. The width of the kernel is controlled by the *Kernel Halfwidth (Degrees)* parameter (default *10* degrees).
- **Symmetrization**: Each accumulated misorientation is symmetrized using the grain-exchange (antipodal) symmetry between the two neighboring **Features** as well as the full crystal symmetry (Laue class) of the phase, consistent with MTEX's treatment of grain-boundary misorientation.
- **Binning**: The resulting density is evaluated at the misorientation bin centers of EbsdLib's MDF grid for the phase's Laue class (for example, 5,832 bins for a cubic-m3m Laue class). This produces one MDF value per bin.
- **Weighting**: By default every boundary face contributes equally to the density estimate, matching MTEX's default behavior. If *Weight by Boundary Face Area* is enabled, each boundary face instead contributes proportionally to its physical area (computed from the **Image Geometry**'s voxel spacing), which is useful when boundary segmentation produces **Cell** faces of markedly different sizes.

Only **Cell** pairs that share a **Phase** are used to build a given phase's MDF; boundary faces between **Features** of different phases are skipped, since a correlated single-phase MDF is not well defined across a phase boundary.

## How the MDF Is Computed

For each phase, the **Filter** builds the MDF in four steps:

1. **Collect boundary misorientations.** Every **Cell** face where the two neighboring **Cells** belong to different **Features** of the same **Phase** contributes one misorientation, computed directly from the two **Cells'** orientations. This is the "correlated" part: the misorientations come from real neighboring-grain pairs, not from randomly combining single-grain orientations.

2. **Fold to the fundamental zone and assign a grid cell.** Each misorientation is reduced into the misorientation fundamental zone of the phase's Laue class (applying crystal and grain-exchange symmetry so equivalent misorientations map to a single canonical representative) and then assigned to the nearest cell of a **fixed discrete grid**. This is EbsdLib's MDF grid: a fixed number of cells that depends only on the Laue class — for example **5,832 cells (18 × 18 × 18)** for cubic *m-3m*, which works out to roughly **3–4° of misorientation per cell**. The contributing weight of every boundary face landing in a cell is summed there. This "snap to a grid cell" step is why the stored MDF has a finite, fixed resolution.

3. **Estimate the density.** A de la Vallee Poussin kernel (a smooth, bell-shaped weighting function whose width is set by *Kernel Halfwidth*) is centered on each occupied grid cell and summed, with the sum symmetrized over the phase's crystal symmetry and the grain-exchange symmetry between the two grains. Evaluating this kernel sum at every grid-cell center produces the **MDF** array — one density value per cell.

4. **Build the angle-distribution curve.** For each of *Number of Curve Points* angles from 0 up to the largest possible misorientation angle for the Laue class, the density is averaged over all misorientation axes at that angle. This collapses the full 3D MDF down to the 1D **MDF Density** curve. The analytic random-texture distribution for the same Laue class is computed independently to give the **Random Density** curve.

### Resolution of the Output

The MDF this **Filter** produces is a **tabulated density on a fixed grid**, not a continuous mathematical function. Two separate scales set how much detail it can show:

- **The grid spacing** (a few degrees per cell, fixed by the Laue class) is the hard floor on resolution — misorientations are snapped to grid cells before anything else happens, so features finer than one cell cannot be represented no matter what other settings are used.
- **The kernel halfwidth** (default *10* degrees) is the smoothing width applied on top of the grid. Because the default halfwidth is larger than the grid spacing, the kernel — not the grid — normally sets the width of the features you see. Lowering the halfwidth sharpens the estimate (down to the grid floor); raising it smooths the estimate further.

As a result the output curve looks smooth even though it is assembled from discrete grid cells, and a sharp real feature (such as an annealing-twin peak) will appear as a peak roughly one kernel-halfwidth wide, centered within about one grid cell of its true angle. If you need finer angular detail than the grid provides, that is a limitation of the discrete MDF representation rather than of the input data.

## Required Inputs

- **Image Geometry**: The geometry whose **Cell** faces are scanned for **Feature** boundaries.
- **Euler Angles**: A three-component `float32` array giving the Bunge (Z-X-Z) Euler angle orientation of each **Cell**.
- **Phases**: An `int32` array giving the **Ensemble**/Phase index of each **Cell**.
- **Feature Ids**: An `int32` array giving the **Feature** each **Cell** belongs to.
- **Crystal Structures**: A `uint32` **Ensemble** array giving the Laue class of each **Phase**.

## Parameters

- **Kernel Halfwidth (Degrees)**: The halfwidth of the de la Vallee Poussin kernel used for the density estimate. The MTEX default, and the default for this **Filter**, is *10* degrees. Smaller values produce a sharper, more detailed density estimate; larger values produce a smoother, more heavily averaged one.
- **Number of Curve Points**: The number of samples used to build each phase's angle-distribution curve (see below). Default is *200*.
- **Weight by Boundary Face Area**: When *false* (the MTEX default and the default for this **Filter**), every qualifying boundary face contributes equally to the density estimate regardless of its physical size. When *true*, each face is weighted by its physical area, computed from the **Image Geometry** spacing.

## Output

The **Filter** creates a new **DataGroup** (default name *MDF Data*) containing one child **DataGroup** per **Phase**, named `Phase-1`, `Phase-2`, and so on (**Ensemble** 0, the unused/invalid phase, is skipped). Each phase group contains:

- **MDF**: A `float64` array with one value per misorientation bin of the phase's Laue-class MDF grid (for example, 5,832 values for a cubic-m3m Laue class). This is the raw correlated MDF, gridified onto EbsdLib's standard MDF bin layout, and is the array to export or feed into further texture-analysis tools.
- **Angle Distribution**: An **Attribute Matrix** with *Number of Curve Points* tuples, holding three parallel `float64` arrays that together form the columns of a 2D misorientation-angle plot (or CSV export):
  - **Angles**: The misorientation angle at each curve sample, **in degrees**.
  - **MDF Density**: The measured, correlated misorientation density at each angle, evaluated from the phase's kernel density estimate.
  - **Random Density**: The analytic Mackenzie reference density at each angle — the misorientation-angle distribution expected for a **completely random** texture of the phase's Laue class. This curve does not depend on the input microstructure and is provided so the measured MDF can be visually or numerically compared against the random-texture baseline (a ratio, or overlay, of *MDF Density* to *Random Density* is a standard way to identify texture components that deviate from randomness).

Both *MDF Density* and *Random Density* are **unit-mean-relative densities**: each curve is normalized so that its average value over all misorientation angles is 1.0. A value greater than 1 at a given angle indicates that angle is more common than it would be in a hypothetical uniform (equal-density-per-angle) distribution weighted for a random texture, and a value of exactly 1 in *Random Density* corresponds to the expected frequency for a perfectly random texture at that angle. Because both curves share this same normalization convention, they may be plotted together on one axis or divided directly to obtain a texture-strength ratio.

If a phase has no qualifying same-phase **Feature** boundaries (for example, a single-**Feature** phase, or a phase entirely isolated from **Features** of the same phase), the **Filter** emits a warning, leaves that phase's **MDF** array filled with zeros, and still populates **Angles** and **Random Density** (since the random reference does not depend on measured data); **MDF Density** is left at zero in this case.

% Auto generated parameter table will be inserted here

## Example Pipelines

## Citations

[Bachmann, F., Hielscher, R., Schaeben, H., 2010. *Texture Analysis with MTEX – Free and Open Source Software Toolbox*. **Solid State Phenomena 160, 63-68**](https://doi.org/10.4028/www.scientific.net/SSP.160.63)

[Mackenzie, J.K., 1958. *Second Paper on Statistics Associated with the Random Disorientation of Cubes*. **Biometrika 45, 229-240**](https://doi.org/10.2307/2333199)

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
