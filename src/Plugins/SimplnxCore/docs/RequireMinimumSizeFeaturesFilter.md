# Remove Minimum Size Features

## Group (Subgroup)

Processing (Cleanup)

## Description

This **Filter** removes **Features** that have a total number of **Cells** below the minimum threshold defined by the user. Entering a number larger than the largest **Feature** generates an *error* (since all **Features** would be removed). Hence, a choice of threshold should be carefully chosen if it is not known how many **Cells** are in the largest **Features**. After removing all the small **Features**, the remaining **Features** are isotropically coarsened to fill the gaps left by the small **Features**.

The **Filter** can be run in a mode where the minimum number of neighbors is applied to a single **Ensemble**.  The user can select to apply the minimum to one specific **Ensemble**.

## Algorithm

### What the filter does (conceptually)

The input is a segmented volume: every **Cell** has a **Feature ID** identifying which **Feature** (grain, particle, phase domain, etc.) it belongs to. Small features — often from noise, bad data, or over-segmentation — are frequently not physically meaningful. This filter removes any feature whose voxel count is below a user-specified threshold and then reassigns those voxels to their neighbors so the volume remains fully labeled.

Running the filter leaves the volume without any sub-threshold features and with no "holes" (no voxels left unlabeled), ready for downstream analysis. Any previously-computed **feature-level** data (centroids, average orientations, neighbor lists) is invalidated by the cleanup and must be recomputed.

### Phase 1 — Mark small features as removed

The filter first walks the **per-feature voxel count** array (an array indexed by Feature ID, typically of length "thousands") and marks any feature with a count below the threshold as inactive. If "Apply Single Phase" is enabled, only features belonging to the selected phase are considered.

Then it scans the full per-cell **Feature IDs** volume in 64K-tuple chunks:

1. Bulk-read one chunk of Feature IDs into RAM (`copyIntoBuffer`).
2. For each voxel in the chunk, if its feature was marked inactive, overwrite the voxel's Feature ID with `-1` (the "bad voxel" sentinel).
3. If the chunk contained any modifications, write the chunk back (`copyFromBuffer`); otherwise skip the write.

This replaces a naive per-voxel `setValue(-1)` loop, which would issue one HDF5 chunk-op per voxel on OOC-backed data. Only chunks that actually changed incur a write.

If **every** feature would be removed by the threshold, the filter errors out — this is almost always a sign the user chose the threshold incorrectly.

### Phase 2 — Fill the gaps by majority neighbor vote

After phase 1, every voxel whose feature was too small is sitting at Feature ID `-1`. The filter iteratively relabels these bad voxels by **majority vote among their 6 face-neighbors** (±X, ±Y, ±Z). The voting is done across a series of passes:

1. **Scan pass** — walk the volume in Z-major order using a **rolling 3-slice buffer** (previous, current, and next Z-slices of Feature IDs loaded in RAM). For each bad voxel, tally the Feature IDs of its 6 face-neighbors into a small vote counter indexed by Feature ID. The neighbor whose ID received the most votes is chosen.
2. Because every neighbor read comes from the 3-slice RAM buffer (not the OOC store), the expensive "chunk thrashing" access pattern is eliminated. At the end of each Z-slice the bottom slice is evicted, a new top slice is loaded, and the buffers slide forward.
3. Bad voxels that picked a winner (some bad voxels have no valid neighbor, e.g. when they're surrounded by other bad voxels) are recorded into a **sparse parallel list**: one vector of changed voxel global indices, one vector of chosen neighbor global indices. The list grows with the number of bad voxels processed **per iteration**, not with the volume size.
4. **Transfer phase** — the chosen Feature IDs are written back into the Feature IDs volume, and every other cell-level array (the full set of arrays in the **Cell Attribute Matrix**) has the neighbor's tuple copied into the bad voxel's tuple. This runs one `ChunkedTransferWorker` per cell-level array in parallel via `ParallelTaskAlgorithm`. Each worker:
   - Allocates a slab buffer sized to its per-array memory budget (64 MB by default) and walks the sorted `changedVoxels` list one Z-batch at a time.
   - Reads the batch's Z-range **plus a ±1 Z-slice margin** into the slab — the margin guarantees every neighbor index falls inside the loaded slab, even when the neighbor is one Z-slice off from the voxel.
   - Applies every `(voxel ← neighbor)` copy entirely in-memory on the slab.
   - Writes back **only the interior** (the margin is never mutated).

   Because each worker owns its own DataArray, the outer parallelism across arrays is safe even though an individual DataStore is not internally thread-safe.
5. **Iteration** — phase 2 repeats (scan + transfer) until no bad voxels remain. Each iteration shrinks the bad-voxel set.

### Why the rolling buffer matters

Without it, each bad-voxel evaluation would read its 6 neighbors via per-element `getValue()` on the OOC store. For a 2 billion-voxel volume with tens of millions of bad voxels per iteration across several iterations, that is a nine-digit count of chunk load/evict cycles. With the 3-slice buffer, the total chunk-load count drops to roughly the Z dimension per iteration — typically a four-digit count — and all neighbor reads hit RAM.

### Memory footprint

Peak working memory is bounded by:

- A 3-slice Feature IDs buffer during the scan: `3 * Dx * Dy * sizeof(int32)`.
- Two sparse vectors sized to the per-iteration bad-voxel count (not the full volume).
- A per-array transfer slab capped at 64 MB per parallel worker.

All three terms are O(slice) or O(iteration bad count), never O(volume). The filter handles billion-voxel volumes without OOM.

## WARNING: Feature Data Will Become Invalid

By modifying the cell level data, any feature data that was previously computed will most likely be invalid at this point. Filters that compute feature level data should be rerun to ensure accurate final results from your pipeline.

## WARNING: NeighborList Removal

If the Cell Feature AttributeMatrix contains any *NeighborList* data arrays, those arrays will be **REMOVED** because those lists are now invalid. Re-run the *Find Neighbors* filter to re-create the lists.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (02) Small IN100 Full Reconstruction


## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
