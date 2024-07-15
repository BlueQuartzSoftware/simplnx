
"""
This is a collection of filters that showcase using python packages to perform various
functions.

You may need the following python packages installed to see every filter
- matplotlib
- opencv *(Requires Python 3.12)
- scipy

"""

_filters = []

# FILTER_START: CalculateHistogramFilter
from .CalculateHistogramFilter import CalculateHistogramFilter
_filters.append(CalculateHistogramFilter)
# FILTER_END: CalculateHistogramFilter

# FILTER_START: InterpolateGridDataFilter
from .InterpolateGridDataFilter import InterpolateGridDataFilter
_filters.append(InterpolateGridDataFilter)
# FILTER_END: InterpolateGridDataFilter

# FILTER_START: CliReaderFilter
from .CliReaderFilter import CliReaderFilter
_filters.append(CliReaderFilter)
# FILTER_END: CliReaderFilter

# FILTER_START: ContourDetectionFilter
from .ContourDetectionFilter import ContourDetectionFilter
_filters.append(ContourDetectionFilter)
# FILTER_END: ContourDetectionFilter

# FILTER_START: NPSortArray
from .NPSortArray import NPSortArray
_filters.append(NPSortArray)
# FILTER_END: NPSortArray

# FILTER_START: ReadPeregrineHDF5File
from .ReadPeregrineHDF5File import ReadPeregrineHDF5File
_filters.append(ReadPeregrineHDF5File)
# FILTER_END: ReadPeregrineHDF5File

# FILTER_START: WriteAbaqusFile
from .WriteAbaqusFile import WriteAbaqusFile
_filters.append(WriteAbaqusFile)
# FILTER_END: WriteAbaqusFile

# FILTER_START: WriteAnsysFile
from .WriteAnsysFile import WriteAnsysFile
_filters.append(WriteAnsysFile)
# FILTER_END: WriteAnsysFile

# FILTER_START: WriteMedFile
from .WriteMedFile import WriteMedFile
_filters.append(WriteMedFile)
# FILTER_END: WriteMedFile

# FILTER_START: WriteGmshFile
from .WriteGmshFile import WriteGmshFile
_filters.append(WriteGmshFile)
# FILTER_END: WriteGmshFile

# FILTER_START: WriteTetGenFile
from .WriteTetGenFile import WriteTetGenFile
_filters.append(WriteTetGenFile)
# FILTER_END: WriteTetGenFile

# FILTER_START: WriteVtuFile
from .WriteVtuFile import WriteVtuFile
_filters.append(WriteVtuFile)
# FILTER_END: WriteVtuFile

# FILTER_START: ReadMeshFile
from .ReadMeshFile import ReadMeshFile
_filters.append(ReadMeshFile)
# FILTER_END: ReadMeshFile

import simplnx as nx

class NXDataAnalysisToolkit:
  def __init__(self) -> None:
    pass

  def id(self) -> nx.Uuid:
    return nx.Uuid('7ce1af33-d790-4378-9f75-b81483ce7737')

  def name(self) -> str:
    return 'NXDataAnalysisToolkit'

  def description(self) -> str:
    return 'A plugin containing Python DREAM3D filters that analyze data, specifically using Python libraries like numpy, scipy, and matplotlib.'

  def vendor(self) -> str:
    return 'BlueQuartz Software'

  def get_filters(self):
    return _filters

