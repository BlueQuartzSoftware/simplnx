#include "WriteStlFile.hpp"

// NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/Math/MatrixMath.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
using TriStore = AbstractDataStore<IGeometry::MeshIndexArrayType::value_type>;
using VertexStore = AbstractDataStore<IGeometry::SharedVertexList::value_type>;

Result<> SingleWriteOutStl(const fs::path& path, const IGeometry::MeshIndexType numTriangles, const std::string&& header, const TriStore& triangles, const VertexStore& vertices)
{
  Result<> result;

  // Create output file writer in binary write out mode to ensure cross-compatibility
  FILE* filePtr = fopen(path.string().c_str(), "wb");

  if(filePtr == nullptr)
  {
    fclose(filePtr);
    return {MakeWarningVoidResult(-27886, fmt::format("Error Opening STL File. Unable to create temp file at path '{}' for original file '{}'", path.string(), path.filename().string()))};
  }

  int32 triCount = 0;

  { // Scope header output processing to keep overhead low and increase readability
    if(header.size() >= 80)
    {
      result = MakeWarningVoidResult(-27884,
                                     fmt::format("Warning: Writing STL File '{}'. Header was over the 80 characters supported by STL. Length of header: {}. Only the first 80 bytes will be written.",
                                                 path.filename().string(), header.length()));
    }

    std::array<char, 80> stlFileHeader = {};
    stlFileHeader.fill(0);
    size_t headLength = 80;
    if(header.length() < 80)
    {
      headLength = static_cast<size_t>(header.length());
    }

    // std::string c_str = header;
    memcpy(stlFileHeader.data(), header.data(), headLength);
    // Return the number of bytes written - which should be 80
    fwrite(stlFileHeader.data(), 1, 80, filePtr);
  }

  fwrite(&triCount, 1, 4, filePtr);
  triCount = 0; // Reset this to Zero. Increment for every triangle written

  size_t totalWritten = 0;
  std::array<float, 3> vecA = {0.0f, 0.0f, 0.0f};
  std::array<float, 3> vecB = {0.0f, 0.0f, 0.0f};

  std::array<char, 50> data = {};
  nonstd::span<float32> normalPtr(reinterpret_cast<float32*>(data.data()), 3);
  nonstd::span<float32> vert1Ptr(reinterpret_cast<float32*>(data.data() + 12), 3);
  nonstd::span<float32> vert2Ptr(reinterpret_cast<float32*>(data.data() + 24), 3);
  nonstd::span<float32> vert3Ptr(reinterpret_cast<float32*>(data.data() + 36), 3);
  nonstd::span<uint16> attrByteCountPtr(reinterpret_cast<uint16*>(data.data() + 48), 2);
  attrByteCountPtr[0] = 0;

  // Loop over all the triangles for this spin
  for(IGeometry::MeshIndexType triangle = 0; triangle < numTriangles; ++triangle)
  {
    // Get the true indices of the 3 nodes
    IGeometry::MeshIndexType nId0 = triangles[triangle * 3];
    IGeometry::MeshIndexType nId1 = triangles[triangle * 3 + 1];
    IGeometry::MeshIndexType nId2 = triangles[triangle * 3 + 2];

    vert1Ptr[0] = static_cast<float>(vertices[nId0 * 3]);
    vert1Ptr[1] = static_cast<float>(vertices[nId0 * 3 + 1]);
    vert1Ptr[2] = static_cast<float>(vertices[nId0 * 3 + 2]);

    vert2Ptr[0] = static_cast<float>(vertices[nId1 * 3]);
    vert2Ptr[1] = static_cast<float>(vertices[nId1 * 3 + 1]);
    vert2Ptr[2] = static_cast<float>(vertices[nId1 * 3 + 2]);

    vert3Ptr[0] = static_cast<float>(vertices[nId2 * 3]);
    vert3Ptr[1] = static_cast<float>(vertices[nId2 * 3 + 1]);
    vert3Ptr[2] = static_cast<float>(vertices[nId2 * 3 + 2]);

    // Compute the normal
    vecA[0] = vert2Ptr[0] - vert1Ptr[0];
    vecA[1] = vert2Ptr[1] - vert1Ptr[1];
    vecA[2] = vert2Ptr[2] - vert1Ptr[2];

    vecB[0] = vert3Ptr[0] - vert1Ptr[0];
    vecB[1] = vert3Ptr[1] - vert1Ptr[1];
    vecB[2] = vert3Ptr[2] - vert1Ptr[2];

    MatrixMath::CrossProduct(vecA.data(), vecB.data(), normalPtr.data());
    MatrixMath::Normalize3x1(normalPtr.data());

    totalWritten = fwrite(data.data(), 1, 50, filePtr);
    if(totalWritten != 50)
    {
      fclose(filePtr);
      return {MakeWarningVoidResult(
          -27883, fmt::format("Error Writing STL File '{}'. Not enough elements written for Triangle {}. Wrote {} of 50. No file written.", path.filename().string(), triangle, totalWritten))};
    }
    triCount++;
  }

  fseek(filePtr, 80L, SEEK_SET);
  fwrite(reinterpret_cast<char*>(&triCount), 1, 4, filePtr);
  fclose(filePtr);
  return result;
}

/**
 * @brief This class provides an interface to write the STL Files in parallel
 */
class MultiWriteStlFileImpl
{
public:
  MultiWriteStlFileImpl(WriteStlFile* filter, const fs::path path, const IGeometry::MeshIndexType numTriangles, const std::string header, const TriStore& triangles, const VertexStore& vertices,
                        const Int32AbstractDataStore& featureIds, const int32 featureId)
  : m_Filter(filter)
  , m_Path(path)
  , m_NumTriangles(numTriangles)
  , m_Header(header)
  , m_Triangles(triangles)
  , m_Vertices(vertices)
  , m_FeatureIds(featureIds)
  , m_FeatureId(featureId)
  {
  }
  ~MultiWriteStlFileImpl() = default;

  void operator()() const
  {
    // Create output file writer in binary write out mode to ensure cross-compatibility
    FILE* filePtr = fopen(m_Path.string().c_str(), "wb");

    if(filePtr == nullptr)
    {
      fclose(filePtr);
      m_Filter->sendThreadSafeProgressMessage(
          {MakeWarningVoidResult(-27876, fmt::format("Error Opening STL File. Unable to create temp file at path '{}' for original file '{}'", m_Path.string(), m_Path.filename().string()))});
      return;
    }

    int32 triCount = 0;

    { // Scope header output processing to keep overhead low and increase readability
      if(m_Header.size() >= 80)
      {
        m_Filter->sendThreadSafeProgressMessage(MakeWarningVoidResult(
            -27874, fmt::format("Warning: Writing STL File '{}'. Header was over the 80 characters supported by STL. Length of header: {}. Only the first 80 bytes will be written.",
                                m_Path.filename().string(), m_Header.length())));
      }

      std::array<char, 80> stlFileHeader = {};
      stlFileHeader.fill(0);
      size_t headLength = 80;
      if(m_Header.length() < 80)
      {
        headLength = static_cast<size_t>(m_Header.length());
      }

      // std::string c_str = header;
      memcpy(stlFileHeader.data(), m_Header.data(), headLength);
      // Return the number of bytes written - which should be 80
      fwrite(stlFileHeader.data(), 1, 80, filePtr);
    }

    fwrite(&triCount, 1, 4, filePtr);
    triCount = 0; // Reset this to Zero. Increment for every triangle written

    size_t totalWritten = 0;
    std::array<float, 3> vecA = {0.0f, 0.0f, 0.0f};
    std::array<float, 3> vecB = {0.0f, 0.0f, 0.0f};

    std::array<char, 50> data = {};
    nonstd::span<float32> normalPtr(reinterpret_cast<float32*>(data.data()), 3);
    nonstd::span<float32> vert1Ptr(reinterpret_cast<float32*>(data.data() + 12), 3);
    nonstd::span<float32> vert2Ptr(reinterpret_cast<float32*>(data.data() + 24), 3);
    nonstd::span<float32> vert3Ptr(reinterpret_cast<float32*>(data.data() + 36), 3);
    nonstd::span<uint16> attrByteCountPtr(reinterpret_cast<uint16*>(data.data() + 48), 2);
    attrByteCountPtr[0] = 0;

    const usize numComps = m_FeatureIds.getNumberOfComponents();
    // Loop over all the triangles for this spin
    for(IGeometry::MeshIndexType triangle = 0; triangle < m_NumTriangles; ++triangle)
    {
      // Get the true indices of the 3 nodes
      IGeometry::MeshIndexType nId0 = m_Triangles[triangle * 3];
      IGeometry::MeshIndexType nId1 = m_Triangles[triangle * 3 + 1];
      IGeometry::MeshIndexType nId2 = m_Triangles[triangle * 3 + 2];

      if(m_FeatureIds[triangle * numComps] == m_FeatureId)
      {
        // winding = 0; // 0 = Write it using forward spin
      }
      else if(numComps > 1 && m_FeatureIds[triangle * numComps + 1] == m_FeatureId)
      {
        // Switch the 2 node indices
        IGeometry::MeshIndexType temp = nId1;
        nId1 = nId2;
        nId2 = temp;
      }
      else
      {
        continue; // We do not match either spin so move to the next triangle
      }

      vert1Ptr[0] = static_cast<float>(m_Vertices[nId0 * 3]);
      vert1Ptr[1] = static_cast<float>(m_Vertices[nId0 * 3 + 1]);
      vert1Ptr[2] = static_cast<float>(m_Vertices[nId0 * 3 + 2]);

      vert2Ptr[0] = static_cast<float>(m_Vertices[nId1 * 3]);
      vert2Ptr[1] = static_cast<float>(m_Vertices[nId1 * 3 + 1]);
      vert2Ptr[2] = static_cast<float>(m_Vertices[nId1 * 3 + 2]);

      vert3Ptr[0] = static_cast<float>(m_Vertices[nId2 * 3]);
      vert3Ptr[1] = static_cast<float>(m_Vertices[nId2 * 3 + 1]);
      vert3Ptr[2] = static_cast<float>(m_Vertices[nId2 * 3 + 2]);

      // Compute the normal
      vecA[0] = vert2Ptr[0] - vert1Ptr[0];
      vecA[1] = vert2Ptr[1] - vert1Ptr[1];
      vecA[2] = vert2Ptr[2] - vert1Ptr[2];

      vecB[0] = vert3Ptr[0] - vert1Ptr[0];
      vecB[1] = vert3Ptr[1] - vert1Ptr[1];
      vecB[2] = vert3Ptr[2] - vert1Ptr[2];

      MatrixMath::CrossProduct(vecA.data(), vecB.data(), normalPtr.data());
      MatrixMath::Normalize3x1(normalPtr.data());

      totalWritten = fwrite(data.data(), 1, 50, filePtr);
      if(totalWritten != 50)
      {
        fclose(filePtr);
        m_Filter->sendThreadSafeProgressMessage({MakeWarningVoidResult(
            -27873, fmt::format("Error Writing STL File '{}': Not enough bytes written for triangle {}. Only {} bytes written of 50 bytes", m_Path.filename().string(), triCount, totalWritten))});
        break;
      }
      triCount++;
    }

    fseek(filePtr, 80L, SEEK_SET);
    fwrite(reinterpret_cast<char*>(&triCount), 1, 4, filePtr);
    fclose(filePtr);
  }

private:
  WriteStlFile* m_Filter = nullptr;
  const fs::path m_Path;
  const IGeometry::MeshIndexType m_NumTriangles;
  const std::string m_Header;
  const TriStore& m_Triangles;
  const VertexStore& m_Vertices;
  const Int32AbstractDataStore& m_FeatureIds;
  const int32 m_FeatureId;
};
} // namespace

// -----------------------------------------------------------------------------
WriteStlFile::WriteStlFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteStlFileInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
WriteStlFile::~WriteStlFile() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& WriteStlFile::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> WriteStlFile::operator()()
{
  const auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleGeomPath);
  const ::VertexStore& vertices = triangleGeom.getVertices()->getDataStoreRef();
  const ::TriStore& triangles = triangleGeom.getFaces()->getDataStoreRef();
  const IGeometry::MeshIndexType nTriangles = triangleGeom.getNumberOfFaces();

  auto groupingType = static_cast<GroupingType>(m_InputValues->GroupingType);

  if(groupingType == GroupingType::SingleFile)
  {
    auto atomicFileResult = AtomicFile::Create(m_InputValues->OutputStlFile);
    if(atomicFileResult.invalid())
    {
      return ConvertResult(std::move(atomicFileResult));
    }
    AtomicFile atomicFile = std::move(atomicFileResult.value());

    {                                                             // Scoped to ensure file lock is released and header string is untouched since it is invalid after move
      std::string header = "DREAM3D Generated For Triangle Geom"; // Char count: 35

      // validate name is less than 40 characters
      if(triangleGeom.getName().size() < 41)
      {
        header += " " + triangleGeom.getName();
      }

      auto result = ::SingleWriteOutStl(atomicFile.tempFilePath(), nTriangles, std::move(header), triangles, vertices);
      if(result.invalid())
      {
        return result;
      }
    }

    Result<> commitResult = atomicFile.commit();
    if(commitResult.invalid())
    {
      return commitResult;
    }
    return {};
  }

  const std::filesystem::path outputPath = m_InputValues->OutputStlDirectory;
  { // Scope to cut overhead
    // Make sure any directory path is also available as the user may have just typed
    // in a path without actually creating the full path
    Result<> createDirectoriesResult = nx::core::CreateOutputDirectories(outputPath);
    if(createDirectoriesResult.invalid())
    {
      return createDirectoriesResult;
    }
  }

  // The writing of the files can happen in parallel as much as the Operating System will allow
  ParallelTaskAlgorithm taskRunner;
  taskRunner.setParallelizationEnabled(true);

  // Store a list of Atomic Files, so we can clean up or finish depending on the outcome of all the writes
  std::vector<Result<AtomicFile>> fileList;
  const auto& featureIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsPath)->getDataStoreRef();

  if(groupingType == GroupingType::Features)
  {
    // Faster and more memory efficient since we don't need phases
    std::unordered_set<int32> uniqueGrainIds(featureIds.cbegin(), featureIds.cend());

    fileList.reserve(uniqueGrainIds.size());

    usize fileIndex = 0;
    for(const auto featureId : uniqueGrainIds)
    {
      // Generate the output file
      fileList.push_back(AtomicFile::Create(m_InputValues->OutputStlDirectory / fmt::format("{}Feature_{}.stl", m_InputValues->OutputStlPrefix, featureId)));
      if(fileList[fileIndex].invalid())
      {
        return ConvertResult(std::move(fileList[fileIndex]));
      }
      m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Writing STL for Feature Id {}", featureId));
      taskRunner.execute(MultiWriteStlFileImpl(this, fileList[fileIndex].value().tempFilePath(), nTriangles, {"DREAM3D Generated For Feature ID " + StringUtilities::number(featureId)}, triangles,
                                               vertices, featureIds, featureId));
      fileIndex++;
      if(m_HasErrors)
      {
        break;
      }
    }
    taskRunner.wait();
  }

  if(groupingType == GroupingType::FeaturesAndPhases)
  {
    std::map<int32, int32> uniqueGrainIdToPhase;

    const auto& featurePhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeaturePhasesPath);
    for(IGeometry::MeshIndexType i = 0; i < nTriangles; i++)
    {
      uniqueGrainIdToPhase.emplace(featureIds[i * 2], featurePhases[i * 2]);
      uniqueGrainIdToPhase.emplace(featureIds[i * 2 + 1], featurePhases[i * 2 + 1]);
    }

    // Loop over the unique feature Ids
    usize fileIndex = 0;
    for(const auto& [featureId, value] : uniqueGrainIdToPhase)
    {
      // Generate the output file
      fileList.push_back(AtomicFile::Create(m_InputValues->OutputStlDirectory / fmt::format("{}Ensemble_{}_Feature_{}.stl", m_InputValues->OutputStlPrefix, value, featureId)));

      if(fileList[fileIndex].invalid())
      {
        return ConvertResult(std::move(fileList[fileIndex]));
      }
      m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Writing STL for Phase {} and Feature Id {}", value, featureId));
      taskRunner.execute(MultiWriteStlFileImpl(this, fileList[fileIndex].value().tempFilePath(), nTriangles,
                                               {"DREAM3D Generated For Feature ID " + StringUtilities::number(featureId) + " Phase " + StringUtilities::number(value)}, triangles, vertices, featureIds,
                                               featureId));
      fileIndex++;
      if(m_HasErrors)
      {
        break;
      }
    }
    taskRunner.wait();
  }

  // Group Triangles by Part Number which is a single component Int32 Array
  if(groupingType == GroupingType::PartNumber)
  {
    const auto& partNumbers = m_DataStructure.getDataAs<Int32Array>(m_InputValues->PartNumberPath)->getDataStoreRef();
    // Faster and more memory efficient since we don't need phases
    // Build up a list of the unique Part Numbers
    std::unordered_set<int32> uniquePartNumbers(partNumbers.cbegin(), partNumbers.cend());
    fileList.reserve(uniquePartNumbers.size()); // Reserved enough file names

    // Loop over each Part Number and write a file
    usize fileIndex = 0;
    for(const auto currentPartNumber : uniquePartNumbers)
    {
      // Generate the output file
      fileList.push_back(AtomicFile::Create(m_InputValues->OutputStlDirectory / fmt::format("{}{}.stl", m_InputValues->OutputStlPrefix, currentPartNumber)));
      if(fileList[fileIndex].invalid())
      {
        return ConvertResult(std::move(fileList[fileIndex]));
      }
      m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Writing STL for Part Number {}", currentPartNumber));
      taskRunner.execute(MultiWriteStlFileImpl(this, fileList[fileIndex].value().tempFilePath(), nTriangles, {"DREAM3D Generated For Part Number " + StringUtilities::number(currentPartNumber)},
                                               triangles, vertices, partNumbers, currentPartNumber));
      fileIndex++;
      if(m_HasErrors)
      {
        break;
      }
    }
    taskRunner.wait();
  }

  // Commit all the temp files
  for(auto& atomicFile : fileList)
  {
    Result<> commitResult = atomicFile.value().commit();
    if(commitResult.invalid())
    {
      m_Result = MergeResults(m_Result, commitResult);
    }
  }

  return m_Result;
}

// -----------------------------------------------------------------------------
void WriteStlFile::sendThreadSafeProgressMessage(Result<>&& result)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);
  if(result.invalid())
  {
    m_HasErrors = true;
    m_Result = MergeResults(m_Result, result);
  }
}

// NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
