#include "WriteNodesAndElementsFiles.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/HexahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/SIMPLNXVersion.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
template <typename T>
void WriteValue(std::ofstream& file, const T& value)
{
  if constexpr(std::is_floating_point_v<T>)
  {
    // For floating-point numbers, use up to 4 decimal places
    file << std::fixed << std::setprecision(4) << value;
  }
  else
  {
    // For non-floating-point types, reset to default format with no precision
    file << std::defaultfloat << std::setprecision(0) << value;
  }
}

template <typename T>
Result<> WriteFile(const fs::path& outputFilePath, const DataArray<T>& array, bool includeArrayHeaders, std::vector<std::string_view> arrayHeaders, bool numberRows, bool includeComponentCount)
{
  std::ofstream file(outputFilePath.string());
  if(!file.is_open())
  {
    return MakeErrorResult(to_underlying(WriteNodesAndElementsFiles::ErrorCodes::FailedToOpenOutputFile), fmt::format("Failed to open output file \"{}\".", outputFilePath.string()));
  }

  file << fmt::format("# This file was created by simplnx v{}", Version::Complete()) << '\n';

  if(includeArrayHeaders)
  {
    WriteValue(file, string_utilities::Join(arrayHeaders, " "));
    file << '\n';
  }
  usize numComps = array.getNumberOfComponents();
  for(usize i = 0; i < array.getNumberOfTuples(); i++)
  {
    if(numberRows)
    {
      WriteValue(file, i);
      file << " ";
    }

    if(includeComponentCount)
    {
      WriteValue(file, numComps);
      file << " ";
    }

    for(usize j = 0; j < numComps; j++)
    {
      WriteValue(file, array[i * numComps + j]);
      if(j != numComps - 1)
      {
        file << " ";
      }
    }
    file << '\n';
  }

  return {};
}
} // namespace

// -----------------------------------------------------------------------------
WriteNodesAndElementsFiles::WriteNodesAndElementsFiles(DataStructure& dataStructure, const IFilter::MessageHandler& /*mesgHandler*/, const std::atomic_bool& shouldCancel,
                                                       WriteNodesAndElementsFilesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
{
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
const std::atomic_bool& WriteNodesAndElementsFiles::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> WriteNodesAndElementsFiles::operator()()
{
  auto& iNodeGeometry = m_DataStructure.getDataRefAs<INodeGeometry0D>(m_InputValues->selected_geometry_path);
  auto geomType = iNodeGeometry.getGeomType();
  UInt64ArrayType* cellsArrayPtr = nullptr;

  switch(geomType)
  {
  case IGeometry::Type::Edge: {
    auto& geom = m_DataStructure.getDataRefAs<EdgeGeom>(m_InputValues->selected_geometry_path);
    cellsArrayPtr = geom.getEdges();
    break;
  }
  case IGeometry::Type::Triangle: {
    auto& geom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->selected_geometry_path);
    cellsArrayPtr = geom.getFaces();
    break;
  }
  case IGeometry::Type::Quad: {
    auto& geom = m_DataStructure.getDataRefAs<QuadGeom>(m_InputValues->selected_geometry_path);
    cellsArrayPtr = geom.getFaces();
    break;
  }
  case IGeometry::Type::Tetrahedral: {
    auto& geom = m_DataStructure.getDataRefAs<TetrahedralGeom>(m_InputValues->selected_geometry_path);
    cellsArrayPtr = geom.getPolyhedra();
    break;
  }
  case IGeometry::Type::Hexahedral: {
    auto& geom = m_DataStructure.getDataRefAs<HexahedralGeom>(m_InputValues->selected_geometry_path);
    cellsArrayPtr = geom.getPolyhedra();
    break;
  }
  case IGeometry::Type::Vertex: {
    break;
  }
  case IGeometry::Type::Image:
    return MakeErrorResult(to_underlying(ErrorCodes::UnsupportedGeometryType), fmt::format("The Image geometry type is not supported by this filter.  Please choose another geometry."));
  case IGeometry::Type::RectGrid: {
    return MakeErrorResult(to_underlying(ErrorCodes::UnsupportedGeometryType), fmt::format("The Rectilinear Grid geometry type is not supported by this filter.  Please choose another geometry."));
  }
  }

  const Float32ArrayType& vertices = iNodeGeometry.getVerticesRef();

  if(m_InputValues->write_node_file)
  {
    std::vector<std::string> arrayHeaders;
    if(m_InputValues->number_nodes)
    {
      arrayHeaders.emplace_back("NODE_NUM");
    }
    arrayHeaders.insert(arrayHeaders.end(), {"X", "Y", "Z"});

    std::vector<std::string_view> arrayHeadersViews(arrayHeaders.size());
    std::transform(arrayHeaders.begin(), arrayHeaders.end(), arrayHeadersViews.begin(), [](const std::string& s) { return std::string_view(s); });
    auto result = WriteFile(m_InputValues->node_file_path, vertices, m_InputValues->include_node_file_header, arrayHeadersViews, m_InputValues->number_nodes, false);
    if(result.invalid())
    {
      return result;
    }
  }

  if(m_InputValues->write_element_file && geomType != IGeometry::Type::Vertex)
  {
    std::vector<std::string> arrayHeaders;
    if(m_InputValues->number_elements)
    {
      arrayHeaders.emplace_back("ELEMENT_NUM");
    }
    arrayHeaders.emplace_back("NUM_VERTS_IN_ELEMENT");
    for(usize i = 0; i < cellsArrayPtr->getNumberOfComponents(); i++)
    {
      std::string vertexHeader = fmt::format("V{}_Index", i);
      arrayHeaders.push_back(vertexHeader);
    }

    std::vector<std::string_view> arrayHeadersViews(arrayHeaders.size());
    std::transform(arrayHeaders.begin(), arrayHeaders.end(), arrayHeadersViews.begin(), [](const std::string& s) { return std::string_view(s); });
    auto result = WriteFile(m_InputValues->element_file_path, *cellsArrayPtr, m_InputValues->include_element_file_header, arrayHeadersViews, m_InputValues->number_elements, true);
    if(result.invalid())
    {
      return result;
    }
  }

  return {};
}
