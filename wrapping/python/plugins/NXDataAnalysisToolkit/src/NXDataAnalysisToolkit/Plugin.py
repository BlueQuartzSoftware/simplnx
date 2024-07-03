
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
try:
  from NXDataAnalysisToolkit.CalculateHistogramFilter import CalculateHistogramFilter
  _filters.append(CalculateHistogramFilter)
except ImportError:
  pass
# FILTER_END: CalculateHistogramFilter

# FILTER_START: InterpolateGridDataFilter
try:
  from NXDataAnalysisToolkit.InterpolateGridDataFilter import InterpolateGridDataFilter
  _filters.append(InterpolateGridDataFilter)
except ImportError:
  pass
# FILTER_END: InterpolateGridDataFilter

# FILTER_START: CliReaderFilter
try:
  from NXDataAnalysisToolkit.CliReaderFilter import CliReaderFilter
  _filters.append(CliReaderFilter)
except ImportError:
  pass
# FILTER_END: CliReaderFilter

# FILTER_START: ContourDetectionFilter
try:
  from NXDataAnalysisToolkit.ContourDetectionFilter import ContourDetectionFilter
  _filters.append(ContourDetectionFilter)
except ImportError:
  pass
# FILTER_END: ContourDetectionFilter

# FILTER_START: NPSortArray
try:
  from NXDataAnalysisToolkit.NPSortArray import NPSortArray
  _filters.append(NPSortArray)
except ImportError:
  pass
# FILTER_END: NPSortArray

# FILTER_START: ReadPeregrineHDF5File
try:
  from NXDataAnalysisToolkit.ReadPeregrineHDF5File import ReadPeregrineHDF5File
  _filters.append(ReadPeregrineHDF5File)
except ImportError:
  pass
# FILTER_END: ReadPeregrineHDF5File

# FILTER_START: WriteAbaqusFile
try:
  from DataAnalysisToolkit.WriteAbaqusFile import WriteAbaqusFile
  _filters.append(WriteAbaqusFile)
except ImportError:
  pass
# FILTER_END: WriteAbaqusFile

# FILTER_START: WriteAnsysFile
try:
  from DataAnalysisToolkit.WriteAnsysFile import WriteAnsysFile
  _filters.append(WriteAnsysFile)
except ImportError:
  pass
# FILTER_END: WriteAnsysFile

# FILTER_START: WriteMedFile
try:
  from DataAnalysisToolkit.WriteMedFile import WriteMedFile
  _filters.append(WriteMedFile)
except ImportError:
  pass
# FILTER_END: WriteMedFile

# FILTER_START: WriteGmshFile
try:
  from DataAnalysisToolkit.WriteGmshFile import WriteGmshFile
  _filters.append(WriteGmshFile)
except ImportError:
  pass
# FILTER_END: WriteGmshFile

# FILTER_START: WriteTetGenFile
try:
  from DataAnalysisToolkit.WriteTetGenFile import WriteTetGenFile
  _filters.append(WriteTetGenFile)
except ImportError:
  pass
# FILTER_END: WriteTetGenFile

# FILTER_START: WriteVtuFile
try:
  from DataAnalysisToolkit.WriteVtuFile import WriteVtuFile
  _filters.append(WriteVtuFile)
except ImportError:
  pass
# FILTER_END: WriteVtuFile

# FILTER_START: ReadMeshFile
try:
  from DataAnalysisToolkit.ReadMeshFile import ReadMeshFile
  _filters.append(ReadMeshFile)
except ImportError:
  pass
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

