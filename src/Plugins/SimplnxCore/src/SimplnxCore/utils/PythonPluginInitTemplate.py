
"""
Insert documentation here for #PLUGIN_NAME#
"""
from #PLUGIN_NAME#.Plugin import #PLUGIN_NAME#

__all__ = ['#PLUGIN_NAME#', 'get_plugin']

"""
This section conditionally tries to import each filter
"""

#PLUGIN_IMPORT_CODE#

def get_plugin():
  return #PLUGIN_NAME#()
