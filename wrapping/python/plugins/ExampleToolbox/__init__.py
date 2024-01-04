
from ExampleToolbox.Plugin import ExampleToolbox

from ExampleToolbox.SomeFilter import SomeFilter
from ExampleToolbox.OtherFilter import OtherFilter
from ExampleToolbox.ThirdFilter import ThirdFilter


def get_plugin():
  return ExampleToolbox()

__all__ = ['ExampleToolbox', 'SomeFilter', 'OtherFilter', 'ThirdFilter', 'get_plugin']

