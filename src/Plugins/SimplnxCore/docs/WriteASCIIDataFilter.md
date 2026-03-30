# Write ASCII Data

## Group (Subgroup)

IO (Output) (Write) (Export) (Text) (CSV) (ASCII)

## Description

This filter will write the selected DataArrays to either individual files or as a single CSV style of file.

## String Data Array Caveats

- The "Maximum Tuples per Line" will not have any effect for that specific array.
- If the output is for a single file, then each String value will be enclosed in a set of Single Quotes (') characters.

### Multiple Files

Each input data array will be written to its own output file. The name of the file will be the name of the Data Array + the extension from the parameters.

![Example of multiple output files](Images/Write_Asci_1.png)

### Single File

The output data file will be a column oriented CSV file. The optional header of each column will be the name of the Data Array. If the Data Array has multiple components then the zero based index will also be appended to the data array name. For example Euler Angles have 3 components, the header would look like:

```console
Euler_0,Euler_1,Euler_2
```

![Example of single output file](Images/Write_Asci_2.png)

### Delimiter

The *Delimiter* parameter selects the character used to separate values within each row:

- **Space**: Values are separated by a single space character.
- **Semicolon**: Values are separated by a semicolon (`;`).
- **Comma**: Values are separated by a comma (`,`). This is the standard CSV delimiter.
- **Colon**: Values are separated by a colon (`:`).
- **Tab**: Values are separated by a tab character.

### Header and Index Options

The *Header and Index Options* parameter controls whether column headers and/or a row index are written to the output file:

- **Neither**: No headers or index columns are written; only data values are output.
- **Headers**: Column headers (array names) are written as the first row of the file.
- **Index**: A zero-based row index column is prepended to each data row.
- **Both**: Both column headers (first row) and a row index column are included.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
