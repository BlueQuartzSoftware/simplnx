from TestPlugin.Plugin import TestPythonPlugin
from TestPlugin.CreateArray import CreateArrayFilter
from TestPlugin.InitializeData import InitializeDataPythonFilter
from TestPlugin.TemplateFilter import TemplateFilter

def get_plugin():
  return TestPythonPlugin()

__all__ = ['TestPythonPlugin', 'CreateArrayFilter', 'InitializeDataPythonFilter', 'TemplateFilter', 'get_plugin']
