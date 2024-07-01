"""
This code can be used to debug your python based DREAM3D-NX filter. There are a
number of bits of code that you will need to change in order for you to be able
to debug.


"""
from typing import List
import simplnx as nx

# ------------------------------------------------------------------------------
# This NEEDS to be executed here so that we can load the python based plugin
#
# You will need to REPLACE the name of the plugin and the name of the filter
# that you are trying to debug in the next 3 lines of code
# ------------------------------------------------------------------------------
import NXDataAnalysisToolkit
nx.load_python_plugin(NXDataAnalysisToolkit)
import NXDataAnalysisToolkit.CliReaderFilter


"""
The below are convenience functions that you can use to check the result of running
preflight or execute on the filter or execute on a loaded pipeline. You should
NOT have to change anything these functions.
"""
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


# *****************************************************************************
# This section is where you will need to programmatically execute what ever 
# needs to be done to prep your filter to run. This may involve programmatically
# running filters one after another or loading a pipeline to prep the DataStructure
# and then running your filter. Take a look at the Examples/scripts and 
# Examples/pipelines for examples to do that.
# *****************************************************************************


# Create a Data Structure
data_structure = nx.DataStructure()

# Wrap the python filter in this "proxy" class from the target plugin so we can use it.
pynx_filter = nx.PyFilter(NXDataAnalysisToolkit.CliReaderFilter())

# Execute the filter and check the result. We use the `execute2()` method to
# run the filter.
result = pynx_filter.execute2(data_structure=data_structure, 
                              cli_file_path="/paht/to/input/file.cli")
check_filter_execute_result(pynx_filter, result)

# ------------------------------------------------------------------------------
# If we want to check the results of the filter, we can save this file to a 
# dream3d file and load the .dream3d file directly into DREAM3D-NX to see the
# immediate results.
# ------------------------------------------------------------------------------
result = nx.WriteDREAM3DFilter.execute(data_structure=data_structure, 
                                       export_file_path="/path/to/output/file.dream3d",
                                       write_xdmf_file=False)
check_filter_execute_result(nx.WriteDREAM3DFilter, result)
