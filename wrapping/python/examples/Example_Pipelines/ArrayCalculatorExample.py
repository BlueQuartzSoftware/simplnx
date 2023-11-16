import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

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
if len(result.warnings) != 0:
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
if len(result.warnings) != 0:
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
if len(result.warnings) != 0:
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
    infix_equation=calc_param, 
    scalar_type=cx.NumericType.float32
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 7
# Instantiate Filter
filter = cx.ArrayCalculatorFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    calculated_array=cx.DataPath("Caclulated_ConfidenceIndex"),
    infix_equation="Confidence Index*100",
    scalar_type=9
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")


# Filter 8
# Instantiate Filter
filter = cx.ArrayCalculatorFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    calculated_array=cx.DataPath("Caclulated_EulerAngles"),
    infix_equation="EulerAngles/2",
    scalar_type=8
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 9
# Instantiate Filter
filter = cx.ArrayCalculatorFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    calculated_array=cx.DataPath("Caclulated_EulerAngles2"),
    infix_equation="EulerAngles[0]+EulerAngles[1]",
    scalar_type=8
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 10
# Define output file path
output_file_path = "Data/Output/ArrayCalculatorExampleResults.dream3d"
# Instantiate Filter
filter = cx.ExportDREAM3DFilter()
# Execute Filter with Parameters
result = filter.execute(data_structure=data_structure, 
                       export_file_path=output_file_path, 
                       write_xdmf_file=True)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

print("===> Pipeline Complete")