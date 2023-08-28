# Import the DREAM3D Base library and Plugins
import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

# Create a Data Structure
data_structure = cx.DataStructure()

ensemble_info_parameter = []
ensemble_info_parameter.append(["Hexagonal-High 6/mmm","Primary","Phase 1"])
ensemble_info_parameter.append(["Cubic-High m-3m","Primary","Phase 2"])

create_ensemble_info = cxor.CreateEnsembleInfoFilter()
result = create_ensemble_info.execute(data_structure=data_structure,
                             cell_ensemble_attribute_matrix_name=cx.DataPath(["Phase Information"]), 
                             crystal_structures_array_name="CrystalStructures", 
                             phase_names_array_name="Phase Names", 
                             phase_types_array_name="Primary", 
                             ensemble=ensemble_info_parameter
                             )
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the filter")

