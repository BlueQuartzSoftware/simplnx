#include "ReadStlFile.hpp"

#include "SimplnxCore/utils/StlUtilities.hpp"

#include "simplnx/Common/Range.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/GeometryUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <utility>

using namespace nx::core;

ReadStlFile::ReadStlFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadStlFileInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ReadStlFile::~ReadStlFile() noexcept = default;

Result<> ReadStlFile::operator()()
{
  const std::string stlFilePathStr = m_InputValues->stlFilePath.string();

  // Inspect the file before reading it. This parses the header and triangle count and, more
  // importantly, decides whether the per-triangle "attribute byte count" fields in this file are
  // real byte lengths or vendor garbage that must be ignored. See StlUtilities::SanityCheckFile().
  const StlConstants::StlFileCheck stlFileCheck = StlUtilities::SanityCheckFile(m_InputValues->stlFilePath);
  if(stlFileCheck.error != 0)
  {
    return MakeErrorResult(stlFileCheck.error, stlFileCheck.errorMessage);
  }
  const int32_t triCount = stlFileCheck.numTriangles;
  const uintmax_t stlFileSize = stlFileCheck.fileSize;

  // Open File
  FILE* f = std::fopen(stlFilePathStr.c_str(), "rb");
  if(nullptr == f)
  {
    return MakeErrorResult(StlConstants::k_ErrorOpeningFile, fmt::format("Error opening STL file '{}'", stlFilePathStr));
  }
  StlUtilities::StlFileSentinel fileSentinel(f); // Will ensure that the file is closed when this method returns

  // Skip past the 80 byte header and the triangle count. SanityCheckFile() already parsed both,
  // so there is no reason to read them a second time.
  if(std::fseek(f, static_cast<long>(StlConstants::k_StlFixedHeaderBytes), SEEK_SET) != 0)
  {
    return MakeErrorResult(StlConstants::k_StlHeaderParseError, fmt::format("Error seeking past the {} byte header of STL file '{}'", StlConstants::k_StlFixedHeaderBytes, stlFilePathStr));
  }

  auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->geometryPath);

  triangleGeom.resizeFaceList(triCount);
  triangleGeom.resizeVertexList(static_cast<usize>(triCount) * 3);

  using SharedTriList = AbstractDataStore<IGeometry::MeshIndexArrayType::value_type>;
  using SharedVertList = AbstractDataStore<IGeometry::SharedVertexList::value_type>;

  SharedTriList& triangles = triangleGeom.getFaces()->getDataStoreRef();
  SharedVertList& nodes = triangleGeom.getVertices()->getDataStoreRef();

  auto& faceNormalsStore = m_DataStructure.getDataAs<Float64Array>(m_InputValues->faceNormalsDataPath)->getDataStoreRef();

  // Read the triangles
  std::array<float, StlConstants::k_StlElementCount> fileVert = {0.0F};
  uint16_t attrByteCount = 0;

  MessageHelper messageHelper(m_MessageHandler);
  ThrottledMessenger throttledMessenger = messageHelper.createThrottledMessenger();

  fpos_t pos;

  for(int32_t t = 0; t < triCount; ++t)
  {
    throttledMessenger.sendThrottledMessage([&]() { return fmt::format("Reading {:.2f}% Complete", CalculatePercentComplete(t, triCount)); });
    if(m_ShouldCancel)
    {
      return {};
    }
    // Get the current File Position
    fgetpos(f, &pos);
#if defined(__APPLE__) || defined(_WIN32)
    if(pos >= stlFileSize)
#else
    if(pos.__pos >= stlFileSize)
#endif
    {
      std::string msg = fmt::format(
          "Trying to read at file position {} >= file size {}.\n  File Header: '{}'\n  Header Triangle Count: {}  Current Triangle: {}\n  The STL File does not conform to the STL file specification.",
#if defined(__APPLE__) || defined(_WIN32)
          pos,
#else
          pos.__pos,
#endif
          stlFileSize, stlFileCheck.header, triCount, t);
      return MakeErrorResult(StlConstants::k_StlFileLengthError, msg);
    }

    // Read the Vertices and Normal (12 total float32 = 48 Bytes)
    size_t objsRead = std::fread(fileVert.data(), sizeof(float), StlConstants::k_StlElementCount, f); // Read the Triangle
    if(StlConstants::k_StlElementCount != objsRead)
    {
      std::string msg = fmt::format("Error reading Triangle '{}' from STL file '{}'. Object Count was {} and should have been {}", t, stlFilePathStr, objsRead, StlConstants::k_StlElementCount);
      return MakeErrorResult(StlConstants::k_TriangleParseError, msg);
    }
    // Read the Uint16 value. This value is supposed to represent the number of bytes following
    // a triangle that are file- or vendor-specific metadata
    // Lots of writers/vendors do NOT set this properly, which can cause problems.
    objsRead = std::fread(&attrByteCount, sizeof(uint16_t), 1, f); // Read the Triangle Attribute Data length
    if(objsRead != 1)
    {
      std::string msg = fmt::format("Error reading Number of attributes for triangle '{}' from STL file '{}'. uint16 count was {} and should have been 1", t, stlFilePathStr, objsRead);
      return MakeErrorResult(StlConstants::k_AttributeParseError, msg);
    }
    // The file size told us this file really does carry attribute payload bytes, so the count is
    // an actual length and the payload has to be skipped. When the flag is false the count is
    // vendor garbage (a packed color, for example) and seeking on it would desynchronize the read.
    if(attrByteCount > 0 && stlFileCheck.attributePayloadPresent)
    {
      std::ignore = std::fseek(f, static_cast<long>(attrByteCount), SEEK_CUR); // Skip past the Triangle Attribute data since we don't know how to read it anyway
    }

    // Write the data into the actual geometry
    faceNormalsStore[3 * t + 0] = static_cast<double>(fileVert[0]);
    faceNormalsStore[3 * t + 1] = static_cast<double>(fileVert[1]);
    faceNormalsStore[3 * t + 2] = static_cast<double>(fileVert[2]);
    nodes[3 * (3 * t + 0) + 0] = fileVert[3];
    nodes[3 * (3 * t + 0) + 1] = fileVert[4];
    nodes[3 * (3 * t + 0) + 2] = fileVert[5];
    nodes[3 * (3 * t + 1) + 0] = fileVert[6];
    nodes[3 * (3 * t + 1) + 1] = fileVert[7];
    nodes[3 * (3 * t + 1) + 2] = fileVert[8];
    nodes[3 * (3 * t + 2) + 0] = fileVert[9];
    nodes[3 * (3 * t + 2) + 1] = fileVert[10];
    nodes[3 * (3 * t + 2) + 2] = fileVert[11];
    triangles[t * 3] = 3 * t + 0;
    triangles[t * 3 + 1] = 3 * t + 1;
    triangles[t * 3 + 2] = 3 * t + 2;
  }

  return GeometryUtilities::EliminateDuplicateNodes(triangleGeom, m_InputValues->scaleOutput ? std::optional<float32>(m_InputValues->scaleFactor) : std::nullopt);
  // The fileSentinel will ensure the FILE* is closed.
}
