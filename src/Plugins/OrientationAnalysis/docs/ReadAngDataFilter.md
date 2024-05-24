# Read EDAX EBSD Data (.ang)

## Group (Subgroup)

IO (Input)

## Description

This **Filter** will read a single .ang file into a new **Image Geometry**, allowing the immediate use of **Filters** on the data instead of having to generate the intermediate .h5ebsd file. A **Cell Attribute Matrix** and Ensemble Attribute Matrix** will also be created to hold the imported EBSD information. Currently, the user has no control over the names of the created **Attribute Arrays**. The user should be aware that simply reading the file then performing operations that are dependent on the proper crystallographic and sample reference frame will be undefined or simply **wrong**. In order to bring the crystal reference frame and sample reference frame into coincidence, rotations will need to be applied to the data.

### Default TSL Transformations

If the data has come from a TSL acquisition system and the settings of the acquisition software were in the default modes, he following reference frame transformations may need to be performed based on the version of the OIM Analysis software being used to collect the data:

+ Sample Reference Frame: 180<sup>o</sup> about the <010> Axis
+ Crystal Reference Frame: 90<sup>o</sup> about the <001> Axis

The user also may want to assign un-indexed pixels to be ignored by flagging them as "bad". The Threshold Objects **Filter** can be used to define this *mask* by thresholding on values such as *Confidence Index* > 0.1 or *Image Quality* > desired quality.

### Note About Sample Grid

OIMAnalysis can create EBSD data sampled on a hexagonal grid. The user can look in the .ang file into the header (those lines starting with the "#" character) for a line that is:

```text
# GRID: HexGrid
```

If the user's .ang files are hexagonal grid files then they will need to run the {ref}`Convert EDAX Hex Grid to Square Grid (.ang)<OrientationAnalysis/ConvertHexGridToSquareGridFilter:Description>` filter to first convert the input files square gridded files.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ INL Export
+ Export Small IN100 ODF Data (StatsGenerator)
+ Edax IPF Colors
+ Confidence Index Histogram

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
