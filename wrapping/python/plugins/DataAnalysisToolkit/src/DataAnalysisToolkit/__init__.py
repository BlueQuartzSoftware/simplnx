from DataAnalysisToolkit.Plugin import DataAnalysisToolkit

__all__ = ['DataAnalysisToolkit', 'get_plugin']

"""
This section conditionally tries to import each filter
"""

# FILTER_START: CalculateHistogramFilter
try:
  from DataAnalysisToolkit.CalculateHistogramFilter import CalculateHistogramFilter
  __all__.append('CalculateHistogramFilter')
except ImportError:
  pass
# FILTER_END: CalculateHistogramFilter

# FILTER_START: InterpolateGridDataFilter
try:
  from DataAnalysisToolkit.InterpolateGridDataFilter import InterpolateGridDataFilter
  __all__.append('InterpolateGridDataFilter')
except ImportError:
  pass
# FILTER_END: InterpolateGridDataFilter

# FILTER_START: CliReaderFilter
try:
  from DataAnalysisToolkit.CliReaderFilter import CliReaderFilter
  __all__.append('CliReaderFilter')
except ImportError:
  pass
# FILTER_END: CliReaderFilter

# FILTER_START: ContourDetectionFilter
try:
  from DataAnalysisToolkit.ContourDetectionFilter import ContourDetectionFilter
  __all__.append('ContourDetectionFilter')
except ImportError:
  pass
# FILTER_END: ContourDetectionFilter

# FILTER_START: NPSortArray
try:
  from DataAnalysisToolkit.NPSortArray import NPSortArray
  __all__.append('NPSortArray')
except ImportError:
  pass
# FILTER_END: NPSortArray

# FILTER_START: ReadPeregrineHDF5File
try:
  from DataAnalysisToolkit.ReadPeregrineHDF5File import ReadPeregrineHDF5File
  __all__.append('ReadPeregrineHDF5File')
except ImportError:
  pass
# FILTER_END: ReadPeregrineHDF5File

def get_plugin():
  return DataAnalysisToolkit()
