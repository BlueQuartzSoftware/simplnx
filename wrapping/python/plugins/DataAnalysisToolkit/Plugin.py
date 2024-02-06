from DataAnalysisToolkit.CalculateHistogramFilter import CalculateHistogramFilter
from DataAnalysisToolkit.InterpolateGridDataFilter import InterpolateGridDataFilter
from DataAnalysisToolkit.CliReaderFilter import CliReaderFilter
import simplnx as sx

class DataAnalysisToolkit:
  def __init__(self) -> None:
    pass

  def id(self) -> sx.Uuid:
    return sx.Uuid('7ce1af33-d790-4378-9f75-b81483ce7737')

  def name(self) -> str:
    return 'DataAnalysisToolkit'

  def description(self) -> str:
    return 'A plugin that shows examples of Python DREAM3D filters that analyze data, specifically using Python libraries like numpy, scipy, and matplotlib.'

  def vendor(self) -> str:
    return 'BlueQuartz Software'

  def get_filters(self):
    return [CalculateHistogramFilter, InterpolateGridDataFilter, CliReaderFilter]
