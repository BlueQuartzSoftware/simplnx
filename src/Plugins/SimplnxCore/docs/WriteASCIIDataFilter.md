# Write ASCII Data

## Group (Subgroup)

IO (Output)

## Description

This filter will write the selected **Data Array**s to either individual files or as a single CSV style of file. The inputs are any existing **Data Array**s (numeric or string) that the user selects to export.

### Maximum Tuples Per Line

When writing to multiple files, the *Maximum Tuples Per Line* parameter controls how many tuples are printed on each row of an output file (units: tuples per row). For example, with a value of *1* each tuple is written on its own line; with a value of *10* up to ten tuples are written per line before a new line begins. This makes it possible to reshape long single-column output into a more compact block. The parameter does not apply to string arrays.

## String Data Array Caveats

- The "Maximum Tuples Per Line" will not have any effect for that specific array.
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

- **Space [0]**: Values are separated by a single space character.
- **Semicolon [1]**: Values are separated by a semicolon (`;`).
- **Comma [2]**: Values are separated by a comma (`,`). This is the standard CSV delimiter.
- **Colon [3]**: Values are separated by a colon (`:`).
- **Tab [4]**: Values are separated by a tab character.

### Header and Index Options

The *Header and Index Options* parameter controls whether column headers and/or a row index are written to the output file:

- **Neither [0]**: No headers or index columns are written; only data values are output.
- **Headers [1]**: Column headers (array names) are written as the first row of the file.
- **Index [2]**: A zero-based row index column is prepended to each data row.
- **Both [3]**: Both column headers (first row) and a row index column are included.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
