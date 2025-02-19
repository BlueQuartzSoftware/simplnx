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
