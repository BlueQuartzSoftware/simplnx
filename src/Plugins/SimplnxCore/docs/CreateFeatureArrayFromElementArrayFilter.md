# Create Feature Array from Element Array

## Group (Subgroup)

Core (Memory Management)

## Description

This filter promotes **Element** (**Cell**) level data up to the **Feature** level. For a selected **Element** **Data Array**, the filter writes one value (or one tuple, for multi-component arrays) per **Feature** into a new **Data Array** stored in the selected **Feature Attribute Matrix**. It is the inverse of [Create Element Array from Feature Array](CopyFeatureArrayToElementArrayFilter.md), which spreads Feature-level values back down onto every member Element.

This filter is useful when a value that is logically a per-Feature property (for example a phase id, a region label, or an imported per-grain measurement) was stored on the Element grid and needs to live in the **Feature Attribute Matrix** so downstream per-Feature filters can consume it.

### How This Filter Works

1. The filter scans the **Feature Ids** array to find the largest Feature Id *N*. Any negative Feature Id is rejected with error *-5570*, since a Feature Id is an index into the Feature Attribute Matrix. The Feature Ids array and the selected Element array must have the same number of tuples (checked before execution, error *-5571*), and the destination Feature **Attribute Matrix** must not be the Attribute Matrix that holds the input arrays (error *-5572*).
2. The selected **Feature Attribute Matrix** — and every **Data Array** inside it — is resized to exactly *N + 1* tuples. The matrix grows or shrinks as needed to match the Feature Ids array. When this shrinks the matrix, warning *-5573* reports that existing Feature arrays are being truncated; when the largest Feature Id exceeds the number of Elements, warning *-5574* flags the suspicious Feature Ids before the allocation is made.
3. For each Element, the Element's tuple is copied into the output array at the index given by that Element's Feature Id. Elements are visited in index order, so if the Elements belonging to one Feature do not all hold the same value, **the value of the last Element visited wins** and the filter emits a single warning (code -1000) identifying the first inconsistent Feature. Two NaN values are considered consistent, so NaN-padded Features do not trigger the warning.
4. A Feature Id that never appears in the Feature Ids array (a "gap" id) receives the value *0* in the output array.

### Notes on Sizing Behavior

- The filter sizes the output for you.
- Because step 2 resizes *every* array in the destination **Attribute Matrix**, selecting a Feature Attribute Matrix whose tuple count is larger than *max(Feature Ids) + 1* will **truncate** the other Feature arrays stored there. Select the Feature Attribute Matrix that was created from the same Feature Ids array you give to this filter.
- Resizing is not done in place: each array in the destination **Attribute Matrix** is reallocated at the new size and its retained data copied over, one array at a time. While an array is being resized both its old and new buffers exist in memory, so peak memory usage is briefly the size of the **Attribute Matrix** plus one additional copy of its largest array.

### Required Input Sources

- **Cell Feature Ids** -- produced by a segmentation filter such as [Segment Features (Scalar)](ScalarSegmentFeaturesFilter.md).
- **Data to Copy to Feature Data** -- any Element-level **Data Array** whose tuple count matches the Feature Ids array. Both arrays must have the same number of tuples.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
