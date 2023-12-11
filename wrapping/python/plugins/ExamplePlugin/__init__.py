from ExamplePlugin.Plugin import ExamplePlugin
from ExamplePlugin.CalculateHistogramFilter import CalculateHistogramFilter
from ExamplePlugin.InterpolateGridDataFilter import InterpolateGridDataFilter

def get_plugin():
  return ExamplePlugin()

__all__ = ['ExamplePlugin', 'CalculateHistogramFilter', 'InterpolateGridDataFilter', 'get_plugin']
