# Compute Volume Fractions of Ensembles

## Group (Subgroup)

Statistics (Morphological)

## Description

This filter determines the volume fraction of each **Ensemble**. An **Ensemble** is a grouping of **Features** that share a common characteristic, such as a crystallographic phase. The filter counts the number of **Cells** belonging to each **Ensemble** and divides by the total number of **Cells**.

On an **Image Geometry** every **Cell** has the same volume, so this count fraction is identical to the true volume fraction: dividing each **Ensemble**'s **Cell** count by the total **Cell** count gives the same result as dividing each **Ensemble**'s summed **Cell** volume by the total volume. The output is therefore a per-**Ensemble**, dimensionless volume fraction in the range [0, 1], where all of the fractions sum to 1.

The result is written to the *Volume Fractions* array in the selected **Ensemble Attribute Matrix**.

### Required Input Sources

- **Cell Phases** -- a per-**Cell** integer array specifying which **Ensemble** each **Cell** belongs to. This array is typically read directly from EBSD data; the related feature-level array is produced by [Compute Feature Phases](ComputeFeaturePhasesFilter.md).

## Algorithm

Cell phase IDs are read in fixed-size sequential batches. Only one count per ensemble and one output value per ensemble are retained in memory, so working memory does not scale with the number of **Cells**. The resulting ensemble array is written with one checked bulk transfer. This same path is used for in-memory and disk-backed arrays and propagates all storage failures.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
