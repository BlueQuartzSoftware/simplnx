from DataAnalysisToolkit.CalculateHistogramFilter import CalculateHistogramFilter
from DataAnalysisToolkit.InterpolateGridDataFilter import InterpolateGridDataFilter
from DataAnalysisToolkit.CliReaderFilter import CliReaderFilter
from DataAnalysisToolkit.ContourDetectionFilter import ContourDetectionFilter
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
    return [CalculateHistogramFilter, InterpolateGridDataFilter, CliReaderFilter, ContourDetectionFilter]
