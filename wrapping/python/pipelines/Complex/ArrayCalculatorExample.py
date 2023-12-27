import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1
# Instantiate Filter
filter = cx.CreateDataArray()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=True,
    component_count=1,
    data_format="",
    initialization_value="2",
    numeric_type=cx.NumericType.int32,
    output_data_array=cx.DataPath("TestArray"),
    tuple_dimensions=[[10.0]]
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")


# Filter 2
# Instantiate Filter
filter = cx.CreateDataArray()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=True,
    component_count=1,
    data_format="",
    initialization_value="1.23878",
    numeric_type=cx.NumericType.float32,
    output_data_array=cx.DataPath("Confidence Index"),
    tuple_dimensions=[[10.0]]
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 3
# Instantiate Filter
filter = cx.CreateDataArray()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=True,
    component_count=3,
    data_format="",
    initialization_value="1.23878",
    numeric_type=cx.NumericType.float32,
    output_data_array=cx.DataPath("EulerAngles"),
    tuple_dimensions=[[10.0]]
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")


# Filter 4
# Instantiate Filter
filter = cx.ArrayCalculatorFilter()
calc_param = cx.CalculatorParameter.ValueType(cx.DataPath(""), "TestArray+TestArray", cx.CalculatorParameter.AngleUnits.Radians)
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    calculated_array=cx.DataPath("Caclulated_TestArray"),
    calculator_parameter=calc_param, 
    scalar_type=cx.NumericType.float32
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 5
# Instantiate Filter
filter = cx.ArrayCalculatorFilter()
calc_param = cx.CalculatorParameter.ValueType(cx.DataPath(""), "Confidence Index*100", cx.CalculatorParameter.AngleUnits.Radians)
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    calculated_array=cx.DataPath("Caclulated_ConfidenceIndex"),
    calculator_parameter=calc_param, 
    scalar_type=cx.NumericType.float64
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")


# Filter 6
# Instantiate Filter
filter = cx.ArrayCalculatorFilter()
calc_param = cx.CalculatorParameter.ValueType(cx.DataPath(""), "EulerAngles/2", cx.CalculatorParameter.AngleUnits.Radians)
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    calculated_array=cx.DataPath("Caclulated_EulerAngles"),
    calculator_parameter=calc_param, 
    scalar_type=cx.NumericType.float32
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 7
# Instantiate Filter
filter = cx.ArrayCalculatorFilter()
calc_param = cx.CalculatorParameter.ValueType(cx.DataPath(""), "EulerAngles[0]+EulerAngles[1]", cx.CalculatorParameter.AngleUnits.Radians)
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    calculated_array=cx.DataPath("Caclulated_EulerAngles2"),
    calculator_parameter=calc_param, 
    scalar_type=cx.NumericType.float32
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 8
# Define output file path
output_file_path = nxtest.GetDataDirectory() + "Data/Output/ArrayCalculatorExampleResults.dream3d"
# Instantiate Filter
filter = cx.WriteDREAM3DFilter()
# Execute Filter with Parameters
result = filter.execute(data_structure=data_structure, 
                       export_file_path=output_file_path, 
                       write_xdmf_file=True)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

print("===> Pipeline Complete")