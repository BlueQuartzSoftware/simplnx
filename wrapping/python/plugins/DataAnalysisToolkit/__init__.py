from DataAnalysisToolkit.Plugin import DataAnalysisToolkit
from DataAnalysisToolkit.CalculateHistogramFilter import CalculateHistogramFilter
from DataAnalysisToolkit.InterpolateGridDataFilter import InterpolateGridDataFilter
from DataAnalysisToolkit.CliReaderFilter import CliReaderFilter
from DataAnalysisToolkit.ContourStatisticsFilter import ContourStatisticsFilter

def get_plugin():
  return DataAnalysisToolkit()

__all__ = ['DataAnalysisToolkit', 'CalculateHistogramFilter', 'InterpolateGridDataFilter', 'CliReaderFilter', 'ContourStatisticsFilter', 'get_plugin']
