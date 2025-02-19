import lldb

import simplnx_pretty_printer_utils as utils


def image_geom_summary(valobj, internal_dict):
    """
    Return a string summary for nx::core::ImageGeom.
    """
    dims_valobj = valobj.GetChildMemberWithName('m_Dimensions')
    summary = f'Dims:{utils.array_summary(dims_valobj, internal_dict)}'

    origin_valobj = valobj.GetChildMemberWithName('m_Origin')
    summary += f' Origin:{utils.array_summary(origin_valobj, internal_dict)}'

    spacing_val_obj= valobj.GetChildMemberWithName('m_Spacing')
    summary += f' Spacing:{utils.array_summary(spacing_val_obj, internal_dict)}'

    return summary

def vertex_geom_summary(valobj, internal_dict):
    sbtype = valobj.GetType()
    obj_type = None
    if sbtype.IsPointerType():
        # Dereference the pointer to get an SBValue that represents the actual object
        obj_type = valobj.Dereference()
    elif sbtype.IsReferenceType():
        obj_type = valobj
    else:
        # Could be a class/struct/primitive, etc.
        return "Object (by value)"

    vert_result = obj_type.EvaluateExpression("getNumberOfVertices()")

    if vert_result.IsValid() and vert_result.GetError().Success():
        return f"Verts: {vert_result.GetValueAsUnsigned()}"
    else:
        return "nx::core::VertexGeom* COULD NOT GENERATE SUMMARY"

def edge_geom_summary(valobj, internal_dict):

    sbtype = valobj.GetType()
    obj_type = None
    if sbtype.IsPointerType():
        # Dereference the pointer to get an SBValue that represents the actual object
        obj_type = valobj.Dereference()
    elif sbtype.IsReferenceType():
        obj_type = valobj
    else:
        # Could be a class/struct/primitive, etc.
        return "Object (by value)"

    vert_result = obj_type.EvaluateExpression("getNumberOfVertices()")
    edge_result = obj_type.EvaluateExpression("getNumberOfCells()")

    if vert_result.IsValid() and vert_result.GetError().Success() and edge_result.IsValid() and edge_result.GetError().Success():
        return f"Verts: {vert_result.GetValueAsUnsigned()} Edges: {edge_result.GetValueAsUnsigned()}"
    else:
        return "nx::core::EdgeGeom* COULD NOT GENERATE SUMMARY"


def __lldb_init_module(debugger, internal_dict):
    """
    LLDB will call this function automatically when the script is imported.
    We register our pretty-printer (summary) for the C++ type nx::core::DataPath.
    """
    debugger.HandleCommand(
        'type summary add -F image_geom_pretty_printer.image_geom_summary "nx::core::ImageGeom"'
    )
    debugger.HandleCommand(
        'type summary add -F image_geom_pretty_printer.vertex_geom_summary "nx::core::VertexGeom"'
    )
    debugger.HandleCommand(
        'type summary add -F image_geom_pretty_printer.edge_geom_summary "nx::core::EdgeGeom"'
    )
