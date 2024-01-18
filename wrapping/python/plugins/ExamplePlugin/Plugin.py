
"""
Insert documentation here.
"""

from ExamplePlugin.ExampleFilter1 import ExampleFilter1
from ExamplePlugin.ExampleFilter2 import ExampleFilter2
from ExamplePlugin.CreateArray import CreateArrayFilter
from ExamplePlugin.InitializeData import InitializeDataPythonFilter
from ExamplePlugin.TemplateFilter import TemplateFilter

# FILTER_INCLUDE_INSERT

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
    return [ExampleFilter1,ExampleFilter2,CreateArrayFilter,InitializeDataPythonFilter,TemplateFilter] # FILTER_NAME_INSERT
