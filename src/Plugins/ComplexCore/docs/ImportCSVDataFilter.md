# Import CSV Data 

## Group (Subgroup) ##

IO (Input)

## Description ##

This **Filter** reads CSV data from any text-based file and imports the data into DREAM3D-NX-style arrays.  The user uses the **Filter's** wizard to specify which file to import, how the data is formatted, what to call each array, and what type each array should be.

*Note:* This **Filter** is intended to read data that is column-oriented, such that each created DREAM3D-NX array corresponds to a column of data in the CSV file. Therefore, this **Filter** will only import scalar arrays. If multiple columns are in fact different components of the same array, then the columns may be imported as separate arrays and then combined in the correct order using the [Combine Attribute Arrays](@ref combineattributearrays) **Filter**.

### Separating the Data ###

After clicking the **Import Data...** button and selecting a file, a wizard appears. The user can choose how the data is delimited: comma (,), tab, semicolon (;) or space ( ). The user may also elect to treat consecutive delimiters as one delimiter.

![Selecting how the data is separated](Images/Import_CSV_1.png)

### Selecting the Delimiter ###

![Selecting the delimiting character(s)](Images/Import_CSV_2.png)

### Selecting Import Rows, Data Representation and Column Headers ###

On the last page, the user can set the following information:

+ Header names, either from a line number in the file or manually through a dialog box or select the generated header values
+ Row index to start the import
+ Column data format (choosing the data type or deciding to skip the column)


![Setting Numerical Representations, Column Headers and other information](Images/Import_CSV_3.png)


If the data does not have any headers, the user can set a string value for each column. These values will be used as the name of the **Data Array** in DREAM3D-NX.

![Setting Names of each Column which will be used as the name of each **Attribute Array** ](Images/Import_CSV_4.png)

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| Wizard Data Object | CSVWizardData | The object that holds all relevant data collected from the wizard |
| Tuple Dimensions | DynamicTableData | The tuple dimensions for the arrays being imported from the file |
| Use Existing Group | bool | Determines whether or not to store the imported data arrays in an existing group |
| Existing Data Group (Use Existing Group - ON) | DataPath | The data path to the existing group where the imported arrays will be stored |
| New Data Group (Use Existing Group - OFF) | DataPath | The data path to the newly created group where the imported arrays will be stored |

## Required Geometry ##

Not Applicable

## Required Objects ##

| Kind | Default Name | Description |
|------|--------------|------|----------------------|-------------|
| **Data Group** | None | The existing data group to store the imported data arrays (only if Existing Data Group is turned ON) |

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| One or more **Element/Feature/Ensemble/etc. Data Arrays** | None | Any | 1 | One or more arrays that are created due to importing CSV data via the wizard |

## Example Pipelines ##


## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists ##

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:


## Python Filter Arguments

+ module: complex
+ Class Name: ImportCSVDataFilter
+ Displayed Name: Import CSV Data

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| created_data_group | New Data Group | Store the imported CSV data arrays in a newly created group. | complex.DataGroupCreationParameter |
| selected_data_group | Existing Data Group | Store the imported CSV data arrays in this existing group. | complex.DataGroupSelectionParameter |
| tuple_dimensions | CSV Tuple Dimensions | The tuple dimensions for the imported CSV data arrays | complex.DynamicTableParameter |
| use_existing_group | Use Existing Group | Store the imported CSV data arrays in an existing group. | complex.BoolParameter |
| wizard_data | CSV Wizard Data | Holds all relevant csv file data collected from the wizard | complex.ImportCSVDataParameter |

