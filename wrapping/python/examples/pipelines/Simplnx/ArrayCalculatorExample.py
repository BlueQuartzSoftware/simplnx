import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

#Create a Data Structure
data_structure = nx.DataStructure()

# Filter 1
# Instantiate Filter
filter = nx.CreateDataArray()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=True,
    component_count=1,
    data_format="",
    initialization_value="2",
    numeric_type=nx.NumericType.int32,
    output_data_array=nx.DataPath("TestArray"),
    tuple_dimensions=[[10.0]]
)
nxtest.check_filter_result(filter, result)


# Filter 2
# Instantiate Filter
filter = nx.CreateDataArray()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=True,
    component_count=1,
    data_format="",
    initialization_value="1.23878",
    numeric_type=nx.NumericType.float32,
    output_data_array=nx.DataPath("Confidence Index"),
    tuple_dimensions=[[10.0]]
)
nxtest.check_filter_result(filter, result)

# Filter 3
# Instantiate Filter
filter = nx.CreateDataArray()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=True,
    component_count=3,
    data_format="",
    initialization_value="1.23878",
    numeric_type=nx.NumericType.float32,
    output_data_array=nx.DataPath("EulerAngles"),
    tuple_dimensions=[[10.0]]
)
nxtest.check_filter_result(filter, result)


# Filter 4
# Instantiate Filter
filter = nx.ArrayCalculatorFilter()
calc_param = nx.CalculatorParameter.ValueType(nx.DataPath(""), "TestArray+TestArray", nx.CalculatorParameter.AngleUnits.Radians)
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    calculated_array=nx.DataPath("Caclulated_TestArray"),
    calculator_parameter=calc_param, 
    scalar_type=nx.NumericType.float32
)
nxtest.check_filter_result(filter, result)

# Filter 5
# Instantiate Filter
filter = nx.ArrayCalculatorFilter()
calc_param = nx.CalculatorParameter.ValueType(nx.DataPath(""), "Confidence Index*100", nx.CalculatorParameter.AngleUnits.Radians)
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    calculated_array=nx.DataPath("Caclulated_ConfidenceIndex"),
    calculator_parameter=calc_param, 
    scalar_type=nx.NumericType.float64
)
nxtest.check_filter_result(filter, result)


# Filter 6
# Instantiate Filter
filter = nx.ArrayCalculatorFilter()
calc_param = nx.CalculatorParameter.ValueType(nx.DataPath(""), "EulerAngles/2", nx.CalculatorParameter.AngleUnits.Radians)
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    calculated_array=nx.DataPath("Caclulated_EulerAngles"),
    calculator_parameter=calc_param, 
    scalar_type=nx.NumericType.float32
)
nxtest.check_filter_result(filter, result)

# Filter 7
# Instantiate Filter
filter = nx.ArrayCalculatorFilter()
calc_param = nx.CalculatorParameter.ValueType(nx.DataPath(""), "EulerAngles[0]+EulerAngles[1]", nx.CalculatorParameter.AngleUnits.Radians)
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    calculated_array=nx.DataPath("Caclulated_EulerAngles2"),
    calculator_parameter=calc_param, 
    scalar_type=nx.NumericType.float32
)
nxtest.check_filter_result(filter, result)

# Filter 8
# Define output file path
output_file_path = nxtest.GetDataDirectory() + "//Output/ArrayCalculatorExampleResults.dream3d"
# Instantiate Filter
filter = nx.WriteDREAM3DFilter()
# Execute Filter with Parameters
result = filter.execute(data_structure=data_structure, 
                       export_file_path=output_file_path, 
                       write_xdmf_file=True)
nxtest.check_filter_result(filter, result)

# *****************************************************************************
# THIS SECTION IS ONLY HERE FOR CLEANING UP THE CI Machines
# If you are using this code, you should COMMENT out the next line
nxtest.cleanup_test_file(output_file_path)
# *****************************************************************************


print("===> Pipeline Complete")