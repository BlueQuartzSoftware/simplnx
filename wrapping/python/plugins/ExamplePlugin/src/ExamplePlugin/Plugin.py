
"""
Insert documentation here.
"""

_filters = []

try:
  from ExamplePlugin.ExampleFilter1 import ExampleFilter1
  _filters.append(ExampleFilter1)
except ImportError:
  pass
try:
  from ExamplePlugin.ExampleFilter2 import ExampleFilter2
  _filters.append(ExampleFilter2)
except ImportError:
  pass
try:
  from ExamplePlugin.CreateArray import CreateArrayFilter
  _filters.append(CreateArrayFilter)
except ImportError:
  pass
try:
  from ExamplePlugin.InitializeDataFilter import InitializeDataPythonFilter
  _filters.append(InitializeDataPythonFilter)
except ImportError:
  pass
try:
  from ExamplePlugin.TemplateFilter import TemplateFilter
  _filters.append(TemplateFilter)
except ImportError:
  pass



import simplnx as nx

class ExamplePlugin:
  def __init__(self) -> None:
    pass

  def id(self) -> nx.Uuid:
    return nx.Uuid('29b0bf6b-b67d-4030-92cd-bcdf7a7196d4')

  def name(self) -> str:
    return 'ExamplePlugin'

  def description(self) -> str:
    return 'ExamplePlugin'

  def vendor(self) -> str:
    return 'Description'

  def get_filters(self):
    return _filters 
