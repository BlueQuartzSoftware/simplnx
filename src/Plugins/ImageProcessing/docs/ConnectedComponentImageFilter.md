# Connected Component Image Filter

Label the objects in a binary image.

## Group (Subgroup)

ImageProcessing (Segmentation)

## Description

Labels the objects in the input image: nonzero input voxels are foreground, and zero-valued voxels are background. Below the provisional ID limit, each distinct foreground object gets a unique label. Objects reached earlier by a raster-order scan (X fastest, then Y, then Z) get a lower label. Labels start at 1 and are consecutive; background is 0.

The input array must be single-component (scalar) and may use **any integer type** (signed or unsigned). The filter tests each native input value only as zero or nonzero. A large uint64 value is valid foreground. Output is always **uint32**, with 4 bytes per value; input values are not converted to output label values. Below the provisional ID limit, the filter preserves the legacy ITK label values.

### Fully Connected

Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. For objects that are 1 pixel wide, use FullyConnectedOn.

### Provisional Label Limit Handling

A **provisional run ID** identifies one uninterrupted foreground run along X before connected runs are joined. Several provisional runs can form one final component. For example, stripes joined by a bridge can use many run IDs but produce one component. The limit therefore applies to provisional runs, not to the voxel count or the final component count.

- **Error (default):** execution accepts provisional ID 4,294,967,295 and stops before creating ID 4,294,967,296. The check occurs when a new run needs an ID, so a large image can fail after a long first pass. New pipelines and existing pipelines without an explicit choice use Error.
- **Warn and Continue (Legacy Behavior):** execution sends one warning immediately when it first exceeds the limit. Normal completion and later processing or I/O errors retain that warning. The application's general cancellation handler can replace a cancelled result and its retained diagnostics; the immediate warning has already been sent. This choice keeps the old uint32 wrap behavior: ID 4,294,967,296 becomes background zero, the next ID reuses label 1, and later IDs reuse earlier labels. Foreground can disappear and distinct components can merge, including components already joined to other runs. Segmentation, feature counts, component sizes, and downstream per-feature statistics can be incorrect. The scanline route can join zero-labelled foreground runs to other runs; the tiled route skips zero labels in its previous-row neighbor checks. Different routes can therefore give different incorrect results after overflow.

To run an old pipeline with intentional overflow compatibility, explicitly select *Warn and Continue (Legacy Behavior)*. Use this choice only if you accept the scientific consequences. It does not add capacity, preserve valid label identities after overflow, or guarantee completion. Correct support for more provisional IDs requires a separate algorithm or internal representation change. Shared component-counting and composite filters retain Error; they do not inherit this compatibility choice.

Both choices retain independent allocation, addressability, I/O, and cancellation handling. Memory or I/O failures can still stop the filter. The policy does not add an all-or-nothing guarantee for an unrelated late output-write failure.

## Algorithm

The filter scans the image in X-fastest order. It encodes each foreground run along X. A union-find table joins runs that have the selected connectivity.

After the first pass, the filter maps each provisional label to a consecutive label. The first output label is one. Each table entry represents a run, not a voxel.

For OOC 3D data, the filter first requests memory for the complete working state. A complete grant uses one input read and one output write.

If this request is not complete, the filter requests memory for the uint32 provisional-label volume. A complete second grant keeps that volume in memory.

If the second grant is not complete, the filter uses bounded replay. Pass 1 reads each input plane once. It encodes independent rows in parallel and keeps only adjacent-plane run data.

After union resolution, pass 2 reads adjacent plane groups. It counts and maps independent rows in parallel. The group size uses at most one-twentieth of the complete replay working set, with a one-plane minimum. The shared cache budget can reduce this size.

The bounded 3D route uses two full input reads and one full output write. Grouping changes the number of transfers, not the logical I/O. The route does not store a complete provisional-label volume.

For true 2D data (`Z=1`), a checked scheduler uses at most 64 MiB for blocks. An extremely wide row uses X tiles.

The tiled first pass can use temporary records for previous-row labels. The final pass rereads bounded input tiles and writes bounded output tiles.

Below the provisional ID limit, all routes create labels at the same raster run starts and preserve the same union and consecutive-label order. Explicit legacy overflow retains each route's existing zero/reuse behavior.

The equivalence tables remain resident memory proportional to the number of provisional runs, even when image transfers use bounded buffers or disk storage. Three uint32 tables with 2^32 entries alone need at least 48 GiB, before line records, image buffers, and other state. Allocation can fail before the ID limit is reached. This policy does not make the union-find tables bounded in size.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
