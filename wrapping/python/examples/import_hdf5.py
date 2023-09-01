import complex as cx
import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

# Create the DataStructure object
data_structure = cx.DataStructure()


dataset1 = cx.ImportHDF5DatasetParameter.DatasetImportInfo()
dataset1.dataset_path = "/DataStructure/DataContainer/CellData/Confidence Index"
dataset1.tuple_dims = "117,201,189"
dataset1.component_dims = "1"

dataset2 = cx.ImportHDF5DatasetParameter.DatasetImportInfo()
dataset2.dataset_path = "/DataStructure/DataContainer/CellData/EulerAngles"
dataset2.tuple_dims = "117,201,189"
dataset2.component_dims = "3"

import_hdf5_param = cx.ImportHDF5DatasetParameter()
import_hdf5_param.input_file = "/Users/mjackson/DREAM3DNXData/Data/Output/Reconstruction/SmallIN100_Final.dream3d"
import_hdf5_param.datasets = [dataset1, dataset2]
# import_hdf5_param.parent = cx.DataPath(["Imported Data"])

result = cx.ImportHDF5Dataset.execute(data_structure=data_structure,
                                      import_hd_f5_file=import_hdf5_param
                                      )
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the ImportHDF5DatasetParameter filter")
