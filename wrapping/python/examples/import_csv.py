import complex as cx
import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

# Create the DataStructure object
data_structure = cx.DataStructure()

import_csv_wizard_data = cx.CSVWizardData()
import_csv_wizard_data.input_file_path = "test_csv_data.csv"
import_csv_wizard_data.begin_index = 2

import_csv_wizard_data.comma_as_delimiter = True
import_csv_wizard_data.semicolon_as_delimiter = False
import_csv_wizard_data.space_as_delimiter = False
import_csv_wizard_data.tab_as_delimiter = False
import_csv_wizard_data.consecutive_delimiters = False

import_csv_wizard_data.data_headers = []
import_csv_wizard_data.data_types = [cx.DataType.float32,cx.DataType.float32,cx.DataType.float32,cx.DataType.float32,cx.DataType.float32,cx.DataType.float32,cx.DataType.int32 ]
import_csv_wizard_data.delimiters = [","]

import_csv_wizard_data.header_line = 1
import_csv_wizard_data.header_mode = cx.CSVWizardData.HeaderMode.Line

import_csv_wizard_data.number_of_lines = 37990


# This will store the imported arrays into a newly generated DataGroup
result = cx.ImportCSVDataFilter.execute(data_structure=data_structure, 
                                      # This will store the imported arrays into a newly generated DataGroup
                                      created_data_group=cx.DataPath(["Imported Data"]),  
                                      # We are not using this parameter but it still needs a value
                                      selected_data_group=cx.DataPath(),  
                                      # The dimensions of the tuples. Can be 1-N dimensions
                                      tuple_dimensions=[[37989]], 
                                      # Use an existing DataGroup or AttributeMatrix. If an AttributemMatrix is used, the total number of tuples must match
                                      use_existing_group=False,   
                                      # The CSVWizardData object with all member variables set.
                                      wizard_data=import_csv_wizard_data # The CSVWizardData object with all member variables set.
                                      )
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the ImportCSVDataFilter filter")
