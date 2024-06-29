from NXDataAnalysisToolkit.Plugin import NXDataAnalysisToolkit

__all__ = ['NXDataAnalysisToolkit', 'get_plugin']

"""
This section conditionally tries to import each filter
"""

# FILTER_START: CalculateHistogramFilter
try:
  from NXDataAnalysisToolkit.CalculateHistogramFilter import CalculateHistogramFilter
  __all__.append('CalculateHistogramFilter')
except ImportError:
  pass
# FILTER_END: CalculateHistogramFilter

# FILTER_START: InterpolateGridDataFilter
try:
  from NXDataAnalysisToolkit.InterpolateGridDataFilter import InterpolateGridDataFilter
  __all__.append('InterpolateGridDataFilter')
except ImportError:
  pass
# FILTER_END: InterpolateGridDataFilter

# FILTER_START: CliReaderFilter
try:
  from NXDataAnalysisToolkit.CliReaderFilter import CliReaderFilter
  __all__.append('CliReaderFilter')
except ImportError:
  pass
# FILTER_END: CliReaderFilter

# FILTER_START: ContourDetectionFilter
try:
  from NXDataAnalysisToolkit.ContourDetectionFilter import ContourDetectionFilter
  __all__.append('ContourDetectionFilter')
except ImportError:
  pass
# FILTER_END: ContourDetectionFilter

# FILTER_START: NPSortArray
try:
  from NXDataAnalysisToolkit.NPSortArray import NPSortArray
  __all__.append('NPSortArray')
except ImportError:
  pass
# FILTER_END: NPSortArray

# FILTER_START: ReadPeregrineHDF5File
try:
  from NXDataAnalysisToolkit.ReadPeregrineHDF5File import ReadPeregrineHDF5File
  __all__.append('ReadPeregrineHDF5File')
except ImportError:
  pass
# FILTER_END: ReadPeregrineHDF5File

def get_plugin():
  return NXDataAnalysisToolkit()
