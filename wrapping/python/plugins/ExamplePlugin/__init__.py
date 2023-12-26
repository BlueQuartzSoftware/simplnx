from ExamplePlugin.Plugin import ExamplePlugin
from ExamplePlugin.CalculateHistogramFilter import CalculateHistogramFilter
from ExamplePlugin.InterpolateGridDataFilter import InterpolateGridDataFilter
from ExamplePlugin.CliReaderFilter import CliReaderFilter

def get_plugin():
  return ExamplePlugin()

__all__ = ['ExamplePlugin', 'CalculateHistogramFilter', 'InterpolateGridDataFilter', 'CliReaderFilter', 'get_plugin']
