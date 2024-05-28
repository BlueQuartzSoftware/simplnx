# import simplnx as nx
# import DataAnalysisToolkit
# nx.load_python_plugin(DataAnalysisToolkit)

# # Create a Data Structure
# data_structure = nx.DataStructure()

# # Wrap the python filter in this "proxy" class from the target plugin so we can use it.
# pynx_filter = nx.PyFilter(DataAnalysisToolkit.CliReaderFilter())

# # Execute the filter and check the result. We use the `execute2()` method to
# # run the filter.
# result = pynx_filter.execute2(data_structure=data_structure, 
#                               cli_file_path="/Volumes/BQ_Data/Data/CLI_Files/MachineEquiv Final Layout_OSU_REV2.cli",
#                               mask_x_dimension=False,
#                               mask_y_dimension=False,
#                               mask_z_dimension=False,
#                               min_max_x_coords=[-70,-53],
#                               min_max_y_coords=[-20,0],
#                               min_max_z_coords=[-2,20])

# import numpy as np
# import simplnx as nx
# import DataAnalysisToolkit
# nx.load_python_plugin(DataAnalysisToolkit)

# # Create a Data Structure
# data_structure = nx.DataStructure()

# # Wrap the python filter in this "proxy" class from the target plugin so we can use it.
# pynx_filter = nx.PyFilter(DataAnalysisToolkit.CliReaderFilter())

# # Execute the filter and check the result. We use the `execute2()` method to
# # run the filter.
# result = pynx_filter.execute2(data_structure=data_structure, 
#                               cli_file_path="/Volumes/BQ_Data/Data/CLI_Files/MachineEquiv Final Layout_OSU_REV2.cli",
#                             #   use_x_dimension_range=True,
#                             #   min_max_x_coords=[-60, -30],
#                             #   use_y_dimension_range=True,
#                             #   min_max_y_coords=[-60,-40],
#                               use_z_dimension_range=True,
#                               min_max_z_coords=[5000,6000],
#                               out_of_bounds_behavior=0)


import simplnx as nx

data_structure = nx.DataStructure()
pipeline = nx.Pipeline().from_file('Pipelines/lesson_2.d3dpipeline')
for index, filter in enumerate(pipeline):
  print(f"[{index}]: {filter.get_filter().human_name()}")

create_data_group_args = {
        "data_object_path": nx.DataPath("Small IN100/EBSD Data")
    }

pipeline.insert(2, nx.CreateDataGroup(), create_data_group_args)

for index, filter in enumerate(pipeline):
  print(f"[{index}]: {filter.get_filter().human_name()}")

def check_pipeline_result(result: nx.Result) -> None:
    """
    This function will check the `result` for any errors. If errors do exist then a 
    `RuntimeError` will be thrown. Your own code to modify this to return something
    else that doesn't just stop your script in its tracks.
    """
    if len(result.warnings) != 0:
        for w in result.warnings:
            print(f'Warning: ({w.code}) {w.message}')
    
    has_errors = len(result.errors) != 0 
    if has_errors:
        for err in result.errors:
            print(f'Error: ({err.code}) {err.message}')
        raise RuntimeError(result)
    
    print(f"Pipeline :: No errors running the pipeline")

result = pipeline.execute(data_structure)
check_pipeline_result(result=result)