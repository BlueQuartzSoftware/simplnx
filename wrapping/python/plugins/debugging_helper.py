from typing import List
import simplnx as nx

# ------------------------------------------------------------------------------
# This NEEDS to be executed here so that we can load the python based plugin
# ------------------------------------------------------------------------------
import DataAnalysisToolkit
nx.load_python_plugin(DataAnalysisToolkit)
# Import our Filter from our plugin
import DataAnalysisToolkit.CliReaderFilter



# ------------------------------------------------------------------------------
# check_filter_execute_result
# ------------------------------------------------------------------------------
def check_filter_preflight_result(filter: nx.IFilter, result: nx.IFilter.PreflightResult) -> None:
    """This function will check the result of a filter's preflight method"""
    has_errors = len(result.get_result()) != 0 
    if has_errors:
        print(f'{filter.name()} :: Errors: {result.get_result()}')
        raise RuntimeError(result)
    
    print(f"{filter.name()} :: No errors preflighting the filter")

# ------------------------------------------------------------------------------
# check_filter_execute_result
# ------------------------------------------------------------------------------
def check_filter_execute_result(filter: nx.IFilter, result: nx.IFilter.ExecuteResult) -> None:
    """This function will check the result of a filter's execute method."""
    if len(result.warnings) != 0:
        print(f'{filter.name()} ::  Warnings: {result.warnings}')
    
    has_errors = len(result.errors) != 0 
    if has_errors:
        print(f'{filter.name()} :: Errors: {result.errors}')
        raise RuntimeError(result)
    
    print(f"{filter.name()} :: No errors running the filter")

# ------------------------------------------------------------------------------
# check_pipeline_execute_result
# ------------------------------------------------------------------------------
def check_pipeline_execute_result(result: nx.IFilter.ExecuteResult) -> None:
    """This method will check the result of a pipeline's execute method"""
    if len(result.warnings) != 0:
        print(f'{filter.name()} ::  Warnings: {result.warnings}')
    
    has_errors = len(result.errors) != 0 
    if has_errors:
        print(f'{filter.name()} :: Errors: {result.errors}')
        raise RuntimeError(result)
    
    print(f"Pipeline :: No errors running the pipeline")  





# ------------------------------------------------------------------------------
# Create a Data Structure
# ------------------------------------------------------------------------------
data_structure = nx.DataStructure()

# ------------------------------------------------------------------------------
# Wrap the python filter from the target plugin so we can use it.
# ------------------------------------------------------------------------------
pynx_filter = nx.PyFilter(DataAnalysisToolkit.CliReaderFilter())

# ------------------------------------------------------------------------------
# Execute the filter and check the result. We use the `execute2()` method to
# run the filter.
# ------------------------------------------------------------------------------
result = pynx_filter.execute2(data_structure=data_structure, 
                              cli_file_path="/Volumes/Sabrent-2TB/UDRI/MachineEquiv Final Layout_OSU_REV2.cli")
check_filter_execute_result(pynx_filter, result)

# ------------------------------------------------------------------------------
# Write the DataStructure out to a .dream3d file so we can look at it in DREAM3D-NX
# ------------------------------------------------------------------------------
result = nx.WriteDREAM3DFilter.execute(data_structure=data_structure, 
                                       export_file_path="/Volumes/Sabrent-2TB/UDRI/OUTPUT/MachineEquiv Final Layout_OSU_REV2.dream3d",
                                       write_xdmf_file=False)
check_filter_execute_result(nx.WriteDREAM3DFilter, result)

