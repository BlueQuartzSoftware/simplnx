import complex as cx
import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

# Create the DataStructure object
data_structure = cx.DataStructure()
# This file has 7 columns to import
read_csv_data = cx.ReadCSVDataParameter()
read_csv_data.input_file_path = "wrapping/python/examples/test_csv_data.csv"
read_csv_data.start_import_row = 2
read_csv_data.delimiters = [',']
read_csv_data.custom_headers = []
read_csv_data.column_data_types = [cx.DataType.float32,cx.DataType.float32,cx.DataType.float32,cx.DataType.float32,cx.DataType.float32,cx.DataType.float32,cx.DataType.int32 ]
read_csv_data.skipped_array_mask = [False,False,False,False,False,False,False ]
read_csv_data.tuple_dims = [37989]
read_csv_data.headers_line = 1
read_csv_data.header_mode = cx.ReadCSVDataParameter.HeaderMode.Line

# This will store the imported arrays into a newly generated DataGroup
result = cx.ReadCSVFileFilter.execute(data_structure=data_structure, 
                                      # This will store the imported arrays into a newly generated DataGroup
                                      created_data_group=cx.DataPath(["Imported Data"]),  
                                      # We are not using this parameter but it still needs a value
                                      selected_data_group=cx.DataPath(),  
                                      # Use an existing DataGroup or AttributeMatrix. If an AttributemMatrix is used, the total number of tuples must match
                                      use_existing_group=False,   
                                      # The ReadCSVData object with all member variables set.
                                      read_csv_data=read_csv_data # The ReadCSVData object with all member variables set.
                                      )
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the ReadCSVFileFilter filter")
