"""
Insert documentation here.
"""

_filters = []

"""
This section conditionally tries to import each filter
"""

#PLUGIN_IMPORT_CODE#

import simplnx as nx

class #PLUGIN_NAME#:
  """
  This class defines the plugin's basic information. 
  """
  def __init__(self) -> None:
    pass

  def id(self) -> nx.Uuid:
    """This returns the UUID of the filter. Each Plugin has a unique UUID value. DO NOT change this.
    :return: The Plugins's Uuid value
    :rtype: string
    """
    return nx.Uuid('#PLUGIN_UUID#')

  def name(self) -> str:
    """The returns the name of plugin. DO NOT Change this
    :return: The name of the plugin
    :rtype: string
    """    
    return '#PLUGIN_NAME#'

  def description(self) -> str:
    """This returns the description of the plugin. Feel free to edit this.
    :return: The plugin's descriptive text
    :rtype: string
    """    
    return '#PLUGIN_SHORT_NAME#'

  def vendor(self) -> str:
    """This returns the name of the organization that is writing the plugin. Feel free to edit this.
    :return: The plugin's organization
    :rtype: string
    """
    return '#PLUGIN_DESCRIPTION#'

  def get_filters(self):
    return _filters

