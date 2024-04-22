
"""
This is a collection of filters that showcase using python packages to perform various
functions.

You may need the following python packages installed to see every filter
- matplotlib
- opencv *(Requires Python 3.12)
- scipy

"""

_filters = []

try:
  from DataAnalysisToolkit.CalculateHistogramFilter import CalculateHistogramFilter
  _filters.append(CalculateHistogramFilter)
except ImportError:
  pass
try:
  from DataAnalysisToolkit.InterpolateGridDataFilter import InterpolateGridDataFilter
  _filters.append(InterpolateGridDataFilter)
except ImportError:
  pass
try:
  from DataAnalysisToolkit.CliReaderFilter import CliReaderFilter
  _filters.append(CliReaderFilter)
except ImportError:
  pass
try:
  from DataAnalysisToolkit.ContourDetectionFilter import ContourDetectionFilter
  _filters.append(ContourDetectionFilter)
except ImportError:
  pass
try:
  from DataAnalysisToolkit.NPSortArray import NPSortArray
  _filters.append(NPSortArray)
except ImportError:
  pass


import simplnx as nx

class DataAnalysisToolkit:
  def __init__(self) -> None:
    pass

  def id(self) -> nx.Uuid:
    return nx.Uuid('7ce1af33-d790-4378-9f75-b81483ce7737')

  def name(self) -> str:
    return 'DataAnalysisToolkit'

  def description(self) -> str:
    return 'A plugin that shows examples of Python DREAM3D filters that analyze data, specifically using Python libraries like numpy, scipy, and matplotlib.'

  def vendor(self) -> str:
    return 'BlueQuartz Software'

  def get_filters(self):
    return _filters 


