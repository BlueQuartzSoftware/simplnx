import complex as cx
import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

# Create the DataStructure object
data_structure = cx.DataStructure()

csv_importer_data = cx.CSVImporterData()
csv_importer_data.input_file_path = "wrapping/python/examples/test_csv_data.csv"
csv_importer_data.start_import_row = 2

csv_importer_data.comma_as_delimiter = True
csv_importer_data.semicolon_as_delimiter = False
csv_importer_data.space_as_delimiter = False
csv_importer_data.tab_as_delimiter = False
csv_importer_data.consecutive_delimiters = False

csv_importer_data.custom_headers = []
csv_importer_data.data_types = [cx.DataType.float32,cx.DataType.float32,cx.DataType.float32,cx.DataType.float32,cx.DataType.float32,cx.DataType.float32,cx.DataType.int32 ]
csv_importer_data.skipped_array_mask = [False,False,False,False,False,False,False ]
csv_importer_data.tuple_dims = [37989]

csv_importer_data.headers_line = 1
csv_importer_data.header_mode = cx.CSVImporterData.HeaderMode.Line

# This will store the imported arrays into a newly generated DataGroup
result = cx.ImportCSVDataFilter.execute(data_structure=data_structure, 
                                      # This will store the imported arrays into a newly generated DataGroup
                                      created_data_group=cx.DataPath(["Imported Data"]),  
                                      # We are not using this parameter but it still needs a value
                                      selected_data_group=cx.DataPath(),  
                                      # Use an existing DataGroup or AttributeMatrix. If an AttributemMatrix is used, the total number of tuples must match
                                      use_existing_group=False,   
                                      # The CSVImporterData object with all member variables set.
                                      csv_importer_data=csv_importer_data # The CSVImporterData object with all member variables set.
                                      )
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the ImportCSVDataFilter filter")
