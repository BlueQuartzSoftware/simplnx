from DataAnalysisToolkit.Plugin import DataAnalysisToolkit
from DataAnalysisToolkit.CalculateHistogramFilter import CalculateHistogramFilter
from DataAnalysisToolkit.InterpolateGridDataFilter import InterpolateGridDataFilter
from DataAnalysisToolkit.CliReaderFilter import CliReaderFilter

def get_plugin():
  return DataAnalysisToolkit()

__all__ = ['DataAnalysisToolkit', 'CalculateHistogramFilter', 'InterpolateGridDataFilter', 'CliReaderFilter', 'get_plugin']
