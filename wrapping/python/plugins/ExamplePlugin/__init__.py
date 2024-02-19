from ExamplePlugin.Plugin import ExamplePlugin

from ExamplePlugin.ExampleFilter1 import ExampleFilter1
from ExamplePlugin.ExampleFilter2 import ExampleFilter2
from ExamplePlugin.CreateArray import CreateArrayFilter
from ExamplePlugin.InitializeData import InitializeDataPythonFilter
from ExamplePlugin.TemplateFilter import TemplateFilter

# FILTER_INCLUDE_INSERT

def get_plugin():
  return ExamplePlugin()

__all__ = ['ExamplePlugin','ExampleFilter1', 'ExampleFilter2', 'CreateArrayFilter', 'InitializeDataPythonFilter', 'TemplateFilter', 'get_plugin'] # FILTER_NAME_INSERT

