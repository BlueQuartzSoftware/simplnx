import complex as cx
import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

# Create the DataStructure object
data_structure = cx.DataStructure()

# Create the ReadH5EbsdFileParameter and assign values to it.
h5ebsdParameter = cxor.ReadH5EbsdFileParameter.ValueType()
h5ebsdParameter.euler_representation=0
h5ebsdParameter.end_slice=117
h5ebsdParameter.selected_array_names=["Confidence Index", "EulerAngles", "Fit", "Image Quality", "Phases", "SEM Signal", "X Position", "Y Position"]
h5ebsdParameter.input_file_path="Data/Output/Reconstruction/Small_IN100.h5ebsd"
h5ebsdParameter.start_slice=1
h5ebsdParameter.use_recommended_transform=True

# Execute Filter with Parameters
result = cxor.ReadH5EbsdFilter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="CellData",
    cell_ensemble_attribute_matrix_name="CellEnsembleData",
    data_container_name=cx.DataPath("DataContainer"),
    read_h5_ebsd_parameter=h5ebsdParameter
)

dataset1 = cx.ReadHDF5DatasetParameter.DatasetImportInfo()
dataset1.dataset_path = "/DataStructure/DataContainer/CellData/Confidence Index"
dataset1.tuple_dims = "117,201,189"
dataset1.component_dims = "1"

dataset2 = cx.ReadHDF5DatasetParameter.DatasetImportInfo()
dataset2.dataset_path = "/DataStructure/DataContainer/CellData/EulerAngles"
dataset2.tuple_dims = "117,201,189"
dataset2.component_dims = "3"

import_hdf5_param = cx.ReadHDF5DatasetParameter.ValueType()
import_hdf5_param.input_file = "Data/Output/Reconstruction/SmallIN100_Final.dream3d"
import_hdf5_param.datasets = [dataset1, dataset2]
# import_hdf5_param.parent = cx.DataPath(["Imported Data"])

result = cx.ReadHDF5Dataset.execute(data_structure=data_structure,
                                      import_hd_f5_file=import_hdf5_param
                                      )
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the ReadHDF5DatasetParameter filter")

