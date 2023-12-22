# Import the DREAM3D Base library and Plugins
import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import complex_test_dirs as cxtest

import numpy as np



# ------------------------------------------------------------------------------
# Create a DataArray that is as long as my CSV file (99 Rows in this case)
# ------------------------------------------------------------------------------
# Create a Data Structure
data_structure = nx.DataStructure()

param1 = cxor.OEMEbsdScanSelectionParameter.ValueType()
param1.input_file_path = "LEROY_0089_Section_382.h5"
param1.stacking_order = 0
param1.scan_names = ["LEROY_0089_Section_382"]

result = cxor.ReadH5EspritDataFilter.execute(data_structure = data_structure, 
                                             cell_attribute_matrix_name = "Cell Data", 
                                             cell_ensemble_attribute_matrix_name = "Cell Ensemble Data", 
                                             degrees_to_radians = True, 
                                             image_geometry_name = nx.DataPath("ImageGeom"),
                                             origin = [0.0, 0.0, 0.0], 
                                             read_pattern_data = False, 
                                             selected_scan_names = param1, 
                                             z_spacing = 1.0)

if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the ReadH5EspritDataFilter")




#------------------------------------------------------------------------------
# Write the DataStructure to a .dream3d file
#------------------------------------------------------------------------------
output_file_path = cxtest.GetTestTempDirectory() + "/import_esprit.dream3d"
result = nx.WriteDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the WriteDREAM3DFilter")

print(f'{output_file_path}')
