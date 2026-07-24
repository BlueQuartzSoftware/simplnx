# Create Element Array from Feature Array

## Group (Subgroup)

Core (Memory/Management)

## Description

This **Filter** copies the values associated with a **Feature** to all the **Elements** that belong to that **Feature**. For every **Element** `i`, the created array's tuple is the selected **Feature** array's tuple at index `FeatureIds[i]`. Xdmf visualization files write only the **Element** attributes, so if the user wants to display a spatial map of a **Feature** level attribute, this **Filter** will transfer that information down to the **Element** level.

Multiple **Feature** arrays may be selected and copied in a single filter instance; all selected arrays must have the same number of tuples. Each created **Element** array is placed next to the selected *Cell Feature Ids* array and is named by appending the *Created Array Suffix* to the source array's name (e.g., `AvgTemp` with suffix `_Cell` creates `AvgTemp_Cell`). The created arrays keep the source array's data type and component layout.

Note that each created array is **Element**-sized (number of **Elements** × components × bytes per value), so selecting many **Feature** arrays multiplies the additional memory required.

### Input Validation

- All selected **Feature** arrays must have the same number of tuples (error -3020).
- The *Created Array Suffix* must not contain a `/` character (error -3021).
- Every value in the *Cell Feature Ids* array must be non-negative. A negative value stops the filter with an error (-5355).
- The largest **Feature** Id must be a valid tuple index into each selected **Feature** array; otherwise the filter stops with an error (-5351).
- A **Feature** array with *more* tuples than the largest **Feature** Id requires is accepted — the extra tuples are simply never read. (Legacy DREAM3D 6.5 rejected this case.)

% Auto generated parameter table will be inserted here

## Example Pipelines

- (02) Image Segmentation (ITKImageProcessing)
- (04) Porosity Analysis (ITKImageProcessing)

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
