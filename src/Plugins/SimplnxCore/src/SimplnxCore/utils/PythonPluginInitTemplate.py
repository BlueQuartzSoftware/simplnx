from @PLUGIN_NAME@.Plugin import @PLUGIN_NAME@

@PLUGIN_IMPORT_CODE@
# FILTER_INCLUDE_INSERT

def get_plugin():
  return @PLUGIN_NAME@()

__all__ = [@PLUGIN_FILTER_LIST@] # FILTER_NAME_INSERT

