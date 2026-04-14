# Multi-Threshold Objects

## Group (Subgroup)

Processing (Threshold)

## Description

This **Filter** allows the user to input single or multiple criteria for thresholding **Attribute Arrays** in an **Attribute Matrix**. Internally, the algorithm creates the output boolean arrays for each comparison that the user creates.  Comparisons can be either a value and boolean operator (*Less Than*, *Greater Than*, *Equal To*, *Not Equal To*) or a collective set of comparisons. Then all the output arrays are compared with their given comparison operator ( *And* / *Or* ) with the value of a set being the result of its own comparisons calculated from top to bottom.

An example of this **Filter's** use would be after EBSD data is read into DREAM3D-NX and the user wants to have DREAM3D-NX consider **Cells** that the user considers *good*. The user would insert this **Filter** and select the criteria that makes a **Cell** *good*. All arrays **must** come from the same **Attribute Matrix** in order for the **Filter** to execute.

For example, an integer array contains the values 1, 2, 3, 4, 5. For a comparison value of 3 and the comparison operator greater than, the boolean threshold array produced will contain *false*, *false*, *false*, *true*, *true*. For the comparison set { *Greater Than* 2 AND *Less Than* 5} OR *Equals* 1, the boolean threshold array produced will contain *true*, *false*, *true*, *true*, *false*.

It is possible to set custom values for both the TRUE and FALSE values that will be output to the threshold array.  For example, if the user selects an output threshold array type of uint32, then they could set a custom FALSE value of 5 and a custom TRUE value of 20.  So then instead of outputting 0's and 1's to the threshold array, the filter would output 5's and 20's.

**NOTE**: If custom TRUE/FALSE values are chosen, then using the resulting mask array in any other filters that require a mask array will break those other filters.  This is because most other filters that require a mask array make the assumption that the true/false values are 1/0.

## Algorithm

This filter has two algorithm implementations that are automatically selected at runtime based on how the input data is stored. The user does not need to choose between them.

### In-Core Algorithm (Direct)

When all input arrays reside in memory, the **Direct** algorithm is used. For each threshold condition, it reads the input array via per-element access and compares every element against the threshold value. The results are stored in a temporary vector, then merged into the output mask using AND/OR logic.

### Out-of-Core Algorithm (Scanline)

When any input array is backed by chunked on-disk storage (out-of-core), the **Scanline** algorithm is used. Out-of-core data lives in compressed chunks on disk; per-element access would trigger repeated chunk load/decompress/evict cycles ("chunk thrashing").

The Scanline algorithm processes data in fixed-size chunks (64K tuples at a time):

1. Read a chunk of the input array via bulk I/O (copyIntoBuffer)
2. Apply the threshold comparison to produce a chunk-sized result buffer
3. For the first threshold condition, write results directly to the output mask via bulk I/O
4. For subsequent conditions, read the current output chunk, merge using AND/OR logic, and write back

This approach replaces the Direct variant's per-element reads with sequential bulk reads, and reduces temporary memory from O(n) to O(64K) per threshold condition.

### Performance

The in-core Direct algorithm is faster for in-memory data. The out-of-core Scanline algorithm converts random per-element access into sequential bulk I/O and reduces peak memory usage. For a dataset with 100 million tuples, the Scanline variant uses approximately 64 KB of temporary memory per threshold condition instead of approximately 100 MB. Both produce identical output.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
