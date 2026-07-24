# Compute Number of Features

## Group (Subgroup)

Statistics (Morphological)

## Description

This **Filter** determines the number of **Features** in each **Ensemble** by counting the **Features** assigned to each **Ensemble**. An **Ensemble** is a group of **Features** that share common characteristics — most commonly a *phase* (a distinct material or crystallographic constituent in the sample). The filter reads the per-**Feature** phase array and tallies how many **Features** belong to each phase.

The output is a single-component count array indexed by **Ensemble** (phase) Id: tuple *i* holds the number of **Features** belonging to **Ensemble** *i*.

### Required Input Sources

- **Feature Phases** -- the per-**Feature** phase (**Ensemble** Id) array, produced by [Compute Feature Phases](ComputeFeaturePhasesFilter.md).

% Auto generated parameter table will be inserted here

## Example Pipelines

+ INL Export

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
