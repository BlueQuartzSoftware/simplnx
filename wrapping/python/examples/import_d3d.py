import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import complex_test_dirs as cxtest

import numpy as np


# Create the DataStructure object
data_structure = nx.DataStructure()

import_data = cx.Dream3dImportParameter.ImportData()
import_data.file_path = cxtest.GetTestTempDirectory() + "/basic_ebsd_example.dream3d"
import_data.data_paths = None  # Use 'None' to import the entire file.

print(f'{import_data.file_path}')

result = cx.ReadDREAM3DFilter.execute(data_structure=data_structure, import_file_data=import_data)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the ReadDREAM3DFilter filter")


#------------------------------------------------------------------------------
# Print out the children of some of the Attribute Matrix groups
#------------------------------------------------------------------------------
children = data_structure.get_children("Small IN100")
for child in children:
    print(f'{child}')

children = data_structure.get_children("Small IN100/Scan Data")
for child in children:
    print(f'{child}')

#------------------------------------------------------------------------------
# Get the underlying data from the DataStructure
#------------------------------------------------------------------------------
npview_data_path = nx.DataPath("Small IN100/Scan Data/Image Quality")
npview = data_structure[npview_data_path].npview()

# Change the underlying data based on some criteria using Numpy
npview[npview < 120] = 0

#------------------------------------------------------------------------------
# Write the DataStructure to a .dream3d file
#------------------------------------------------------------------------------
output_file_path = cxtest.GetTestTempDirectory() + "/import_data.dream3d"
result = cx.WriteDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the WriteDREAM3DFilter")
