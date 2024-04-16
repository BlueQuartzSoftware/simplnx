
from DataAnalysisToolkit.Plugin import DataAnalysisToolkit

__all__ = ['DataAnalysisToolkit', 'get_plugin']

try:
  from DataAnalysisToolkit.CalculateHistogramFilter import CalculateHistogramFilter
  __all__.append('CalculateHistogramFilter')
except ImportError:
  pass
try:
  from DataAnalysisToolkit.InterpolateGridDataFilter import InterpolateGridDataFilter
  __all__.append('InterpolateGridDataFilter')
except ImportError:
  pass
try:
  from DataAnalysisToolkit.CliReaderFilter import CliReaderFilter
  __all__.append('CliReaderFilter')
except ImportError:
  pass
try:
  from DataAnalysisToolkit.ContourDetectionFilter import ContourDetectionFilter
  __all__.append('ContourDetectionFilter')
except ImportError:
  pass
try:
  from DataAnalysisToolkit.NPSortArray import NPSortArray
  __all__.append('NPSortArray')
except ImportError:
  pass
try:
  from DataAnalysisToolkit.ReadPeregrineHDF5File import ReadPeregrineHDF5File
  __all__.append('ReadPeregrineHDF5File')
except ImportError:
  pass


from DataAnalysisToolkit.ReadPeregrineHDF5File import ReadPeregrineHDF5File
# FILTER_INCLUDE_INSERT

def get_plugin():
  return DataAnalysisToolkit()