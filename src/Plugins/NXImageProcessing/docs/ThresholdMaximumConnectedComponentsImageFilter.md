# Threshold Maximum Connected Components Image Filter

## Group (Subgroup)

NXImageProcessing (Segmentation)

## Description

This filter selects a lower intensity threshold with a **component-count heuristic**: it compares the numbers of connected regions at two candidate thresholds and narrows the interval. It does not guarantee a global maximum. The input is a scalar cell array in the selected image geometry. Integer and floating inputs produce the same fixed **uint8** output type, with one byte per pixel or voxel. Output values are **Inside Value** and **Outside Value**; the threshold itself is not saved as an image.

A pixel is inside when its native intensity is at or above the selected lower threshold and at or below **Upper Boundary**. Both bounds are inclusive. The search uses **face connectivity**, which joins pixels through shared edges in 2-D or shared faces in 3-D. Diagonal contact alone does not join regions.

### Search score and stopping units

The score counts connected components of the would-be **nonzero output**, including components whose size is exactly **Minimum Object Size In Pixels**. This parameter counts pixels or voxels and defaults to *0*. Image spacing does not change the count or the two-intensity-unit stopping rule. Minimum Object Size In Pixels changes the search score. Small objects can remain in the final output.

**Inside Value** defaults to *1* and **Outside Value** defaults to *0*. Both are dimensionless uint8 values. With these defaults, the score counts in-band regions. Inside 0/Outside 1 scores the complement. If both values are nonzero, all pixels participate as foreground. If both are zero, none participate. A nonzero Outside Value can connect regions that would otherwise be separate.

The search stops when the interval is at most **2 input-intensity units** wide. This is an absolute rule for integers and floating inputs. It is not a relative tolerance or a measure of segmentation accuracy. For example, normalized values spanning 0 to 1 skip the search. For the values `[0,1,0,1]` at the default Inside and Outside settings, the integer midpoint rounds toward the lower endpoint to 0 and produces `[1,1,1,1]`. Floating-point input has midpoint 0.5 and produces `[0,1,0,1]`. In both cases the search stops before scoring alternate candidates. Scaling the intensities can change the selected mask. The right candidate wins only if its score is strictly greater; ties select the left candidate.

For floating input, native rounding can repeat the complete search state while the interval is still wider than 2. The filter stops, retains the candidate selected by the normal comparison, and issues one precision warning. It does not choose a different endpoint or perform an exhaustive search.

### Upper Boundary

Upper Boundary is a Float64 parameter in input-intensity units. Its default is *65536*. Integer input truncates an in-range fractional request toward zero. Thus an unsigned request of *-0.5* becomes *0*; *-1* requests an empty native band. A signed request that truncates to the native minimum remains valid. In-range Float64-to-float32 conversion retains ordinary native rounding.

A finite request above the native maximum, or positive infinity, selects the exact native maximum before conversion. For uint8, the default therefore becomes *255*. int64/uint64 limits are checked before casting; a rounded Float64 value at 2^63 or 2^64 is never cast into the corresponding integer. Float64 cannot express every exact 64-bit integer cutoff above 2^53, even though image bounds and integer search candidates remain exact native integers.

NaN is an invalid Upper Boundary. Pipeline JSON stores finite cutoffs as numbers and signed infinities as the strings `"+Infinity"` and `"-Infinity"`. An older file with a null cutoff has lost its original value. Enter the intended cutoff before loading that pipeline again. A finite request below the native range after integer truncation, a cap below the finite image minimum, or negative infinity produces an empty band. The filter skips search, fills Outside Value and issues an empty-band warning. It does not clamp a low cutoff upward or treat this output as an optimized threshold. Structurally valid zero-length input remains empty and is distinct from a nonempty all-nonfinite image.

### Input with NaN or infinity

Finite pixels set the native search bounds. A NaN or infinity lies outside the finite band and receives **Outside Value**. If there are no finite pixels, every output pixel receives **Outside Value**. A nonzero Outside Value can connect regions or make the entire image foreground. Clean invalid intensities upstream before relying on object counts, fractions or boundaries.

### Numeric migration

The starting midpoint now includes the image minimum, and interval arithmetic avoids native integer overflow. This corrects offset, negative and wide-range cases. For input `[100,102]`, a nonbinding cap and Inside 1/Outside 0, the old starting threshold 1 gave mask `[1,1]`; the corrected midpoint 101 gives `[0,1]`. The output type is unchanged, but segmentation and downstream measurements can change. Constant images use their actual constant as the threshold.

## Algorithm

One initial scan collects the native minimum and maximum of the finite pixels. Existing resident spans are reduced directly; other stores are read serially into bounded owned buffers, whose independent blocks can be reduced in parallel. The filter does not create a full-image finite mask or convert the integer image to Float64. Initial, left and right integer midpoint calculations retain native 64-bit distinctions and their left/right rounding rules.

Each iteration performs two connected-component scans and compares their scores. 3-D counting reads budget-derived bands with a one-plane fallback and retains current/previous run planes. True 2-D counting uses bounded row blocks; very wide rows use bounded X tiles and provisional scratch, using disk storage for out-of-core input. These are shared traversal routes for resident and disk-backed arrays, with serial store I/O and stable owned compute buffers. Final classification and all-Outside fills use bounded writes.

Union-find and size tables still grow with the number of provisional foreground runs. Bounded staging is not a bound on total RAM. A provisional run ID beyond uint32 capacity, an allocation/addressability failure, or an I/O error stops execution. Search changes can change the number of full-image count passes and therefore CPU time and disk I/O. No fixed runtime improvement or legacy byte-parity guarantee is implied.

% Auto generated parameter table will be inserted here

## References

1) Urish KL, August J, Huard J. "Unsupervised segmentation for myofiber counting in immunofluorescent microscopy images". Insight Journal. ISC/NA-MIC/MICCAI Workshop on Open-Source Software (2005) https://insight-journal.org/browse/publication/40
2) Pikaz A, Averbuch, A. "Digital image thresholding based on topological stable-state". Pattern Recognition, 29(5): 829-843, 1996.

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
