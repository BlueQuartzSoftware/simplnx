import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1
result = cx.CreateDataArray.execute(
    data_structure=data_structure,
    advanced_options=True,
    component_count=1,
    data_format="Unknown",
    initialization_value="2",
    numeric_type=4,
    output_data_array=cx.DataPath("TestArray"),
    tuple_dimensions=[0.0, 10.0],
)
#Filter 2
result = cx.CreateDataArray.execute(
    data_structure=data_structure,
    advanced_options=True,
    component_count=1,
    data_format="Unknown",
    initialization_value="1.23878",
    numeric_type=8,
    output_data_array=cx.DataPath("Confidence Index"),
    tuple_dimensions=[0.0, 10.0],
)
#Filter 3
result = cx.CreateDataArray.execute(
    data_structure=data_structure,
    advanced_options=True,
    component_count=3,
    data_format="Unknown",
    initialization_value="1.23878",
    numeric_type=8,
    output_data_array=cx.DataPath("EulerAngles"),
    tuple_dimensions=[0.0, 10.0],
)
#Filter 4
result = cx.ArrayCalculatorFilter.execute(
    data_structure=data_structure,
    calculated_array=cx.DataPath("Caclulated_TestArray"),
    infix_equation="TestArray+TestArray",
    scalar_type=8,
)
#Filter 5
result = cx.ArrayCalculatorFilter.execute(
    data_structure=data_structure,
    calculated_array=cx.DataPath("Caclulated_TestArray 2"),
    infix_equation="TestArray+TestArray",
    scalar_type=8,
)
#Filter 6
result = cx.ArrayCalculatorFilter.execute(
    data_structure=data_structure,
    calculated_array=cx.DataPath("Caclulated_TestArray 3"),
    infix_equation="TestArray+TestArray",
    scalar_type=8,
)
#Filter 7
result = cx.ArrayCalculatorFilter.execute(
    data_structure=data_structure,
    calculated_array=cx.DataPath("Caclulated_ConfidenceIndex"),
    infix_equation="Confidence Index*100",
    scalar_type=9,
)
#Filter 8
result = cx.ArrayCalculatorFilter.execute(
    data_structure=data_structure,
    calculated_array=cx.DataPath("Caclulated_EulerAngles"),
    infix_equation="EulerAngles/2",
    scalar_type=8,
)
#FIlter 9
result = cx.ArrayCalculatorFilter.execute(
    data_structure=data_structure,
    calculated_array=cx.DataPath("Caclulated_EulerAngles2"),
    infix_equation="EulerAngles[0]+EulerAngles[1]",
    scalar_type=8,
)
#Filter 10
result =cx.ExportDREAM3DFilter.execute(
    data_structure=data_structure,
    export_file_path=cx.DataPath("Data/Output/ArrayCalculatorExampleResults.dream3d"),
    write_xdmf_file=True,
)