#include "WriteStlFile.hpp"

// NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <filesystem>
#include <unordered_map>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
using TriStore = AbstractDataStore<IGeometry::MeshIndexArrayType::value_type>;
using VertexStore = AbstractDataStore<IGeometry::SharedVertexList::value_type>;

struct LimitBoundAtomicFileFactory;

/**
 * @struct LimitBoundAtomicFile
 * @brief Owns one destination and its numbered overflow AtomicFiles.
 *
 * Each AtomicFile protects its destination until commit. The list does not
 * provide one transaction across all overflow files.
 */
struct LimitBoundAtomicFile
{
  friend LimitBoundAtomicFileFactory;

public:
  LimitBoundAtomicFile() = delete;

  /**
   * @brief Adds one numbered overflow destination.
   * @return New list index or AtomicFile creation error.
   */
  Result<usize> createOverflowFile()
  {
    const fs::path newPath = fs::path(fmt::format("{}/{}_overflow_{}{}", m_InputPath.parent_path().string(), m_InputPath.stem().string(), m_AtomicFilesList.size(), m_InputPath.extension().string()));
    auto atomicFileResult = AtomicFile::Create(newPath);
    if(atomicFileResult.invalid())
    {
      return {{nonstd::make_unexpected(atomicFileResult.errors())}};
    }
    m_AtomicFilesList.emplace_back(std::move(atomicFileResult.value()));

    return {m_AtomicFilesList.size() - 1};
  }

  std::vector<AtomicFile> m_AtomicFilesList = {};

private:
  fs::path m_InputPath;

  LimitBoundAtomicFile(const fs::path& inputPath)
  : m_InputPath(inputPath)
  {
  }
};

/**
 * @struct LimitBoundAtomicFileFactory
 * @brief Creates a validated first file for an overflow sequence.
 */
struct LimitBoundAtomicFileFactory
{
  /**
   * @brief Creates one overflow-sequence owner.
   * @param inputPath Specifies the first destination.
   * @return Initialized owner or AtomicFile creation error.
   */
  static Result<LimitBoundAtomicFile> Create(const fs::path& inputPath)
  {
    LimitBoundAtomicFile outClass(inputPath);

    auto atomicFileResult = AtomicFile::Create(inputPath);
    if(atomicFileResult.invalid())
    {
      return {{nonstd::make_unexpected(atomicFileResult.errors())}};
    }
    outClass.m_AtomicFilesList.emplace_back(std::move(atomicFileResult.value()));

    return {std::move(outClass)};
  }
};

/**
 * @brief Writes one triangle range to a binary STL temporary file.
 * @param filter Receives thread-safe worker warnings.
 * @param path Identifies the temporary output.
 * @param endValue Specifies the exclusive last triangle.
 * @param header Specifies up to 80 header bytes.
 * @param triangles Provides flat triangle connectivity.
 * @param vertices Provides flat XYZ coordinates.
 * @param shouldCancel Stops before later triangles when true.
 * @param startValue Specifies the first triangle.
 * @return Header warning or success after completion or cancellation.
 *
 * Triangle write failures go to filter and can produce a truncated temporary file.
 * Most header, seek, count, and close results are not inspected.
 */
Result<> SingleWriteOutStl(WriteStlFile* filter, const fs::path& path, const IGeometry::MeshIndexType endValue, std::string header, const TriStore& triangles, const VertexStore& vertices,
                           const std::atomic_bool& shouldCancel, const IGeometry::MeshIndexType startValue = 0)
{
  Result<> result;

  // Binary mode prevents platform newline conversion.
  FILE* filePtr = fopen(path.string().c_str(), "wb");

  if(filePtr == nullptr)
  {
    return {MakeWarningVoidResult(-27886, fmt::format("Error Opening STL File. Unable to create temp file at path '{}' for original file '{}'", path.string(), path.filename().string()))};
  }

  int32 triCount = 0;

  {
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
    fwrite(stlFileHeader.data(), 1, 80, filePtr);
  }

  fwrite(&triCount, 1, 4, filePtr);
  triCount = 0;

  size_t totalWritten = 0;
  FloatVec3 vecA = {0.0f, 0.0f, 0.0f};
  FloatVec3 vecB = {0.0f, 0.0f, 0.0f};

  std::array<char, 50> data = {};
  nonstd::span<float32> normalPtr(reinterpret_cast<float32*>(data.data()), 3);
  nonstd::span<float32> vert1Ptr(reinterpret_cast<float32*>(data.data() + 12), 3);
  nonstd::span<float32> vert2Ptr(reinterpret_cast<float32*>(data.data() + 24), 3);
  nonstd::span<float32> vert3Ptr(reinterpret_cast<float32*>(data.data() + 36), 3);
  nonstd::span<uint16> attrByteCountPtr(reinterpret_cast<uint16*>(data.data() + 48), 2);
  attrByteCountPtr[0] = 0;

  for(IGeometry::MeshIndexType triangle = startValue; triangle < endValue; ++triangle)
  {
    if(shouldCancel)
    {
      fseek(filePtr, 80L, SEEK_SET);
      fwrite(reinterpret_cast<char*>(&triCount), 1, 4, filePtr);
      fclose(filePtr);
      return result;
    }

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

    vecA[0] = vert2Ptr[0] - vert1Ptr[0];
    vecA[1] = vert2Ptr[1] - vert1Ptr[1];
    vecA[2] = vert2Ptr[2] - vert1Ptr[2];

    vecB[0] = vert3Ptr[0] - vert1Ptr[0];
    vecB[1] = vert3Ptr[1] - vert1Ptr[1];
    vecB[2] = vert3Ptr[2] - vert1Ptr[2];

    auto temp = vecA.cross(vecB).normalize();
    normalPtr[0] = temp[0];
    normalPtr[1] = temp[1];
    normalPtr[2] = temp[2];

    totalWritten = fwrite(data.data(), 1, 50, filePtr);
    if(totalWritten != 50)
    {
      fclose(filePtr);
      filter->sendThreadSafeProgressMessage({MakeWarningVoidResult(
          -27873, fmt::format("Error Writing STL File '{}': Not enough bytes written for triangle {}. Only {} bytes written of 50 bytes", path.filename().string(), triCount, totalWritten))});
      break;
    }
    triCount++;
  }

  fseek(filePtr, 80L, SEEK_SET);
  fwrite(reinterpret_cast<char*>(&triCount), 1, 4, filePtr);
  fclose(filePtr);
  return result;
}

/**
 * @class SingleOutWrapper
 * @brief Adapts one single-file range to ParallelTaskAlgorithm.
 */
class SingleOutWrapper
{
public:
  /**
   * @brief Creates one borrowed range writer.
   * @param filter Receives worker warnings.
   * @param path Identifies the temporary output.
   * @param endValue Specifies the exclusive last triangle.
   * @param header Specifies the STL header.
   * @param triangles Provides flat triangle connectivity.
   * @param vertices Provides flat XYZ coordinates.
   * @param startValue Specifies the first triangle.
   * @param shouldCancel Stops before later triangles when true.
   */
  SingleOutWrapper(WriteStlFile* filter, const fs::path& path, const IGeometry::MeshIndexType endValue, std::string header, const TriStore& triangles, const VertexStore& vertices,
                   const IGeometry::MeshIndexType startValue, const std::atomic_bool& shouldCancel)
  : m_Filter(filter)
  , m_Path(path)
  , m_EndValue(endValue)
  , m_Header(header)
  , m_Triangles(triangles)
  , m_Vertices(vertices)
  , m_StartValue(startValue)
  , m_ShouldCancel(shouldCancel)
  {
  }
  /**
   * @brief Destroys the borrowed range writer.
   */
  ~SingleOutWrapper() = default;

  /**
   * @brief Writes the captured triangle range.
   */
  void operator()() const
  {
    SingleWriteOutStl(m_Filter, m_Path, m_EndValue, m_Header, m_Triangles, m_Vertices, m_ShouldCancel, m_StartValue);
  }

private:
  WriteStlFile* m_Filter = nullptr;
  const fs::path m_Path;
  const IGeometry::MeshIndexType m_EndValue;
  std::string m_Header;
  const TriStore& m_Triangles;
  const VertexStore& m_Vertices;
  const IGeometry::MeshIndexType m_StartValue;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @brief Buckets triangles by one or two per-triangle labels.
 * @param labels Provides feature IDs or part numbers through direct value access.
 * @return Each distinct label mapped to ascending triangle indexes.
 *
 * One pass replaces a label-by-triangle search. A triangle enters at most two
 * buckets, so resident bucket memory scales with triangle count.
 */
std::unordered_map<int32, std::vector<usize>> BuildTrianglesByLabel(const Int32AbstractDataStore& labels)
{
  const usize numComps = labels.getNumberOfComponents();
  const usize numTriangles = labels.getNumberOfTuples();

  std::unordered_map<int32, std::vector<usize>> trianglesByLabel;
  for(usize triangle = 0; triangle < numTriangles; triangle++)
  {
    const int32 labelA = labels[triangle * numComps];
    trianglesByLabel[labelA].push_back(triangle);
    if(numComps > 1)
    {
      const int32 labelB = labels[triangle * numComps + 1];
      // Do not add one triangle twice when both label components match.
      if(labelB != labelA)
      {
        trianglesByLabel[labelB].push_back(triangle);
      }
    }
  }
  return trianglesByLabel;
}

/**
 * @class MultiWriteStlFileImpl
 * @brief Writes one label group and its overflow files.
 *
 * The task visits only its pre-bucketed triangles. For a two-component label
 * array, component selection also determines output winding.
 */
class MultiWriteStlFileImpl
{
public:
  /**
   * @brief Creates one borrowed group writer.
   * @param filter Receives thread-safe warnings.
   * @param limitBoundAtomicFile Owns the destination sequence.
   * @param header Specifies the STL header.
   * @param triangles Provides flat triangle connectivity.
   * @param vertices Provides flat XYZ coordinates.
   * @param featureIds Provides one or two grouping labels per triangle.
   * @param featureId Specifies this task's label.
   * @param triangleIndices Specifies this task's ascending triangle indexes.
   * @param maxTriangles Limits triangles in one file.
   * @param shouldCancel Stops before later triangles when true.
   */
  MultiWriteStlFileImpl(WriteStlFile* filter, LimitBoundAtomicFile& limitBoundAtomicFile, const std::string header, const TriStore& triangles, const VertexStore& vertices,
                        const Int32AbstractDataStore& featureIds, const int32 featureId, const std::vector<usize>& triangleIndices, const usize maxTriangles, const std::atomic_bool& shouldCancel)
  : m_Filter(filter)
  , m_LimitBoundAtomicFile(limitBoundAtomicFile)
  , m_Header(header)
  , m_Triangles(triangles)
  , m_Vertices(vertices)
  , m_FeatureIds(featureIds)
  , m_FeatureId(featureId)
  , m_TriangleIndices(triangleIndices)
  , m_MaxTriangles(maxTriangles)
  , m_ShouldCancel(shouldCancel)
  {
  }
  /**
   * @brief Destroys the borrowed group writer.
   */
  ~MultiWriteStlFileImpl() = default;

  /**
   * @brief Starts writing at the first temporary file.
   */
  void operator()() const
  {
    // The factory guarantees a valid first AtomicFile.
    write(m_LimitBoundAtomicFile.m_AtomicFilesList[0].tempFilePath(), 0);
  }

  /**
   * @brief Writes one file and recurses into an overflow file when necessary.
   * @param activePath Identifies the active temporary file.
   * @param startIndex Specifies the first index in this task's triangle bucket.
   *
   * Worker failures are sent to the parent result. Stdio seek, header, count,
   * and close results are not inspected.
   */
  void write(const fs::path& activePath, usize startIndex) const
  {
    // Binary mode prevents platform newline conversion.
    FILE* filePtr = fopen(activePath.string().c_str(), "wb");

    if(filePtr == nullptr)
    {
      m_Filter->sendThreadSafeProgressMessage(
          {MakeWarningVoidResult(-27876, fmt::format("Error Opening STL File. Unable to create temp file at path '{}' for original file '{}'", activePath.string(), activePath.filename().string()))});
      return;
    }

    int32 triCount = 0;

    {
      if(m_Header.size() >= 80)
      {
        m_Filter->sendThreadSafeProgressMessage(MakeWarningVoidResult(
            -27874, fmt::format("Warning: Writing STL File '{}'. Header was over the 80 characters supported by STL. Length of header: {}. Only the first 80 bytes will be written.",
                                activePath.filename().string(), m_Header.length())));
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
      fwrite(stlFileHeader.data(), 1, 80, filePtr);
    }

    fwrite(&triCount, 1, 4, filePtr);
    triCount = 0;

    size_t totalWritten = 0;
    FloatVec3 vecA = {0.0f, 0.0f, 0.0f};
    FloatVec3 vecB = {0.0f, 0.0f, 0.0f};

    std::array<char, 50> data = {};
    nonstd::span<float32> normalPtr(reinterpret_cast<float32*>(data.data()), 3);
    nonstd::span<float32> vert1Ptr(reinterpret_cast<float32*>(data.data() + 12), 3);
    nonstd::span<float32> vert2Ptr(reinterpret_cast<float32*>(data.data() + 24), 3);
    nonstd::span<float32> vert3Ptr(reinterpret_cast<float32*>(data.data() + 36), 3);
    nonstd::span<uint16> attrByteCountPtr(reinterpret_cast<uint16*>(data.data() + 48), 2);
    attrByteCountPtr[0] = 0;

    const usize numComps = m_FeatureIds.getNumberOfComponents();
    const usize numGroupTriangles = m_TriangleIndices.size();
    for(usize idx = startIndex; idx < numGroupTriangles; idx++)
    {
      if(m_ShouldCancel)
      {
        fseek(filePtr, 80L, SEEK_SET);
        fwrite(reinterpret_cast<char*>(&triCount), 1, 4, filePtr);
        fclose(filePtr);
        return;
      }

      // Start an overflow file when this file reaches its triangle limit.
      if(triCount == m_MaxTriangles)
      {
        fseek(filePtr, 80L, SEEK_SET);
        fwrite(reinterpret_cast<char*>(&triCount), 1, 4, filePtr);
        fclose(filePtr);

        auto overflowFileResult = m_LimitBoundAtomicFile.createOverflowFile();
        if(overflowFileResult.invalid())
        {
          if(overflowFileResult.errors().empty())
          {
            m_Filter->sendThreadSafeProgressMessage({MakeWarningVoidResult(-27878, "Issue creating overflow file")});
            return;
          }
          m_Filter->sendThreadSafeProgressMessage({MakeWarningVoidResult(overflowFileResult.errors()[0].code, overflowFileResult.errors()[0].message)});
          return;
        }
        write(m_LimitBoundAtomicFile.m_AtomicFilesList[overflowFileResult.value()].tempFilePath(), idx);
        return;
      }

      const IGeometry::MeshIndexType triangle = m_TriangleIndices[idx];

      IGeometry::MeshIndexType nId0 = m_Triangles[triangle * 3];
      IGeometry::MeshIndexType nId1 = m_Triangles[triangle * 3 + 1];
      IGeometry::MeshIndexType nId2 = m_Triangles[triangle * 3 + 2];

      // Put this feature on the first label side by reversing the opposite side.
      if(m_FeatureIds[triangle * numComps] == m_FeatureId)
      {
        // winding = 0; // 0 = Write it using forward spin
      }
      else
      {
        // Reverse winding when the second component matches.
        IGeometry::MeshIndexType temp = nId1;
        nId1 = nId2;
        nId2 = temp;
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

      vecA[0] = vert2Ptr[0] - vert1Ptr[0];
      vecA[1] = vert2Ptr[1] - vert1Ptr[1];
      vecA[2] = vert2Ptr[2] - vert1Ptr[2];

      vecB[0] = vert3Ptr[0] - vert1Ptr[0];
      vecB[1] = vert3Ptr[1] - vert1Ptr[1];
      vecB[2] = vert3Ptr[2] - vert1Ptr[2];

      auto temp = vecA.cross(vecB).normalize();
      normalPtr[0] = temp[0];
      normalPtr[1] = temp[1];
      normalPtr[2] = temp[2];

      totalWritten = fwrite(data.data(), 1, 50, filePtr);
      if(totalWritten != 50)
      {
        fclose(filePtr);
        m_Filter->sendThreadSafeProgressMessage({MakeWarningVoidResult(
            -27873, fmt::format("Error Writing STL File '{}': Not enough bytes written for triangle {}. Only {} bytes written of 50 bytes", activePath.filename().string(), triCount, totalWritten))});
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
  LimitBoundAtomicFile& m_LimitBoundAtomicFile;
  const std::string m_Header;
  const TriStore& m_Triangles;
  const VertexStore& m_Vertices;
  const Int32AbstractDataStore& m_FeatureIds;
  const int32 m_FeatureId;
  const std::vector<usize>& m_TriangleIndices;
  const usize m_MaxTriangles;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @brief Writes one single-file sequence through parallel overflow tasks.
 * @param filter Receives thread-safe worker warnings.
 * @param nTriangles Specifies total triangles.
 * @param header Specifies the STL header.
 * @param firstFile Identifies the first destination.
 * @param triangles Provides flat triangle connectivity.
 * @param vertices Provides flat XYZ coordinates.
 * @param maxTriangles Limits triangles in one file.
 * @param shouldCancel Stops before later triangles or commits when true.
 * @return AtomicFile creation error, or success after cancellation or commits.
 *
 * Commit failures are accumulated locally but not returned. Commits are sequential,
 * so a later failure can leave earlier overflow files published.
 */
Result<> ExecuteSingleFileOverflow(WriteStlFile* filter, const IGeometry::MeshIndexType nTriangles, const std::string& header, const fs::path& firstFile, const TriStore& triangles,
                                   const VertexStore& vertices, const usize maxTriangles, const std::atomic_bool& shouldCancel)
{
  const usize count = nTriangles / maxTriangles;

  auto atomicFileResult = LimitBoundAtomicFileFactory::Create(firstFile);
  if(atomicFileResult.invalid())
  {
    return ConvertResult(std::move(atomicFileResult));
  }
  LimitBoundAtomicFile limitedFile(std::move(atomicFileResult.value()));

  for(usize i = 1; i < count + 1; i++)
  {
    auto overflowFileResult = limitedFile.createOverflowFile();
    if(overflowFileResult.invalid())
    {
      return ConvertResult(std::move(overflowFileResult));
    }
  }

  // Each task writes a separate temporary file.
  ParallelTaskAlgorithm taskRunner;
  taskRunner.setParallelizationEnabled(true);

  for(usize i = 0; i < limitedFile.m_AtomicFilesList.size(); i++)
  {
    const usize startValue = i * maxTriangles;
    usize endValue = (i + 1) * maxTriangles;
    if(endValue > nTriangles)
    {
      endValue = nTriangles;
    }
    taskRunner.execute(SingleOutWrapper(filter, limitedFile.m_AtomicFilesList[i].tempFilePath(), endValue, header, triangles, vertices, startValue, shouldCancel));
  }

  taskRunner.wait();

  if(shouldCancel)
  {
    return {};
  }

  Result<> endResult = {};
  for(auto& atomicFile : limitedFile.m_AtomicFilesList)
  {
    Result<> commitResult = atomicFile.commit();
    if(commitResult.invalid())
    {
      endResult = MergeResults(endResult, commitResult);
    }
  }

  return {};
}
} // namespace

WriteStlFile::WriteStlFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteStlFileInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

WriteStlFile::~WriteStlFile() noexcept = default;

const std::atomic_bool& WriteStlFile::getCancel()
{
  return m_ShouldCancel;
}

Result<> WriteStlFile::operator()()
{
  const auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleGeomPath);
  const ::VertexStore& vertices = triangleGeom.getVertices()->getDataStoreRef();
  const ::TriStore& triangles = triangleGeom.getFaces()->getDataStoreRef();
  const IGeometry::MeshIndexType nTriangles = triangleGeom.getNumberOfFaces();

  auto groupingType = static_cast<GroupingType>(m_InputValues->GroupingType);

  if(groupingType == GroupingType::SingleFile)
  {
    std::string header = "DREAM3D Generated For Triangle Geom";

    // Keep the combined binary STL header below its 80-byte limit.
    if(triangleGeom.getName().size() < 41)
    {
      header += " " + triangleGeom.getName();
    }

    if(triangleGeom.getNumberOfFaces() > m_InputValues->HIDDEN_MaxTrianglesPerFile)
    {
      return ::ExecuteSingleFileOverflow(this, nTriangles, header, m_InputValues->OutputStlFile, triangles, vertices, m_InputValues->HIDDEN_MaxTrianglesPerFile, m_ShouldCancel);
    }

    auto atomicFileResult = AtomicFile::Create(m_InputValues->OutputStlFile);
    if(atomicFileResult.invalid())
    {
      return ConvertResult(std::move(atomicFileResult));
    }
    AtomicFile atomicFile = std::move(atomicFileResult.value());
    {
      auto result = ::SingleWriteOutStl(this, atomicFile.tempFilePath(), nTriangles, header, triangles, vertices, m_ShouldCancel);
      if(result.invalid())
      {
        return result;
      }
      if(m_ShouldCancel)
      {
        return {};
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
  {
    // Create the output directory before creating grouped AtomicFiles.
    Result<> createDirectoriesResult = nx::core::CreateOutputDirectories(outputPath);
    if(createDirectoriesResult.invalid())
    {
      return createDirectoriesResult;
    }
  }

  // Each group task writes a separate temporary file sequence.
  ParallelTaskAlgorithm taskRunner;
  taskRunner.setParallelizationEnabled(true);

  // Keep every AtomicFile alive until all group tasks finish.
  std::vector<LimitBoundAtomicFile> fileList;

  if(groupingType == GroupingType::Features)
  {
    const auto& featureIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsPath)->getDataStoreRef();

    // Build all feature memberships once before per-feature writers start.
    const std::unordered_map<int32, std::vector<usize>> trianglesByFeature = ::BuildTrianglesByLabel(featureIds);

    fileList.reserve(trianglesByFeature.size());

    usize fileIndex = 0;
    for(const auto& [featureId, featureTriangles] : trianglesByFeature)
    {
      fs::path firstFile = m_InputValues->OutputStlDirectory / fmt::format("{}Feature_{}.stl", m_InputValues->OutputStlPrefix, featureId);
      auto atomicFileResult = LimitBoundAtomicFileFactory::Create(firstFile);
      if(atomicFileResult.invalid())
      {
        return ConvertResult(std::move(atomicFileResult));
      }
      fileList.emplace_back(std::move(atomicFileResult.value()));

      m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Writing STL for Feature Id {}", featureId));
      taskRunner.execute(MultiWriteStlFileImpl(this, fileList[fileIndex], {"DREAM3D Generated For Feature ID " + StringUtilities::number(featureId)}, triangles, vertices, featureIds, featureId,
                                               featureTriangles, m_InputValues->HIDDEN_MaxTrianglesPerFile, m_ShouldCancel));
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
    const auto& featureIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsPath)->getDataStoreRef();

    std::map<int32, int32> uniqueGrainIdToPhase;

    const auto& featurePhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeaturePhasesPath);
    for(IGeometry::MeshIndexType i = 0; i < nTriangles; i++)
    {
      uniqueGrainIdToPhase.emplace(featureIds[i * 2], featurePhases[i * 2]);
      uniqueGrainIdToPhase.emplace(featureIds[i * 2 + 1], featurePhases[i * 2 + 1]);
    }

    // Reuse the feature-group membership and winding rule for phase-qualified names.
    const std::unordered_map<int32, std::vector<usize>> trianglesByFeature = ::BuildTrianglesByLabel(featureIds);

    usize fileIndex = 0;
    for(const auto& [featureId, value] : uniqueGrainIdToPhase)
    {
      fs::path firstFile = m_InputValues->OutputStlDirectory / fmt::format("{}Ensemble_{}_Feature_{}.stl", m_InputValues->OutputStlPrefix, value, featureId);
      auto atomicFileResult = LimitBoundAtomicFileFactory::Create(firstFile);
      if(atomicFileResult.invalid())
      {
        return ConvertResult(std::move(atomicFileResult));
      }
      fileList.emplace_back(std::move(atomicFileResult.value()));

      m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Writing STL for Phase {} and Feature Id {}", value, featureId));
      taskRunner.execute(MultiWriteStlFileImpl(this, fileList[fileIndex], {"DREAM3D Generated For Feature ID " + StringUtilities::number(featureId) + " Phase " + StringUtilities::number(value)},
                                               triangles, vertices, featureIds, featureId, trianglesByFeature.at(featureId), m_InputValues->HIDDEN_MaxTrianglesPerFile, m_ShouldCancel));
      fileIndex++;
      if(m_HasErrors)
      {
        break;
      }
    }
    taskRunner.wait();
  }

  if(groupingType == GroupingType::PartNumber)
  {
    const auto& partNumbers = m_DataStructure.getDataAs<Int32Array>(m_InputValues->PartNumberPath)->getDataStoreRef();

    // Build all part-number memberships once before per-part writers start.
    const std::unordered_map<int32, std::vector<usize>> trianglesByPartNumber = ::BuildTrianglesByLabel(partNumbers);
    fileList.reserve(trianglesByPartNumber.size());

    usize fileIndex = 0;
    for(const auto& [currentPartNumber, partTriangles] : trianglesByPartNumber)
    {
      fs::path firstFile = m_InputValues->OutputStlDirectory / fmt::format("{}{}.stl", m_InputValues->OutputStlPrefix, currentPartNumber);
      auto atomicFileResult = LimitBoundAtomicFileFactory::Create(firstFile);
      if(atomicFileResult.invalid())
      {
        return ConvertResult(std::move(atomicFileResult));
      }
      fileList.emplace_back(std::move(atomicFileResult.value()));

      m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Writing STL for Part Number {}", currentPartNumber));
      taskRunner.execute(MultiWriteStlFileImpl(this, fileList[fileIndex], {"DREAM3D Generated For Part Number " + StringUtilities::number(currentPartNumber)}, triangles, vertices, partNumbers,
                                               currentPartNumber, partTriangles, m_InputValues->HIDDEN_MaxTrianglesPerFile, m_ShouldCancel));
      fileIndex++;
      if(m_HasErrors)
      {
        break;
      }
    }
    taskRunner.wait();
  }

  if(m_ShouldCancel)
  {
    return {};
  }

  // Publish each temporary file after all workers finish successfully.
  for(auto& limitedAtomicFile : fileList)
  {
    for(auto& atomicFile : limitedAtomicFile.m_AtomicFilesList)
    {
      Result<> commitResult = atomicFile.commit();
      if(commitResult.invalid())
      {
        m_Result = MergeResults(m_Result, commitResult);
      }
    }
  }

  return m_Result;
}

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
