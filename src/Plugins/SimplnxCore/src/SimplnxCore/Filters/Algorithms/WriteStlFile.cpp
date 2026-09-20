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

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <iterator>
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
 * @brief Rewrites the final triangle count into the file header and closes the file.
 * @param filePtr Owns the open binary STL file. This function always closes it.
 * @param path Identifies the file in error messages.
 * @param triCount Specifies how many triangle records were written.
 * @return The first seek, write, or close error, or success.
 *
 * A binary STL is only readable when the 4 byte count at offset 80 matches the number
 * of triangle records, so each of these failures leaves an unusable file that must not
 * be committed over the destination.
 */
[[nodiscard]] Result<> FinalizeStlFile(FILE* filePtr, const fs::path& path, int32 triCount)
{
  if(fseek(filePtr, 80L, SEEK_SET) != 0)
  {
    // Capture errno before fclose or message formatting can overwrite it.
    const int savedErrno = errno;
    fclose(filePtr);
    return MakeErrorResult(-27882, fmt::format("Error writing STL file '{}': unable to seek to the triangle count at byte offset 80. Cause: {}", path.string(), std::strerror(savedErrno)));
  }
  if(fwrite(reinterpret_cast<char*>(&triCount), 1, 4, filePtr) != 4)
  {
    // Capture errno before fclose or message formatting can overwrite it.
    const int savedErrno = errno;
    fclose(filePtr);
    return MakeErrorResult(-27883, fmt::format("Error writing STL file '{}': unable to write the triangle count {} at byte offset 80. Cause: {}", path.string(), triCount, std::strerror(savedErrno)));
  }
  if(fclose(filePtr) != 0)
  {
    // Capture errno before message formatting can overwrite it.
    const int savedErrno = errno;
    return MakeErrorResult(-27887, fmt::format("Error writing STL file '{}': unable to flush and close the file after {} triangles. Cause: {}", path.string(), triCount, std::strerror(savedErrno)));
  }
  return {};
}

/**
 * @brief Writes one triangle range to a binary STL temporary file.
 * @param path Identifies the temporary output.
 * @param endValue Specifies the exclusive last triangle.
 * @param header Specifies up to 80 header bytes.
 * @param triangles Provides flat triangle connectivity.
 * @param vertices Provides flat XYZ coordinates.
 * @param shouldCancel Stops before later triangles when true.
 * @param startValue Specifies the first triangle.
 * @return The first open, write, seek, or close error, or success carrying any header warning.
 *
 * Every stdio failure becomes an error, because a truncated temporary file must never
 * reach the destination through a commit.
 */
[[nodiscard]] Result<> SingleWriteOutStl(const fs::path& path, const IGeometry::MeshIndexType endValue, std::string header, const TriStore& triangles, const VertexStore& vertices,
                                         const std::atomic_bool& shouldCancel, const IGeometry::MeshIndexType startValue = 0)
{
  Result<> result;

  // Binary mode prevents platform newline conversion.
  FILE* filePtr = fopen(path.string().c_str(), "wb");

  if(filePtr == nullptr)
  {
    // Capture errno before message formatting can overwrite it.
    const int savedErrno = errno;
    return MakeErrorResult(
        -27886, fmt::format("Error opening STL file: unable to create the temporary file '{}' for output file '{}'. Cause: {}", path.string(), path.filename().string(), std::strerror(savedErrno)));
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

    memcpy(stlFileHeader.data(), header.data(), headLength);
    if(fwrite(stlFileHeader.data(), 1, 80, filePtr) != 80)
    {
      // Capture errno before fclose or message formatting can overwrite it.
      const int savedErrno = errno;
      fclose(filePtr);
      return MakeErrorResult(-27880, fmt::format("Error writing STL file '{}': unable to write the 80 byte header. Cause: {}", path.string(), std::strerror(savedErrno)));
    }
  }

  if(fwrite(&triCount, 1, 4, filePtr) != 4)
  {
    // Capture errno before fclose or message formatting can overwrite it.
    const int savedErrno = errno;
    fclose(filePtr);
    return MakeErrorResult(-27881, fmt::format("Error writing STL file '{}': unable to write the 4 byte triangle count placeholder. Cause: {}", path.string(), std::strerror(savedErrno)));
  }
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
      // The header warning collected above must survive any finalize error.
      return MergeResults(std::move(result), FinalizeStlFile(filePtr, path, triCount));
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
      // Capture errno before fclose or message formatting can overwrite it.
      const int savedErrno = errno;
      fclose(filePtr);
      return MakeErrorResult(
          -27885, fmt::format("Error writing STL file '{}': only {} of the 50 bytes for triangle {} were written. Cause: {}", path.string(), totalWritten, triCount, std::strerror(savedErrno)));
    }
    triCount++;
  }

  // The header warning collected above must survive any finalize error.
  return MergeResults(std::move(result), FinalizeStlFile(filePtr, path, triCount));
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
   * @param filter Receives the worker result.
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
   * @brief Writes the captured triangle range and latches its result in the filter.
   *
   * ParallelTaskAlgorithm cannot return a value, so the result travels through the
   * filter's first-error holder instead.
   */
  void operator()() const
  {
    m_Filter->sendThreadSafeProgressMessage(SingleWriteOutStl(m_Path, m_EndValue, m_Header, m_Triangles, m_Vertices, m_ShouldCancel, m_StartValue));
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
   * @param filter Receives the worker result.
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
   * @brief Starts writing at the first temporary file and latches the result in the filter.
   *
   * ParallelTaskAlgorithm cannot return a value, so the result travels through the
   * filter's first-error holder instead.
   */
  void operator()() const
  {
    // The factory guarantees a valid first AtomicFile.
    m_Filter->sendThreadSafeProgressMessage(write(m_LimitBoundAtomicFile.m_AtomicFilesList[0].tempFilePath(), 0));
  }

  /**
   * @brief Writes one file and recurses into an overflow file when necessary.
   * @param activePath Identifies the active temporary file.
   * @param startIndex Specifies the first index in this task's triangle bucket.
   * @return The first open, write, seek, close, or overflow-creation error, or success
   * carrying every header warning from this file and its overflow files.
   *
   * Every stdio failure becomes an error, because a truncated temporary file must never
   * reach the destination through a commit.
   */
  [[nodiscard]] Result<> write(const fs::path& activePath, usize startIndex) const
  {
    Result<> result;

    // Binary mode prevents platform newline conversion.
    FILE* filePtr = fopen(activePath.string().c_str(), "wb");

    if(filePtr == nullptr)
    {
      // Capture errno before message formatting can overwrite it.
      const int savedErrno = errno;
      return MakeErrorResult(-27876, fmt::format("Error opening STL file: unable to create the temporary file '{}' for output file '{}'. Cause: {}", activePath.string(),
                                                 activePath.filename().string(), std::strerror(savedErrno)));
    }

    int32 triCount = 0;

    {
      if(m_Header.size() >= 80)
      {
        result = MergeResults(std::move(result),
                              MakeWarningVoidResult(-27874, fmt::format("Warning: Writing STL File '{}'. Header was over the 80 characters supported by STL. Length of header: {}. Only the "
                                                                        "first 80 bytes will be written.",
                                                                        activePath.filename().string(), m_Header.length())));
      }

      std::array<char, 80> stlFileHeader = {};
      stlFileHeader.fill(0);
      size_t headLength = 80;
      if(m_Header.length() < 80)
      {
        headLength = static_cast<size_t>(m_Header.length());
      }

      memcpy(stlFileHeader.data(), m_Header.data(), headLength);
      if(fwrite(stlFileHeader.data(), 1, 80, filePtr) != 80)
      {
        // Capture errno before fclose or message formatting can overwrite it.
        const int savedErrno = errno;
        fclose(filePtr);
        return MakeErrorResult(-27880, fmt::format("Error writing STL file '{}': unable to write the 80 byte header. Cause: {}", activePath.string(), std::strerror(savedErrno)));
      }
    }

    if(fwrite(&triCount, 1, 4, filePtr) != 4)
    {
      // Capture errno before fclose or message formatting can overwrite it.
      const int savedErrno = errno;
      fclose(filePtr);
      return MakeErrorResult(-27881, fmt::format("Error writing STL file '{}': unable to write the 4 byte triangle count placeholder. Cause: {}", activePath.string(), std::strerror(savedErrno)));
    }
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
        // The header warning collected above must survive any finalize error.
        return MergeResults(std::move(result), FinalizeStlFile(filePtr, activePath, triCount));
      }

      // Start an overflow file when this file reaches its triangle limit.
      if(triCount == m_MaxTriangles)
      {
        Result<> finalizeResult = FinalizeStlFile(filePtr, activePath, triCount);
        if(finalizeResult.invalid())
        {
          // The header warning collected above must survive any finalize error.
          return MergeResults(std::move(result), std::move(finalizeResult));
        }

        auto overflowFileResult = m_LimitBoundAtomicFile.createOverflowFile();
        if(overflowFileResult.invalid())
        {
          if(overflowFileResult.errors().empty())
          {
            return MergeResults(
                std::move(result),
                MakeErrorResult(-27878, fmt::format("Error creating the overflow STL file that follows '{}': the AtomicFile factory reported a failure without a cause.", activePath.string())));
          }
          return MergeResults(std::move(result), ConvertResult(std::move(overflowFileResult)));
        }
        return MergeResults(std::move(result), write(m_LimitBoundAtomicFile.m_AtomicFilesList[overflowFileResult.value()].tempFilePath(), idx));
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
        // Capture errno before fclose or message formatting can overwrite it.
        const int savedErrno = errno;
        fclose(filePtr);
        return MakeErrorResult(-27885, fmt::format("Error writing STL file '{}': only {} of the 50 bytes for triangle {} were written. Cause: {}", activePath.string(), totalWritten, triCount,
                                                   std::strerror(savedErrno)));
      }
      triCount++;
    }

    // The header warning collected above must survive any finalize error.
    return MergeResults(std::move(result), FinalizeStlFile(filePtr, activePath, triCount));
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
 * @param filter Holds the first worker error and every worker warning.
 * @param nTriangles Specifies total triangles.
 * @param header Specifies the STL header.
 * @param firstFile Identifies the first destination.
 * @param triangles Provides flat triangle connectivity.
 * @param vertices Provides flat XYZ coordinates.
 * @param maxTriangles Limits triangles in one file.
 * @param shouldCancel Stops before later triangles or commits when true.
 * @return The first AtomicFile creation, worker, cancellation, or commit error, or success
 * carrying the worker warnings after all commits.
 *
 * Commits are sequential, so a later failure can leave earlier overflow files
 * published.
 */
[[nodiscard]] Result<> ExecuteSingleFileOverflow(WriteStlFile* filter, const IGeometry::MeshIndexType nTriangles, const std::string& header, const fs::path& firstFile, const TriStore& triangles,
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

  // No worker runs past wait(), so the latched result can be read without the mutex
  // contention of another task. A truncated temporary file must never be committed.
  Result<> workerResult = filter->getWorkerResult();
  if(workerResult.invalid())
  {
    return workerResult;
  }

  if(shouldCancel)
  {
    // The warnings latched by the workers must still reach the user.
    return MergeResults(std::move(workerResult), MakeErrorResult(-1, "Filter cancelled"));
  }

  for(auto& atomicFile : limitedFile.m_AtomicFilesList)
  {
    if(shouldCancel)
    {
      // The warnings latched by the workers must still reach the user.
      return MergeResults(std::move(workerResult), MakeErrorResult(-1, "Filter cancelled"));
    }
    Result<> commitResult = atomicFile.commit();
    if(commitResult.invalid())
    {
      // The warnings latched by the workers must still reach the user.
      return MergeResults(std::move(workerResult), std::move(commitResult));
    }
  }

  return workerResult;
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
    Result<> writeResult = ::SingleWriteOutStl(atomicFile.tempFilePath(), nTriangles, header, triangles, vertices, m_ShouldCancel);
    if(writeResult.invalid())
    {
      return writeResult;
    }

    if(m_ShouldCancel)
    {
      // The header warning collected by the write must still reach the user.
      return MergeResults(std::move(writeResult), MakeErrorResult(-1, "Filter cancelled"));
    }
    Result<> commitResult = atomicFile.commit();
    if(commitResult.invalid())
    {
      // The header warning collected by the write must still reach the user.
      return MergeResults(std::move(writeResult), std::move(commitResult));
    }
    return writeResult;
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

  // Keep every AtomicFile alive until all group tasks finish. Declared before the task
  // runner so the runner's destructor joins every worker while these files are still alive.
  std::vector<LimitBoundAtomicFile> fileList;

  // Each group task writes a separate temporary file sequence.
  ParallelTaskAlgorithm taskRunner;
  taskRunner.setParallelizationEnabled(true);

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
        // Workers already running hold references to the block-local membership map, so join them before it goes out of scope.
        taskRunner.wait();
        // The warnings latched by the workers that already finished must still reach the user.
        return MergeResults(getWorkerResult(), ConvertResult(std::move(atomicFileResult)));
      }
      fileList.emplace_back(std::move(atomicFileResult.value()));

      m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Writing STL for Feature Id {}", featureId));
      taskRunner.execute(MultiWriteStlFileImpl(this, fileList[fileIndex], {"DREAM3D Generated For Feature ID " + StringUtilities::number(featureId)}, triangles, vertices, featureIds, featureId,
                                               featureTriangles, m_InputValues->HIDDEN_MaxTrianglesPerFile, m_ShouldCancel));
      fileIndex++;
      if(m_HasErrors.load())
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

    // Workers hold references into fileList, so it must never reallocate while tasks run.
    fileList.reserve(uniqueGrainIdToPhase.size());

    usize fileIndex = 0;
    for(const auto& [featureId, value] : uniqueGrainIdToPhase)
    {
      fs::path firstFile = m_InputValues->OutputStlDirectory / fmt::format("{}Ensemble_{}_Feature_{}.stl", m_InputValues->OutputStlPrefix, value, featureId);
      auto atomicFileResult = LimitBoundAtomicFileFactory::Create(firstFile);
      if(atomicFileResult.invalid())
      {
        // Workers already running hold references to the block-local membership map, so join them before it goes out of scope.
        taskRunner.wait();
        // The warnings latched by the workers that already finished must still reach the user.
        return MergeResults(getWorkerResult(), ConvertResult(std::move(atomicFileResult)));
      }
      fileList.emplace_back(std::move(atomicFileResult.value()));

      m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Writing STL for Phase {} and Feature Id {}", value, featureId));
      taskRunner.execute(MultiWriteStlFileImpl(this, fileList[fileIndex], {"DREAM3D Generated For Feature ID " + StringUtilities::number(featureId) + " Phase " + StringUtilities::number(value)},
                                               triangles, vertices, featureIds, featureId, trianglesByFeature.at(featureId), m_InputValues->HIDDEN_MaxTrianglesPerFile, m_ShouldCancel));
      fileIndex++;
      if(m_HasErrors.load())
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
        // Workers already running hold references to the block-local membership map, so join them before it goes out of scope.
        taskRunner.wait();
        // The warnings latched by the workers that already finished must still reach the user.
        return MergeResults(getWorkerResult(), ConvertResult(std::move(atomicFileResult)));
      }
      fileList.emplace_back(std::move(atomicFileResult.value()));

      m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Writing STL for Part Number {}", currentPartNumber));
      taskRunner.execute(MultiWriteStlFileImpl(this, fileList[fileIndex], {"DREAM3D Generated For Part Number " + StringUtilities::number(currentPartNumber)}, triangles, vertices, partNumbers,
                                               currentPartNumber, partTriangles, m_InputValues->HIDDEN_MaxTrianglesPerFile, m_ShouldCancel));
      fileIndex++;
      if(m_HasErrors.load())
      {
        break;
      }
    }
    taskRunner.wait();
  }

  // No worker runs past the taskRunner.wait() calls above, so the latched result is
  // stable here. A truncated temporary file must never be committed.
  if(m_Result.invalid())
  {
    return m_Result;
  }

  if(m_ShouldCancel)
  {
    // The warnings latched by the workers must still reach the user.
    return MergeResults(getWorkerResult(), MakeErrorResult(-1, "Filter cancelled"));
  }

  // Publish each temporary file after all workers finish successfully.
  for(auto& limitedAtomicFile : fileList)
  {
    for(auto& atomicFile : limitedAtomicFile.m_AtomicFilesList)
    {
      if(m_ShouldCancel)
      {
        // The warnings latched by the workers must still reach the user.
        return MergeResults(getWorkerResult(), MakeErrorResult(-1, "Filter cancelled"));
      }
      Result<> commitResult = atomicFile.commit();
      if(commitResult.invalid())
      {
        // The warnings latched by the workers must still reach the user.
        return MergeResults(getWorkerResult(), std::move(commitResult));
      }
    }
  }

  return m_Result;
}

void WriteStlFile::sendThreadSafeProgressMessage(Result<>&& result)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);

  // Every worker warning reaches the user, including warnings that arrive after the
  // first error.
  WarningCollection& warnings = m_Result.warnings();
  warnings.insert(warnings.end(), std::make_move_iterator(result.warnings().begin()), std::make_move_iterator(result.warnings().end()));

  if(result.invalid() && !m_HasErrors.load())
  {
    // The first error wins, so a later worker cannot bury the original cause.
    m_HasErrors.store(true);
    m_Result = MergeResults(std::move(m_Result), Result<>{{nonstd::make_unexpected(std::move(result.errors()))}});
  }
}

Result<> WriteStlFile::getWorkerResult() const
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);
  return m_Result;
}

// NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
