from TestPlugin.CreateArray import CreateArrayFilter
from TestPlugin.InitializeData import InitializeDataPythonFilter
from TestPlugin.TemplateFilter import TemplateFilter
import simplnx as nx

class TestPythonPlugin:
  def __init__(self) -> None:
    pass

  def id(self) -> nx.Uuid:
    return nx.Uuid('a7d5db89-3bb3-4fed-a590-e2be9a71889d')

  def name(self) -> str:
    return 'TestPythonPlugin'

  def description(self) -> str:
    return 'Test'

  def vendor(self) -> str:
    return 'John Smith'

  def get_filters(self):
    return [CreateArrayFilter, InitializeDataPythonFilter, TemplateFilter]
