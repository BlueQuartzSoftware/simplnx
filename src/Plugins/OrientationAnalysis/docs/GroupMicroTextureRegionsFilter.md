# Group MicroTexture Regions

## Group (Subgroup)

Reconstruction Filters (Grouping)

## Description

This Filter groups neighboring **Features** that have c-axes aligned within a user-defined tolerance. The algorithm for grouping the **Features** is analogous to the algorithm for segmenting the **Features** — only the average orientation of the **Features** is used instead of the orientations of the individual **Cells**, and the criterion for grouping only considers the alignment of the c-axes. The user can specify a tolerance for how closely aligned the c-axes must be for neighbor **Features** to be grouped.

NOTE: This filter is intended for use with *Hexagonal* materials. While the c-axis is actually just referring to the <001> direction and thus will operate on any symmetry, the utility of grouping by <001> alignment is likely only important/useful in materials with anisotropy in that direction (like materials with *Hexagonal* symmetry). Features whose phase resolves to anything other than *Hexagonal_High* are silently left ungrouped.

### Choosing the C-Axis Alignment Tolerance

The default tolerance is **20 degrees**, which is a typical starting point for microtexture-region
segmentation in real EBSD data. Smaller values (a few degrees) will fragment genuine MTRs into many
small groups. Tune the value for your alloy and scan rather than assuming the default is optimal.

### How a region grows: neighbor-to-neighbor vs. running average

With **Group C-Axes With Running Average** off (the default), each candidate **Feature** is compared
against the **Feature** it touches — the one that most recently joined the region. Grouping is
therefore the *transitive closure* of the pairwise tolerance test along chains of neighbors: with a
20 degree tolerance, features at 0, 15 and 30 degrees will all end up in one region even though the
two end members are 30 degrees apart. A region can drift arbitrarily far from its starting
orientation, one small step at a time.

With the option on, each candidate is instead compared against the volume-weighted average c-axis of
the whole region so far, which anchors the region and bounds that drift. Which behavior is correct
depends on the analysis: use neighbor-to-neighbor when you want to enforce a misorientation
requirement between adjacent features, and the running average when you want an ensemble-average
orientation for the region (for example when feeding structure-property models).

Both versions group by **Laue class**, not by phase identity. Two distinct phases that both resolve
to *Hexagonal_High* — for example primary alpha and transformed beta in a titanium alloy — will be
grouped together. That is intended.

### Randomization of Parent Ids

By default the filter assigns parent ids deterministically in the order features are picked as BFS seeds, so identical inputs produce identical parent ids. Set **Randomize Parent Ids** to true to randomly permute the assigned parent ids (useful when feeding the output straight into a color-mapped visualization where adjacent groups should not share the same color by accident). For reproducible randomization, enable **Use Seed for Random Generation** and supply a **Seed** value; the seed actually used is also written to a top-level array (default name `_Group_MicroTexture_Regions_Seed_Value_`) so the run can be replayed.

% Auto generated parameter table will be inserted here

## References

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
