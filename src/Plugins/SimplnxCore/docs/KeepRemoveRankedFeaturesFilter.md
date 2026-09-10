# Keep/Remove Ranked Features

## Description

This **Filter** keeps or removes a count or percentage of **Features** based on their **rank** within
a chosen scalar **Feature** level array. For example, "keep the 10 largest grains by *NumElements*",
or "remove the smallest 10% of grains".

This is a **rank** threshold, not a **value** threshold. It answers *"which are the 10 biggest?"*
rather than *"which are bigger than 100 voxels?"*. The cut therefore depends on the whole population
rather than on each **Feature** independently. For a value threshold, use **Remove Minimum Size
Features** or **Multi-Threshold Objects** instead.

Ranking is not limited to size. Any scalar **Feature** array works — *EquivalentDiameters*,
*AspectRatios*, *NumNeighbors*, *Omega3s*, average misorientation, and so on.

Removed **Features** have their **Cell** level *FeatureIds* set to 0. If *Fill-in Removed Features*
is enabled, the vacated **Cells** are instead assigned to their surviving neighbors by iterative
dilation, so no **Cell** is left unassigned. In both cases the **Feature** **Attribute Matrix** is
then compacted and the surviving **Features** are renumbered contiguously starting at 1.

### Operation and Rank From

*Operation* and *Rank From* are independent. The **selected** set is always the *k* **Features** the
ranking picked; *Operation* decides only what happens to that set.

| Operation | Rank From | Result |
|-----------|-----------|--------|
| Keep | Largest | only the *k* largest survive |
| Keep | Smallest | only the *k* smallest survive |
| Remove | Largest | the *k* largest are removed |
| Remove | Smallest | the *k* smallest are removed |

Note that "Keep the 2 largest" and "Remove the 2 smallest" leave the same **Features** behind only
when there are exactly 4 **Features**. In general the two are different operations.

### Selection Criterion

- **Feature Count** — an exact number of **Features**. If the requested count exceeds the number of
  **Features** present it is clamped, and a warning reports the clamp. This lets one pipeline be
  reused across datasets with differing **Feature** counts.
- **Percent of Feature Count** — a percentage of the *number* of **Features**. 10% of 500 **Features**
  selects 50. The result is rounded to the nearest whole **Feature** and never rounds down to zero.
- **Percent of Summed Value** — **Features** are accumulated in rank order until their running total
  reaches the given percentage of the *summed* ranking value. The **Feature** that crosses the
  threshold is included, so the selection always reaches the target and any percentage above zero
  selects at least one **Feature**.

  This criterion answers "which **Features** make up 70% of the material?" rather than "which are the
  top 70% of **Features**?". It is only physically meaningful for *extensive* quantities such as
  *NumElements* or *Volumes*; applying it to *AspectRatios* or a misorientation produces a number
  with no physical interpretation. All values must be non-negative, since a cumulative fraction is
  undefined when values can cancel each other out.

### Ties

Exactly *k* **Features** are always selected. When several **Features** share the same ranking value
at the cutoff, ties are broken by ascending **Feature** id, which makes results reproducible across
runs and platforms. A warning is emitted whenever a tie straddles the cutoff, because the choice
among the tied **Features** is arbitrary. Ties are common in integer arrays such as *NumElements*.

## Notes

- **Memory:** ranking requires three temporary buffers sized by the **Feature** count — the ranking
  values, a sort order, and a removal flag per **Feature**. For 10 million **Features** with a 32-bit
  ranking array that totals roughly 120 MB. No **Cell** sized buffer is allocated unless *Fill-in
  Removed Features* is enabled, which additionally needs one 32-bit value per **Cell**.
- **Feature** 0 is not a real **Feature**. It is excluded from the ranking, from the percentage
  denominators, and from removal.
- Any **NeighborList** arrays in the **Feature** **Attribute Matrix** cannot be maintained through the
  renumbering and will be removed. A warning lists them during preflight.
- A ranking array containing non-finite values (NaN or infinity) is rejected, because such values
  have no ordering and cannot be sorted.
- The filter warns during preflight when the chosen settings would remove nothing, and when they
  would remove every **Feature**. The latter is an error at execute time, since it would leave
  nothing behind.
- Every value in *Cell Feature Ids* must be in the range 0 through (number of **Feature** tuples - 1),
  and the array must hold one value per **Cell** of the selected geometry. A value outside that range
  stops the filter with error *-45435*, and a tuple-count mismatch with error *-45437*, before any data
  is modified. These checks are shared with [Remove/Extract Flagged Features](RemoveFlaggedFeaturesFilter.md),
  which also documents the fill rules (background **Cells** are never filled but can be fill sources) and
  the *-45436* error that stops a fill that cannot make progress.
- The *Cell Feature Ids* array is always copied by the fill, even when it is listed in *Attribute Arrays
  to Ignore*; the filter warns (*-45438*) and removes it from the list.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
