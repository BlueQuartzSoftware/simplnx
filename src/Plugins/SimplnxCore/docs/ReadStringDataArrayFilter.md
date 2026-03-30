# Read String Data Array

## Group (Subgroup)

Core (IO/Read)

## Description

This **Filter** allows the user to import a plain text file containing the contents of a single Attribute Array. The filter does not care about how many values per line but only about reading the proper number of values from the file.

## Use Cases

There are several use cases for this filter. Both use cases have in common that the data
in the input file should **all** be contained in a single Data Array. If your data file
instead is organized in a table format where each column of data represents a distinct
Data Array then the 'Read CSV Filter' should be used instead.

### Use Case 1

In this use case the user simply wants to read the data into an array within DREAM3D-NX but
does not need to store that data into an AttributeMatrix. In this case the following 
parameters should be set:

- Set Tuple Dimensions should be **ON**
- DataArray Dimensions should also be created. At the minimum the user will need to set the value to the total number of values that will be read.

### Use Case 2

In this use case the user wants to read the data into an existing Attribute Matrix. In this case
the user should set the "Created Arra Path" where the parent object is the target Attribute Matrix and
also set the name of the created Data Array. The following parameters should be set
with the following values:

- Set Tuple Dimensions should be **OFF**
- DataArray Dimensions can be safely ignored as the tuple dimensions from the AttributeMatrix will be used instead.

### Example Data

The example data below has 10 Tuples. The AttributeMatrix would need to have dimensions such that mulplying all the dimensions together yields 10. For example the AttributeMatrix could have X=5, Y=1 and Z=2 or X=10, Y=1 and Z=1. This filter does not require any type of Geometry as the filter is reading data directly into an array.

    companies, cushioned, gabby, sizeable, unintelligible, tonos, approximation, pharmacology, transfection, descending

### Data Format Notes

If the input file has a single string per line then the delimiter does not matter.

### Delimiter

The *Delimiter* parameter provides the following choices:

- **, (comma)**: Values are separated by a comma character.
- **; (semicolon)**: Values are separated by a semicolon character.
- **  (space)**: Values are separated by a space character.
- **: (colon)**: Values are separated by a colon character.
- **\t (Tab)**: Values are separated by a tab character.
- **New Line**: Each value occupies its own line; the newline character acts as the delimiter.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
