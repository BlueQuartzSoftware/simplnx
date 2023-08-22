from typing import List
import complex as cx

class TemplateFilter:
  TEST_KEY = 'test'

  def uuid(self) -> cx.Uuid:
    return cx.Uuid('d3c671e9-12ac-4c20-9e89-f5015c574c8b')

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

  def parameters(self) -> cx.Parameters:
    params = cx.Parameters()

    params.insert(cx.Float64Parameter(TemplateFilter.TEST_KEY, 'Test', '', 0.0))

    return params

  def preflight_impl(self, data_structure: cx.DataStructure, args: dict, message_handler: cx.IFilter.MessageHandler, should_cancel: cx.AtomicBoolProxy) -> cx.IFilter.PreflightResult:
    value: float = args[TemplateFilter.TEST_KEY]
    message_handler(cx.IFilter.Message(cx.IFilter.Message.Type.Info, f'Preflight: {value}'))
    return cx.IFilter.PreflightResult()

  def execute_impl(self, data_structure: cx.DataStructure, args: dict, message_handler: cx.IFilter.MessageHandler, should_cancel: cx.AtomicBoolProxy) -> cx.IFilter.ExecuteResult:
    value: float = args[TemplateFilter.TEST_KEY]
    message_handler(cx.IFilter.Message(cx.IFilter.Message.Type.Info, f'Execute: {value}'))
    return cx.Result()
