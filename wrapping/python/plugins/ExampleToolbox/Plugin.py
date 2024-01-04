
"""
Insert documentation here.
"""

from ExampleToolbox.SomeFilter import SomeFilter
from ExampleToolbox.OtherFilter import OtherFilter
from ExampleToolbox.ThirdFilter import ThirdFilter


import simplnx as nx

class ExampleToolbox:
  def __init__(self) -> None:
    pass

  def id(self) -> nx.Uuid:
    return nx.Uuid('5d2c8bc4-2edc-438d-a5b1-6ecbe09ca5dd')

  def name(self) -> str:
    return 'ExampleToolbox'

  def description(self) -> str:
    return 'ExampleToolbox'

  def vendor(self) -> str:
    return 'Description'

  def get_filters(self):
    return [SomeFilter,OtherFilter,ThirdFilter]


