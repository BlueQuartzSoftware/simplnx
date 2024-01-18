from ExamplePlugin.Plugin import ExamplePlugin

from ExamplePlugin.ExampleFilter1 import ExampleFilter1
from ExamplePlugin.ExampleFilter2 import ExampleFilter2

# FILTER_INCLUDE_INSERT

def get_plugin():
  return ExamplePlugin()

__all__ = ['ExamplePlugin','ExampleFilter1', 'ExampleFilter2', 'get_plugin'] # FILTER_NAME_INSERT

