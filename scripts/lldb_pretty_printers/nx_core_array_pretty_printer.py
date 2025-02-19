import lldb

import simplnx_pretty_printer_utils as utils

def array_summary(valobj, internal_dict):
    return utils.array_summary(valobj, internal_dict)


def __lldb_init_module(debugger, internal_dict):
    """
    LLDB calls this function automatically when this script is imported.
    
    We register a summary for the templated type:
    'nx::core::Array<*, *>' — the wildcard syntax tells LLDB to match
    any T and Dimensions combination.
    
    Alternatively, you can use a regex to match all template instantiations
    of nx::core::Array if the wildcard approach does not work in your LLDB version:
        type summary add -x 'nx::core::Array<.*>' -F nx_core_array_pretty_printer.array_summary
    """
    debugger.HandleCommand(
       'type summary add -F nx_core_array_pretty_printer.array_summary -x "nx::core::Array<.*,.*>"'
       # 'type summary add -x nx::core::Array<.*> -F nx_core_array_pretty_printer.array_summary'
    )
    debugger.HandleCommand(
       # 'type summary add -F nx_core_array_pretty_printer.array_summary "nx::core::Array<*,*>"'
        'type summary add -x nx::core::SizeVec3 -F nx_core_array_pretty_printer.array_summary'
    )
    debugger.HandleCommand(
       # 'type summary add -F nx_core_array_pretty_printer.array_summary "nx::core::Array<*,*>"'
        'type summary add -x nx::core::FloatVec3 -F nx_core_array_pretty_printer.array_summary'
    )
    debugger.HandleCommand(
       # 'type summary add -F nx_core_array_pretty_printer.array_summary "nx::core::Array<*,*>"'
        'type summary add -x nx::core::IntVec3 -F nx_core_array_pretty_printer.array_summary'
    )

    debugger.HandleCommand(
       # 'type summary add -F nx_core_array_pretty_printer.array_summary "nx::core::Array<*,*>"'
        'type summary add -x nx::core::SizeVec4 -F nx_core_array_pretty_printer.array_summary'
    )
    debugger.HandleCommand(
       # 'type summary add -F nx_core_array_pretty_printer.array_summary "nx::core::Array<*,*>"'
        'type summary add -x nx::core::FloatVec4 -F nx_core_array_pretty_printer.array_summary'
    )
    debugger.HandleCommand(
       # 'type summary add -F nx_core_array_pretty_printer.array_summary "nx::core::Array<*,*>"'
        'type summary add -x nx::core::IntVec4 -F nx_core_array_pretty_printer.array_summary'
    )

    debugger.HandleCommand(
       # 'type summary add -F nx_core_array_pretty_printer.array_summary "nx::core::Array<*,*>"'
        'type summary add -x nx::core::Point3D -F nx_core_array_pretty_printer.array_summary'
    )
    debugger.HandleCommand(
       # 'type summary add -F nx_core_array_pretty_printer.array_summary "nx::core::Array<*,*>"'
        'type summary add -x nx::core::Point3Df -F nx_core_array_pretty_printer.array_summary'
    )
    debugger.HandleCommand(
       # 'type summary add -F nx_core_array_pretty_printer.array_summary "nx::core::Array<*,*>"'
        'type summary add -x nx::core::Point3Dd -F nx_core_array_pretty_printer.array_summary'
    )
