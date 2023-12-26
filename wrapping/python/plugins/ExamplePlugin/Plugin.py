from ExamplePlugin.CalculateHistogramFilter import CalculateHistogramFilter
from ExamplePlugin.InterpolateGridDataFilter import InterpolateGridDataFilter
from ExamplePlugin.CliReaderFilter import CliReaderFilter
import complex as cx

class ExamplePlugin:
  def __init__(self) -> None:
    pass

  def id(self) -> cx.Uuid:
    return cx.Uuid('7ce1af33-d790-4378-9f75-b81483ce7737')

  def name(self) -> str:
    return 'ExamplePlugin'

  def description(self) -> str:
    return 'A plugin that shows examples of Python DREAM3D filters that AFRL may be interested in.'

  def vendor(self) -> str:
    return 'BlueQuartz Software'

  def get_filters(self):
    return [CalculateHistogramFilter, InterpolateGridDataFilter, CliReaderFilter]
