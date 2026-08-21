# Write StatsGenerator ODF Angle File

## Group (Subgroup)

IO (Output)

## Description

This **Filter** is used in a workflow where the user would like to generate a synthetic microstructure with an **Orientation Distribution Function (ODF)** that matches (as closely as possible) an existing experimental data set or other data set that is being mimicked. The ODF describes how crystallographic orientations are statistically distributed in the material. The basic workflow is the following:

1. Import Euler angle data (e.g., ANG or CTF files)
2. Optionally threshold the data so each cell is marked as allowable or not-allowable
3. Insert the "Export StatsGenerator ODF Angle File" **Filter** in the **Pipeline**
4. Run the **Pipeline** to produce the file
5. Launch **StatsGenerator**
6. Generate the data
7. Click on the ODF Tab
8. Click on the *Bulk Load* sub tab
9. Select the file that was just written
10. Load the data and inspect the ODF that was generated

## Important Change from Earlier Versions of StatsGenerator

StatsGenerator cannot load data from standard .ang or .ctf files. If you want to get the ODF from an existing experimental data set and you have one of those files then you must use the functionality of this filter.

## Notes on Implementation

+ A separate file is written for each phase
+ Spaces are the default as the delimiters between values. The user can select another value
+ Default values of 1.0 are used for both the *weight* and *sigma*. Both values are dimensionless. **If the user needs a stronger texture due to a low number of angles then larger values should be used such as 10, 100 or even 1000.**
+ The user has the option to convert the supplied Euler angles to degrees. **StatsGenerator** is able to import Euler angles as either degrees or radians based on user input, so the output type from this **Filter** could remain as radians or be converted to degrees. The user should remain cognizant of what representation their angles are in so that the correct option is chosen during the import process in **StatsGenerator**

## Example File

The file written is a simple text file that contains a short comment section and a single *Header* line of data. All comment lines should come **BEFORE** the actual header line. There is a single header line in the form of "Key:Value" and then the lines of data.

    # All lines starting with '#' are comments and should come before the header.
    # DREAM3D-NX StatsGenerator Angles Input File
    # DREAM3D-NX Version 6.1.107.0d8bad9
    # Angle Data is space delimited.
    # Euler0 Euler1 Euler2 Weight Sigma
    Angle Count:100
    0 0 0 1 1
    3.6 1.8 3.6 1 1
    7.2 3.6 7.2 1 1
    10.8 5.4 10.8 1 1
    14.4 7.2 14.4 1 1

The **only** required header line is:

    Angle Count:100

There are 5 columns of data which are the 3 Euler Angles, the Weight Value and the Sigma Value.

### Delimiter

The *Delimiter* parameter provides the following choices:

- **, (comma) [0]**: Uses a comma as the column separator.
- **; (semicolon) [1]**: Uses a semicolon as the column separator.
- **(space) [2]**: Uses a space as the column separator. This is the default value.
- **: (colon) [3]**: Uses a colon as the column separator.
- **\t (tab) [4]**: Uses a tab character as the column separator.

## Required Input Sources

- **Euler Angles** -- typically read from EBSD data via [Read H5EBSD File](ReadH5EbsdFilter.md), [Read EDAX EBSD Data (.ang)](ReadAngDataFilter.md), or [Read Oxford Instr. EBSD Data (.ctf)](ReadCtfDataFilter.md). Supplied in radians (Bunge Z-X-Z convention); use the *Convert to Degrees* option only if **StatsGenerator** will import them as degrees.
- **Phases** -- read alongside the Euler angles from the same EBSD reader. A separate output file is written for each phase.
- **Mask** (optional) -- typically produced by [Multi-Threshold Objects](../SimplnxCore/MultiThresholdObjectsFilter.md). Only required when *Only Write Good Elements* is checked.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ Export Small IN100 ODF Data (StatsGenerator)

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
