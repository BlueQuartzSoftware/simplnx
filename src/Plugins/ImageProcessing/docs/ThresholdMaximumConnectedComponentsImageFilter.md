# Threshold Maximum Connected Components Image Filter

## Group (Subgroup)

ImageProcessing (Segmentation)

## Description

This filter selects a lower intensity threshold with a **component-count heuristic**: it compares the numbers of connected regions at two candidate thresholds and narrows the interval. It does not guarantee a global maximum. The input is a scalar cell array in the selected image geometry. Integer and floating inputs produce the same fixed **uint8** output type, with one byte per pixel or voxel. Output values are **Inside Value** and **Outside Value**; the threshold itself is not saved as an image.

A pixel is inside when its native intensity is at or above the selected lower threshold and at or below **Upper Boundary**. Both bounds are inclusive. The search uses **face connectivity**, which joins pixels through shared edges in 2-D or shared faces in 3-D. Diagonal contact alone does not join regions.

### Search score and stopping units

The score counts connected components of the would-be **nonzero output**, including components whose size is exactly **Minimum Object Size In Pixels**. This parameter is a pixel/voxel count and defaults to *0*. It changes the search score; it does not remove small objects from the final output.

**Inside Value** defaults to *1* and **Outside Value** defaults to *0*. Both are dimensionless uint8 values. With these defaults, the score counts in-band regions. Inside0/Outside1 scores the complement. If both values are nonzero, all pixels participate as foreground. If both are zero, none participate. A nonzero Outside Value can connect regions that would otherwise be separate.

The search stops when the interval is at most **2 input-intensity units** wide. This is an absolute rule for integers and floating inputs. It is not a relative tolerance or a measure of segmentation accuracy. For example, normalized values spanning 0 to 1 skip the search. Binary input `[0,1,0,1]` selects threshold0 and gives `[1,1,1,1]`, although another threshold could give more regions. Scaling the intensities can change the selected mask. The right candidate wins only if its score is strictly greater; ties select the left candidate.

For floating input, native rounding can repeat the complete search state while the interval is still wider than 2. The filter stops, retains the candidate selected by the normal comparison, and issues one precision warning. It does not choose a different endpoint or perform an exhaustive search.

### Upper Boundary

Upper Boundary is a Float64 parameter in input-intensity units. Its default is *65536*. Integer input truncates an in-range fractional request toward zero. Thus an unsigned request of *-0.5* becomes *0*; *-1* requests an empty native band. A signed request that truncates to the native minimum remains valid. In-range Float64-to-float32 conversion retains ordinary native rounding.

A finite request above the native maximum, or positive infinity, selects the exact native maximum before conversion. For uint8, the default therefore becomes *255*. int64/uint64 limits are checked before casting; a rounded Float64 value at 2^63 or 2^64 is never cast into the corresponding integer. Float64 cannot express every exact 64-bit integer cutoff above 2^53, even though image bounds and integer search candidates remain exact native integers.

NaN Upper Boundary is an invalid parameter and always causes a clear error, including in the image Warn mode. A finite request below the native range after integer truncation, a cap below the finite image minimum, or negative infinity produces an empty band. The filter skips search, fills Outside Value and issues an empty-band warning. It does not clamp a low cutoff upward or treat this output as an optimized threshold. Structurally valid zero-length input remains empty and is distinct from a nonempty all-nonfinite image.

### Nonfinite Input Handling

This single choice applies to image NaN, positive infinity and negative infinity:

- *Error* is the default. The initial image scan stops with an error before component search or classification writes. An empty requested band does not bypass this scan. Output allocation can already have occurred through normal preflight actions.
- *Warn and Continue* uses only finite image values to establish native search bounds. Ordinary inclusive comparisons assign every nonfinite pixel Outside Value. One aggregate warning reports separate NaN and infinity counts. A positive-infinity parameter cap selects a finite native maximum; it does not include positive-infinity image pixels.
- If a nonempty Warn input has no finite pixels, the filter skips all threshold/component searches and fills Outside Value. One aggregate warning states the counts and that no threshold was calculated. This is a defined fallback, not a fitted segmentation. No additional UI choice is required.

Outside0 gives no nonzero foreground in an all-Outside fallback. A nonzero Outside Value can make the entire image foreground downstream, or bridge finite regions in a mixed image. These changes affect object counts, fractions, boundaries and measurements. Later filters that require objects can fail on an empty mask or report different results on a whole-image mask. Warn does not waive cancellation, I/O, memory, addressability or strict component-capacity failures.

### Numeric migration

The starting midpoint now includes the image minimum, and interval arithmetic avoids native integer overflow. This corrects offset, negative and wide-range cases. For input `[100,102]`, a nonbinding cap and Inside1/Outside0, the old starting threshold1 gave mask `[1,1]`; the corrected midpoint101 gives `[0,1]`. The output type is unchanged, but segmentation and downstream measurements can change. Constant images use their actual constant as the threshold. Old pipelines that omit the new nonfinite choice use Error and can now stop on questionable data that previously completed.

The finite float32 `RA-Float.nrrd` regression fixture demonstrates this migration on a real image. Its range is -1146 to 32767. With default controls, the corrected initial midpoint changes from 16956.5 to 15810.5, and the final threshold changes from 24164.05859375 to 24166.107421875. Exactly 159 pixels change: foreground decreases from 99487 to 99328 pixels, and face-connected components decrease from 3715 to 3712. An independent six-neighbor flood fill confirms these counts; correcting only the initial seed reproduces the new mask. The retained heuristic can select fewer components after the arithmetic correction. Neither the count nor this regression is a segmentation-accuracy score or a global-maximum guarantee.

## Algorithm

One initial scan collects native finite extrema and bounded nonfinite summaries. Existing resident spans are reduced directly; other stores are read serially into bounded owned buffers, whose independent blocks can be reduced in parallel. The filter does not create a full-image finite mask or convert the integer image to Float64. Initial, left and right integer midpoint calculations retain native 64-bit distinctions and their left/right rounding rules.

Each iteration performs two connected-component scans and compares their scores. 3-D counting reads budget-derived bands with a one-plane fallback and retains current/previous run planes. True 2-D counting uses bounded row blocks; very wide rows use bounded X tiles and provisional scratch, using disk storage for out-of-core input. These are shared traversal routes for resident and disk-backed arrays, with serial store I/O and stable owned compute buffers. Final classification and all-Outside fills use bounded writes.

Union-find and size tables still grow with the number of provisional foreground runs. Bounded staging is not a bound on total RAM. A provisional run ID beyond uint32 capacity, an allocation/addressability failure, or an I/O error stops execution; the image Warn choice does not permit label wrap or incorrect counts. Search changes can change the number of full-image count passes and therefore CPU time and disk I/O. No fixed runtime improvement or legacy byte-parity guarantee is implied.

% Auto generated parameter table will be inserted here

## References

1) Urish KL, August J, Huard J. "Unsupervised segmentation for myofiber counting in immunofluorescent microscopy images". Insight Journal. ISC/NA-MIC/MICCAI Workshop on Open-Source Software (2005) https://insight-journal.org/browse/publication/40
2) Pikaz A, Averbuch, A. "Digital image thresholding based on topological stable-state". Pattern Recognition, 29(5): 829-843, 1996.

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
