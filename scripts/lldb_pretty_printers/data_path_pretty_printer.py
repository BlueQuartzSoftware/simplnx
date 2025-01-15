import lldb

def data_path_summary(valobj, internal_dict):
    """
    Return a string summary for nx::core::DataPath.
    We'll display the contents of m_Path (std::vector<std::string>)
    as a list of unquoted string values.
    """
    path_valobj = valobj.GetChildMemberWithName('m_Path')

    # Use LLDB's synthetic children for std::vector to retrieve each element.
    count = path_valobj.GetNumChildren()
    elements = []
    for i in range(count):
        element = path_valobj.GetChildAtIndex(i)
        # For std::string, GetSummary() often returns the string in quotes, e.g. "\"Hello\""
        str_val = element.GetSummary()
        if str_val is None:
            str_val = "<NULL>"
        else:
            # Strip surrounding double quotes if present
            if len(str_val) >= 2 and str_val.startswith('"') and str_val.endswith('"'):
                str_val = str_val[1:-1]
        elements.append(str_val)

    # Join all the strings into a single display
    return f"\"{'/'.join(elements)}\""

def __lldb_init_module(debugger, internal_dict):
    """
    LLDB will call this function automatically when the script is imported.
    We register our pretty-printer (summary) for the C++ type nx::core::DataPath.
    """
    debugger.HandleCommand(
        'type summary add -F data_path_pretty_printer.data_path_summary "nx::core::DataPath"'
    )
