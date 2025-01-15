import lldb

def array_summary(valobj, internal_dict):
    """
    Custom summary function for nx::core::Array<T, Dimensions>.
    Shows the elements of the private std::array<T, Dimensions> m_Array.
    """

    # 1) Retrieve the m_Array child.
    std_array_elements = valobj.GetChildMemberWithName("m_Array").GetChildMemberWithName("__elems_")

    # 2) For an std::array<T, N>, LLDB usually shows each element as a child.
    #    We'll iterate through them and collect their values/summaries.
    count = std_array_elements.GetNumChildren()

    elements = []
    for i in range(count):
        elem = std_array_elements.GetChildAtIndex(i)
        # Prefer GetValue() if it's a scalar; otherwise, fallback to GetSummary().
        elem_value = elem.GetValue()
        if elem_value is None:
            elem_value = elem.GetSummary()
        if elem_value is None:
            elem_value = "<unavailable>"
        elements.append(elem_value)

    # 4) Return a concise summary.
    return f"{', '.join(elements)}"

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
       'type summary add -F nx_core_array_pretty_printer.array_summary "nx::core::Array<*,*>"'
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
