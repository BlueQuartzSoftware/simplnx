from ExamplePlugin.Plugin import ExamplePlugin

__all__ = ['ExamplePlugin', 'get_plugin']

try:
  from ExamplePlugin.ExampleFilter1 import ExampleFilter1
  __all__.append('ExampleFilter1')
except ImportError:
  pass
try:
  from ExamplePlugin.ExampleFilter2 import ExampleFilter2
  __all__.append('ExampleFilter2')
except ImportError:
  pass
try:
  from ExamplePlugin.CreateArray import CreateArrayFilter
  __all__.append('CreateArrayFilter')
except ImportError:
  pass
try:
  from ExamplePlugin.InitializeDataFilter import InitializeDataPythonFilter
  __all__.append('InitializeDataPythonFilter')
except ImportError:
  pass
try:
  from ExamplePlugin.TemplateFilter import TemplateFilter
  __all__.append('TemplateFilter')
except ImportError:
  pass

# FILTER_INCLUDE_INSERT

def get_plugin():
  return ExamplePlugin()
