
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

