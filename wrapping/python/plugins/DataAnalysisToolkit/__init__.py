
from DataAnalysisToolkit.Plugin import DataAnalysisToolkit

from DataAnalysisToolkit.CalculateHistogramFilter import CalculateHistogramFilter
from DataAnalysisToolkit.InterpolateGridDataFilter import InterpolateGridDataFilter
from DataAnalysisToolkit.CliReaderFilter import CliReaderFilter
from DataAnalysisToolkit.ContourDetectionFilter import ContourDetectionFilter
from DataAnalysisToolkit.NPSortArray import NPSortArray

# FILTER_INCLUDE_INSERT

def get_plugin():
  return DataAnalysisToolkit()

__all__ = ['DataAnalysisToolkit','CalculateHistogramFilter', 'InterpolateGridDataFilter', 'CliReaderFilter', 'get_plugin', 'ContourDetectionFilter', 'NPSortArray'] # FILTER_NAME_INSERT


