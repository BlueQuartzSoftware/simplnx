#include "ReadStlFile.hpp"

#include "SimplnxCore/utils/StlUtilities.hpp"

#include "simplnx/Common/Range.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/GeometryUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <cstdio>
#include <utility>

using namespace nx::core;

namespace
{
class StlFileSentinel
{
public:
  explicit StlFileSentinel(FILE* file)
  : m_File(file)
  {
  }
  ~StlFileSentinel()
  {
    std::ignore = std::fclose(m_File);
  }
  StlFileSentinel(const StlFileSentinel&) = delete;            // Copy Constructor Not Implemented
  StlFileSentinel(StlFileSentinel&&) = delete;                 // Move Constructor Not Implemented
  StlFileSentinel& operator=(const StlFileSentinel&) = delete; // Copy Assignment Not Implemented
  StlFileSentinel& operator=(StlFileSentinel&&) = delete;      // Move Assignment Not Implemented

private:
  FILE* m_File = nullptr;
};

bool IsMagicsFile(const std::string& stlHeaderStr)
{
  // Look for the tell-tale signs that the file was written from Magics Materialise
  // If the file was written by Magics as a "Color STL" file then the 2byte int
  // values between each triangle will be NON-Zero which will screw up the reading.
  // These NON-Zero value do NOT indicate a length but is some sort of color
  // value encoded into the file. Instead of being normal like everyone else and
  // using the STL spec they went off and did their own thing.

  static const std::string k_ColorHeader("COLOR=");
  static const std::string k_MaterialHeader("MATERIAL=");
  if(stlHeaderStr.find(k_ColorHeader) != std::string::npos && stlHeaderStr.find(k_MaterialHeader) != std::string::npos)
  {
    return true;
  }
  return false;
}

bool IsVxElementsFile(const std::string& stlHeader)
{
  // Look for the tell-tale signs that the file was written from VxElements Creaform
  // STL files do not honor the last 2 bytes of the 50 byte Triangle struct
  // as specified in the STL Binary File specification. If we detect this, then we
  // ignore the 2 bytes are anything meaningful.
  return nx::core::StringUtilities::contains(stlHeader, "VXelements");
}
} // End anonymous namespace

ReadStlFile::ReadStlFile(DataStructure& dataStructure, fs::path stlFilePath, const DataPath& geometryPath, const DataPath& faceGroupPath, const DataPath& faceNormalsDataPath, bool scaleOutput,
                         float32 scaleFactor, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler)
: m_DataStructure(dataStructure)
, m_FilePath(std::move(stlFilePath))
, m_GeometryDataPath(geometryPath)
, m_FaceGroupPath(faceGroupPath)
, m_FaceNormalsDataPath(faceNormalsDataPath)
, m_ScaleOutput(scaleOutput)
, m_ScaleFactor(scaleFactor)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ReadStlFile::~ReadStlFile() noexcept = default;

Result<> ReadStlFile::operator()()
{
  std::error_code errorCode;
  auto stlFileSize = std::filesystem::file_size(m_FilePath, errorCode);

  // Open File
  FILE* f = std::fopen(m_FilePath.string().c_str(), "rb");
  if(nullptr == f)
  {
    return MakeErrorResult(nx::core::StlConstants::k_ErrorOpeningFile, "Error opening STL file");
  }
  StlFileSentinel fileSentinel(f); // Will ensure that the file is closed when this method returns

  // Read Header
  std::array<char, nx::core::StlConstants::k_STL_HEADER_LENGTH> stlHeader = {0};
  int32_t triCount = 0;
  if(std::fread(stlHeader.data(), nx::core::StlConstants::k_STL_HEADER_LENGTH, 1, f) != 1)
  {
    return MakeErrorResult(nx::core::StlConstants::k_StlHeaderParseError, "Error reading first 8 bytes of STL header. This can't be good.");
  }

  // Look for the tell-tale signs that the file was written from Magics Materialise
  // If the file was written by Magics as a "Color STL" file then the 2byte int
  // values between each triangle will be NON-Zero which will screw up the reading.
  // This NON-Zero value does NOT indicate a length but is some sort of color
  // value encoded into the file. Instead of being normal like everyone else and
  // using the STL spec they went off and did their own thing.
  std::string stlHeaderStr(stlHeader.data(), nx::core::StlConstants::k_STL_HEADER_LENGTH);

  bool ignoreMetaSizeValue = (IsMagicsFile(stlHeaderStr) || IsVxElementsFile(stlHeaderStr) ? true : false);

  // Read the number of triangles in the file.
  if(std::fread(&triCount, sizeof(int32_t), 1, f) != 1)
  {
    return MakeErrorResult(nx::core::StlConstants::k_TriangleCountParseError, "Error reading number of triangles from file. This is bad.");
  }

  auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_GeometryDataPath);

  triangleGeom.resizeFaceList(triCount);
  triangleGeom.resizeVertexList(triCount * 3);

  using SharedTriList = AbstractDataStore<IGeometry::MeshIndexArrayType::value_type>;
  using SharedVertList = AbstractDataStore<IGeometry::SharedVertexList::value_type>;

  SharedTriList& triangles = triangleGeom.getFaces()->getDataStoreRef();
  SharedVertList& nodes = triangleGeom.getVertices()->getDataStoreRef();

  auto& faceNormalsStore = m_DataStructure.getDataAs<Float64Array>(m_FaceNormalsDataPath)->getDataStoreRef();

  // Read the triangles
  constexpr size_t k_StlElementCount = 12;
  std::array<float, k_StlElementCount> fileVert = {0.0F};
  uint16_t attr = 0;
  std::vector<uint8_t> triangleAttributeBuffer(std::numeric_limits<uint16_t>::max()); // Just allocate a buffer of max UINT16 elements

  fpos_t pos;
  auto start = std::chrono::steady_clock::now();
  int32_t progInt = 0;

  for(int32_t t = 0; t < triCount; ++t)
  {
    progInt = static_cast<float>(t) / static_cast<float>(triCount) * 100.0f;

    auto now = std::chrono::steady_clock::now();
    // Only send updates every 1 second
    if(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 1000)
    {
      std::string message = fmt::format("Reading {}% Complete", progInt);
      m_MessageHandler(nx::core::IFilter::ProgressMessage{nx::core::IFilter::Message::Type::Info, message, progInt});
      start = std::chrono::steady_clock::now();
    }
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
          stlFileSize, stlHeaderStr, triCount, t);
      return MakeErrorResult(nx::core::StlConstants::k_StlFileLengthError, msg);
    }

    // Read the Vertices and Normal (12 total float32 = 48 Bytes)
    size_t objsRead = std::fread(fileVert.data(), sizeof(float), k_StlElementCount, f); // Read the Triangle
    if(k_StlElementCount != objsRead)
    {
      std::string msg = fmt::format("Error reading Triangle '{}'. Object Count was {} and should have been {}", t, objsRead, k_StlElementCount);
      return MakeErrorResult(nx::core::StlConstants::k_TriangleParseError, msg);
    }
    // Read the Uint16 value that is supposed to represent the number of bytes following that are file/vendor specific metadata
    // Lots of writers/vendors do NOT set this properly which can cause problems.
    objsRead = std::fread(&attr, sizeof(uint16_t), 1, f); // Read the Triangle Attribute Data length
    if(objsRead != 1)
    {
      std::string msg = fmt::format("Error reading Number of attributes for triangle '{}'. uint16 count was {} and should have been 1", t, objsRead);
      return MakeErrorResult(nx::core::StlConstants::k_AttributeParseError, msg);
    }
    // If we are trying to follow along the STL Spec, skip the stated bytes unless
    // we detected known Vendors that do not write proper STL Files.
    if(attr > 0 && !ignoreMetaSizeValue)
    {
      std::ignore = std::fseek(f, static_cast<size_t>(attr), SEEK_CUR); // Skip past the Triangle Attribute data since we don't know how to read it anyway
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

  return GeometryUtilities::EliminateDuplicateNodes(triangleGeom, m_ScaleOutput ? std::optional<float32>(m_ScaleFactor) : std::nullopt);
  // The fileSentinel will ensure the FILE* is closed.
}
