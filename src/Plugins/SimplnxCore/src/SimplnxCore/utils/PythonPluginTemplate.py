"""
Insert documentation here.
"""

@PLUGIN_IMPORT_CODE@
# FILTER_INCLUDE_INSERT

import simplnx as nx

class @PLUGIN_NAME@:
  def __init__(self) -> None:
    pass

  def id(self) -> nx.Uuid:
    return nx.Uuid('@PLUGIN_UUID@')

  def name(self) -> str:
    return '@PLUGIN_NAME@'

  def description(self) -> str:
    return '@PLUGIN_SHORT_NAME@'

  def vendor(self) -> str:
    return '@PLUGIN_DESCRIPTION@'

  def get_filters(self):
    return [@PLUGIN_FILTER_LIST@] # FILTER_NAME_INSERT

