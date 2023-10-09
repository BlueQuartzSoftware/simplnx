import complex as cx
import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

# Create the DataStructure object
data_structure = cx.DataStructure()

text_importer_data = cx.TextImporterData()
text_importer_data.input_file_path = "wrapping/python/examples/test_csv_data.csv"
text_importer_data.start_import_row = 2

text_importer_data.comma_as_delimiter = True
text_importer_data.semicolon_as_delimiter = False
text_importer_data.space_as_delimiter = False
text_importer_data.tab_as_delimiter = False
text_importer_data.consecutive_delimiters = False

text_importer_data.custom_headers = []
text_importer_data.data_types = [cx.DataType.float32,cx.DataType.float32,cx.DataType.float32,cx.DataType.float32,cx.DataType.float32,cx.DataType.float32,cx.DataType.int32 ]
text_importer_data.skipped_array_mask = [False,False,False,False,False,False,False ]
text_importer_data.tuple_dims = [37989]

text_importer_data.headers_line = 1
text_importer_data.header_mode = cx.TextImporterData.HeaderMode.Line

# This will store the imported arrays into a newly generated DataGroup
result = cx.ImportTextDataFilter.execute(data_structure=data_structure, 
                                      # This will store the imported arrays into a newly generated DataGroup
                                      created_data_group=cx.DataPath(["Imported Data"]),  
                                      # We are not using this parameter but it still needs a value
                                      selected_data_group=cx.DataPath(),  
                                      # Use an existing DataGroup or AttributeMatrix. If an AttributemMatrix is used, the total number of tuples must match
                                      use_existing_group=False,   
                                      # The TextImporterData object with all member variables set.
                                      text_importer_data=text_importer_data # The TextImporterData object with all member variables set.
                                      )
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the ImportTextDataFilter filter")
