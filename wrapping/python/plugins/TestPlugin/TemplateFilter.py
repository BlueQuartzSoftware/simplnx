from typing import List
import simplnx as nx

class TemplateFilter:
  TEST_KEY = 'test'

  def uuid(self) -> nx.Uuid:
    return nx.Uuid('d3c671e9-12ac-4c20-9e89-f5015c574c8b')

  def human_name(self) -> str:
    return 'Template Filter (Python)'

  def class_name(self) -> str:
    return 'TemplateFilter'

  def name(self) -> str:
    return 'TemplateFilter'

  def default_tags(self) -> List[str]:
    return ['python']

  def clone(self):
    return TemplateFilter()

  def parameters(self) -> nx.Parameters:
    params = nx.Parameters()

    params.insert(nx.Float64Parameter(TemplateFilter.TEST_KEY, 'Test', '', 0.0))

    return params

  def preflight_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.PreflightResult:
    value: float = args[TemplateFilter.TEST_KEY]
    message_handler(nx.IFilter.Message(nx.IFilter.Message.Type.Info, f'Preflight: {value}'))
    return nx.IFilter.PreflightResult()

  def execute_impl(self, data_structure: nx.DataStructure, args: dict, message_handler: nx.IFilter.MessageHandler, should_cancel: nx.AtomicBoolProxy) -> nx.IFilter.ExecuteResult:
    value: float = args[TemplateFilter.TEST_KEY]
    message_handler(nx.IFilter.Message(nx.IFilter.Message.Type.Info, f'Execute: {value}'))
    return nx.Result()

