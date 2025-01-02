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
  if constexpr(std::is_floating_point<T>::value)
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

  file << fmt::format("# This file was created by simplnx v{}", Version::Complete()) << std::endl;

  if(includeArrayHeaders)
  {
    WriteValue(file, StringUtilities::join(arrayHeaders, " "));
    file << std::endl;
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
    file << std::endl;
  }

  return {};
}
} // namespace

// -----------------------------------------------------------------------------
WriteNodesAndElementsFiles::WriteNodesAndElementsFiles(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                       WriteNodesAndElementsFilesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
WriteNodesAndElementsFiles::~WriteNodesAndElementsFiles() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& WriteNodesAndElementsFiles::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
void WriteNodesAndElementsFiles::sendMessage(const std::string& message)
{
  m_MessageHandler(IFilter::Message::Type::Info, message);
}

// -----------------------------------------------------------------------------
Result<> WriteNodesAndElementsFiles::operator()()
{
  auto& iNodeGeometry = m_DataStructure.getDataRefAs<INodeGeometry0D>(m_InputValues->SelectedGeometryPath);
  auto geomType = iNodeGeometry.getGeomType();
  UInt64Array* cellsArray = nullptr;

  switch(geomType)
  {
  case IGeometry::Type::Edge: {
    auto& geom = m_DataStructure.getDataRefAs<EdgeGeom>(m_InputValues->SelectedGeometryPath);
    cellsArray = geom.getEdges();
    break;
  }
  case IGeometry::Type::Triangle: {
    auto& geom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->SelectedGeometryPath);
    cellsArray = geom.getFaces();
    break;
  }
  case IGeometry::Type::Quad: {
    auto& geom = m_DataStructure.getDataRefAs<QuadGeom>(m_InputValues->SelectedGeometryPath);
    cellsArray = geom.getFaces();
    break;
  }
  case IGeometry::Type::Tetrahedral: {
    auto& geom = m_DataStructure.getDataRefAs<TetrahedralGeom>(m_InputValues->SelectedGeometryPath);
    cellsArray = geom.getPolyhedra();
    break;
  }
  case IGeometry::Type::Hexahedral: {
    auto& geom = m_DataStructure.getDataRefAs<HexahedralGeom>(m_InputValues->SelectedGeometryPath);
    cellsArray = geom.getPolyhedra();
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

  const Float32Array& vertices = iNodeGeometry.getVerticesRef();

  if(m_InputValues->WriteNodeFile)
  {
    std::vector<std::string> arrayHeaders;
    if(m_InputValues->NumberNodes)
    {
      arrayHeaders.push_back("NODE_NUM");
    }
    arrayHeaders.insert(arrayHeaders.end(), {"X", "Y", "Z"});

    std::vector<std::string_view> arrayHeadersViews(arrayHeaders.size());
    std::transform(arrayHeaders.begin(), arrayHeaders.end(), arrayHeadersViews.begin(), [](const std::string& s) { return std::string_view(s); });
    auto result = WriteFile(m_InputValues->NodeFilePath, vertices, m_InputValues->IncludeNodeFileHeader, arrayHeadersViews, m_InputValues->NumberNodes, false);
    if(result.invalid())
    {
      return result;
    }
  }

  if(m_InputValues->WriteElementFile && geomType != IGeometry::Type::Vertex)
  {
    std::vector<std::string> arrayHeaders;
    if(m_InputValues->NumberElements)
    {
      arrayHeaders.push_back("ELEMENT_NUM");
    }
    arrayHeaders.push_back("NUM_VERTS_IN_ELEMENT");
    for(usize i = 0; i < cellsArray->getNumberOfComponents(); i++)
    {
      std::string vertexHeader = fmt::format("V{}_Index", i);
      arrayHeaders.push_back(vertexHeader);
    }

    std::vector<std::string_view> arrayHeadersViews(arrayHeaders.size());
    std::transform(arrayHeaders.begin(), arrayHeaders.end(), arrayHeadersViews.begin(), [](const std::string& s) { return std::string_view(s); });
    auto result = WriteFile(m_InputValues->ElementFilePath, *cellsArray, m_InputValues->IncludeElementFileHeader, arrayHeadersViews, m_InputValues->NumberElements, true);
    if(result.invalid())
    {
      return result;
    }
  }

  return {};
}
