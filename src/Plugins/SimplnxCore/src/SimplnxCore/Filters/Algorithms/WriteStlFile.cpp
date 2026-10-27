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

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <iterator>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
using TriStore = AbstractDataStore<IGeometry::MeshIndexArrayType::value_type>;
using VertexStore = AbstractDataStore<IGeometry::SharedVertexList::value_type>;

struct LimitBoundAtomicFileFactory;

/**
 * @brief Returns how many files a sequence needs for a triangle count.
 * @param triangleCount Number of triangles in the sequence.
 * @param maxTriangles Largest number of triangles in one file. A value of 0 is used as 1.
 * @return At least one file; a sequence without triangles still writes one file with a header.
 */
usize StlFileCount(usize triangleCount, usize maxTriangles)
{
  const usize limit = std::max<usize>(maxTriangles, 1);
  return triangleCount == 0 ? 1 : (triangleCount + limit - 1) / limit;
}

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

  std::vector<AtomicFile> m_AtomicFilesList = {};

private:
  fs::path m_InputPath;

  LimitBoundAtomicFile(const fs::path& inputPath)
  : m_InputPath(inputPath)
  {
  }

  /**
   * @brief Adds one numbered overflow destination.
   * @return The warnings of the file-replace hook, or the AtomicFile creation error with its warnings.
   */
  Result<> createOverflowFile()
  {
    const fs::path newPath = fs::path(fmt::format("{}/{}_overflow_{}{}", m_InputPath.parent_path().string(), m_InputPath.stem().string(), m_AtomicFilesList.size(), m_InputPath.extension().string()));
    auto atomicFileResult = AtomicFile::Create(newPath);
    if(atomicFileResult.invalid())
    {
      return ConvertResult(std::move(atomicFileResult));
    }
    m_AtomicFilesList.emplace_back(std::move(atomicFileResult.value()));
    return ConvertResult(std::move(atomicFileResult));
  }
};

/**
 * @struct LimitBoundAtomicFileFactory
 * @brief Creates every file of an overflow sequence.
 */
struct LimitBoundAtomicFileFactory
{
  /**
   * @brief Creates one overflow-sequence owner with all of its files.
   * @param inputPath Specifies the first destination.
   * @param fileCount Number of files in the sequence; at least the first file is created.
   * @return Owner with the warnings of the file-replace hooks, or the first AtomicFile creation error, unchanged, with the
   * warnings collected before it.
   *
   * Every file is created here, on the calling thread, before a worker writes. Thus the file-replace hook runs under the
   * policy of the calling thread and never runs, or throws, while workers run.
   */
  static Result<LimitBoundAtomicFile> Create(const fs::path& inputPath, usize fileCount)
  {
    LimitBoundAtomicFile outClass(inputPath);

    auto atomicFileResult = AtomicFile::Create(inputPath);
    if(atomicFileResult.invalid())
    {
      return ConvertInvalidResult<LimitBoundAtomicFile>(std::move(atomicFileResult));
    }
    outClass.m_AtomicFilesList.emplace_back(std::move(atomicFileResult.value()));
    std::vector<Warning> hookWarnings = std::move(atomicFileResult.warnings());

    for(usize fileIndex = 1; fileIndex < fileCount; fileIndex++)
    {
      Result<> overflowResult = outClass.createOverflowFile();
      overflowResult.warnings().insert(overflowResult.warnings().begin(), hookWarnings.begin(), hookWarnings.end());
      if(overflowResult.invalid())
      {
        return ConvertInvalidResult<LimitBoundAtomicFile>(std::move(overflowResult));
      }
      hookWarnings = std::move(overflowResult.warnings());
    }

    // The warnings of the file-replace hooks travel with the new owner.
    Result<LimitBoundAtomicFile> ownerResult{std::move(outClass)};
    ownerResult.warnings() = std::move(hookWarnings);
    return ownerResult;
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
    m_Filter->sendThreadSafeProgressMessage(writeAll());
  }

  /**
   * @brief Writes every prepared file of the sequence, each with at most maxTriangles triangles.
   * @return The first file error with the header warnings collected before it, or success with every header warning.
   *
   * The factory created one file per StlFileCount() block, so no file is created here.
   */
  [[nodiscard]] Result<> writeAll() const
  {
    Result<> result;
    const usize limit = std::max<usize>(m_MaxTriangles, 1);
    const usize groupTriangleCount = m_TriangleIndices.size();
    const std::vector<AtomicFile>& files = m_LimitBoundAtomicFile.m_AtomicFilesList;
    for(usize fileIndex = 0; fileIndex < files.size(); fileIndex++)
    {
      const usize beginIndex = std::min(fileIndex * limit, groupTriangleCount);
      const usize endIndex = std::min(beginIndex + limit, groupTriangleCount);
      Result<> fileResult = write(files[fileIndex].tempFilePath(), beginIndex, endIndex);
      fileResult.warnings().insert(fileResult.warnings().begin(), result.warnings().begin(), result.warnings().end());
      // The first error wins; MergeResults() is not used because it would make an invalid result without errors valid.
      if(fileResult.invalid() || m_ShouldCancel)
      {
        return fileResult;
      }
      result = std::move(fileResult);
    }
    return result;
  }

  /**
   * @brief Writes one file of the sequence.
   * @param activePath Identifies the active temporary file.
   * @param startIndex Specifies the first index in this task's triangle bucket.
   * @param endIndex Specifies one past the last index in this task's triangle bucket.
   * @return The first open, write, seek, or close error, or success carrying the header warning of this file.
   *
   * Every stdio failure becomes an error, because a truncated temporary file must never
   * reach the destination through a commit.
   */
  [[nodiscard]] Result<> write(const fs::path& activePath, usize startIndex, usize endIndex) const
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
    for(usize idx = startIndex; idx < endIndex; idx++)
    {
      if(m_ShouldCancel)
      {
        // The header warning collected above must survive any finalize error.
        return MergeResults(std::move(result), FinalizeStlFile(filePtr, activePath, triCount));
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
 * @struct StlGroup
 * @brief Describes one label group of a grouped STL write.
 */
struct StlGroup
{
  int32 label = 0;
  fs::path firstFile;
  std::string header;
  std::string message;
};

/**
 * @brief Wraps warnings in a valid result.
 * @param warnings Warnings to carry.
 * @return Valid result with the warnings.
 */
Result<> WarningsOnly(std::vector<Warning> warnings)
{
  Result<> result;
  result.warnings() = std::move(warnings);
  return result;
}

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

  // Every file exists before a worker starts, so the file-replace hook runs on this thread only.
  auto atomicFileResult = LimitBoundAtomicFileFactory::Create(firstFile, count + 1);
  if(atomicFileResult.invalid())
  {
    return ConvertResult(std::move(atomicFileResult));
  }
  LimitBoundAtomicFile limitedFile(std::move(atomicFileResult.value()));
  // The file-replace hooks can report warnings; every later result carries them first.
  const std::vector<Warning> hookWarnings = std::move(atomicFileResult.warnings());
  const auto withHookWarnings = [&hookWarnings](Result<> result) {
    result.warnings().insert(result.warnings().begin(), hookWarnings.begin(), hookWarnings.end());
    return result;
  };

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
    return withHookWarnings(std::move(workerResult));
  }

  if(shouldCancel)
  {
    // The warnings latched by the workers must still reach the user.
    return withHookWarnings(MergeResults(std::move(workerResult), MakeErrorResult(-1, "Filter cancelled")));
  }

  for(auto& atomicFile : limitedFile.m_AtomicFilesList)
  {
    if(shouldCancel)
    {
      // The warnings latched by the workers must still reach the user.
      return withHookWarnings(MergeResults(std::move(workerResult), MakeErrorResult(-1, "Filter cancelled")));
    }
    Result<> commitResult = atomicFile.commit();
    if(commitResult.invalid())
    {
      // The warnings latched by the workers must still reach the user.
      return withHookWarnings(MergeResults(std::move(workerResult), std::move(commitResult)));
    }
  }

  return withHookWarnings(std::move(workerResult));
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
    Result<> result = ::SingleWriteOutStl(atomicFile.tempFilePath(), nTriangles, header, triangles, vertices, m_ShouldCancel);
    if(result.valid())
    {
      // The header warning collected by the write must still reach the user.
      if(m_ShouldCancel)
      {
        result = MergeResults(std::move(result), MakeErrorResult(-1, "Filter cancelled"));
      }
      else if(Result<> commitResult = atomicFile.commit(); commitResult.invalid())
      {
        result = MergeResults(std::move(result), std::move(commitResult));
      }
    }
    // The file-replace hook in AtomicFile::Create() can report warnings; they come first.
    result.warnings().insert(result.warnings().begin(), atomicFileResult.warnings().begin(), atomicFileResult.warnings().end());
    return result;
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

  // The membership map and every AtomicFile are declared before the task runner. Its destructor joins every worker
  // before those objects are destroyed, including during exception unwinding.
  std::unordered_map<int32, std::vector<usize>> trianglesByLabel;
  const Int32AbstractDataStore* groupLabels = nullptr;
  std::vector<::StlGroup> groups;
  std::vector<LimitBoundAtomicFile> fileList;

  if(groupingType == GroupingType::Features)
  {
    groupLabels = &m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsPath)->getDataStoreRef();
    trianglesByLabel = ::BuildTrianglesByLabel(*groupLabels);
    for(const auto& [featureId, featureTriangles] : trianglesByLabel)
    {
      groups.push_back({featureId, m_InputValues->OutputStlDirectory / fmt::format("{}Feature_{}.stl", m_InputValues->OutputStlPrefix, featureId),
                        "DREAM3D Generated For Feature ID " + StringUtilities::number(featureId), fmt::format("Writing STL for Feature Id {}", featureId)});
    }
  }

  if(groupingType == GroupingType::FeaturesAndPhases)
  {
    groupLabels = &m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsPath)->getDataStoreRef();
    const Int32AbstractDataStore& featureIds = *groupLabels;

    std::map<int32, int32> uniqueGrainIdToPhase;
    const auto& featurePhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeaturePhasesPath);
    for(IGeometry::MeshIndexType i = 0; i < nTriangles; i++)
    {
      uniqueGrainIdToPhase.emplace(featureIds[i * 2], featurePhases[i * 2]);
      uniqueGrainIdToPhase.emplace(featureIds[i * 2 + 1], featurePhases[i * 2 + 1]);
    }

    // Reuse the feature-group membership and winding rule for phase-qualified names.
    trianglesByLabel = ::BuildTrianglesByLabel(featureIds);
    for(const auto& [featureId, value] : uniqueGrainIdToPhase)
    {
      groups.push_back({featureId, m_InputValues->OutputStlDirectory / fmt::format("{}Ensemble_{}_Feature_{}.stl", m_InputValues->OutputStlPrefix, value, featureId),
                        "DREAM3D Generated For Feature ID " + StringUtilities::number(featureId) + " Phase " + StringUtilities::number(value),
                        fmt::format("Writing STL for Phase {} and Feature Id {}", value, featureId)});
    }
  }

  if(groupingType == GroupingType::PartNumber)
  {
    groupLabels = &m_DataStructure.getDataAs<Int32Array>(m_InputValues->PartNumberPath)->getDataStoreRef();
    trianglesByLabel = ::BuildTrianglesByLabel(*groupLabels);
    for(const auto& [currentPartNumber, partTriangles] : trianglesByLabel)
    {
      groups.push_back({currentPartNumber, m_InputValues->OutputStlDirectory / fmt::format("{}{}.stl", m_InputValues->OutputStlPrefix, currentPartNumber),
                        "DREAM3D Generated For Part Number " + StringUtilities::number(currentPartNumber), fmt::format("Writing STL for Part Number {}", currentPartNumber)});
    }
  }

  // Every destination and its overflow files are prepared on this thread before the first worker starts. Thus the file-replace
  // hooks run under the policy of this thread, and a hook that fails or throws leaves no worker running.
  fileList.reserve(groups.size());
  std::vector<Warning> hookWarnings;
  for(const ::StlGroup& group : groups)
  {
    const usize fileCount = ::StlFileCount(trianglesByLabel.at(group.label).size(), m_InputValues->HIDDEN_MaxTrianglesPerFile);
    auto atomicFileResult = LimitBoundAtomicFileFactory::Create(group.firstFile, fileCount);
    atomicFileResult.warnings().insert(atomicFileResult.warnings().begin(), hookWarnings.begin(), hookWarnings.end());
    if(atomicFileResult.invalid())
    {
      return ConvertResult(std::move(atomicFileResult));
    }
    hookWarnings = std::move(atomicFileResult.warnings());
    fileList.emplace_back(std::move(atomicFileResult.value()));
  }
  // The warnings of the file-replace hooks come before the worker warnings.
  sendThreadSafeProgressMessage(::WarningsOnly(std::move(hookWarnings)));

  // Each group task writes a separate temporary file sequence.
  ParallelTaskAlgorithm taskRunner;
  taskRunner.setParallelizationEnabled(true);
  for(usize groupIndex = 0; groupIndex < groups.size(); groupIndex++)
  {
    const ::StlGroup& group = groups[groupIndex];
    m_MessageHandler.sendMessage(IFilter::Message::Type::Info, group.message);
    taskRunner.execute(MultiWriteStlFileImpl(this, fileList[groupIndex], group.header, triangles, vertices, *groupLabels, group.label, trianglesByLabel.at(group.label),
                                             m_InputValues->HIDDEN_MaxTrianglesPerFile, m_ShouldCancel));
    if(m_HasErrors.load())
    {
      break;
    }
  }
  taskRunner.wait();

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
    // The first error wins, so a later worker cannot bury the original cause. The latch takes the errors directly, because
    // MergeResults() would make an invalid result without errors valid.
    m_HasErrors.store(true);
    Result<> latchedResult{{nonstd::make_unexpected(std::move(result.errors()))}};
    latchedResult.warnings() = std::move(m_Result.warnings());
    m_Result = std::move(latchedResult);
  }
}

Result<> WriteStlFile::getWorkerResult() const
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);
  return m_Result;
}

// NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
