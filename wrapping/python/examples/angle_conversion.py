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
# Create a DataArray to copy the Euler Angles into 
array_path = nx.DataPath(['Euler Angles'])
result = nx.CreateDataArray.execute(data_structure=data_structure,
                                    numeric_type=nx.NumericType.float32,
                                    component_count=3,
                                    tuple_dimensions=[[99]],
                                    output_data_array=array_path,
                                    initialization_value='0')
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the CreateDataArray")

# Get a numpy.view into the newly created DataArray
data_array = data_structure[array_path]
# Get the underlying DataStore object
data_store = data_array.store
# Get a Numpy View into the data
npdata = data_store.npview()

# Read the CSV file into the DataArray using the numpy view
file_path = cxtest.GetComplexPythonSourceDir() + '/examples/angles.csv'
npdata[:] = np.loadtxt(file_path, delimiter=',')

# ------------------------------------------------------------------------------
# Run the ConvertOrientation Filter to convert the Eulers to Quaternions
# ------------------------------------------------------------------------------
quat_path = nx.DataPath(['Quaternions'])
result = cxor.ConvertOrientations.execute(data_structure=data_structure,
                                          input_orientation_array_path=array_path,
                                          input_type=0,
                                          output_orientation_array_name='Quaternions',
                                          output_type=2)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the ConvertOrientations")

# Get the Quaternions and print them out.
data_array = data_structure[quat_path]
data_store = data_array.store
npdata = data_store.npview()

print(npdata)
