#include "WriteStlFile.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
Result<> SingleWriteOutStl(const fs::path& path, const IGeometry::MeshIndexType numTriangles, const std::string&& header, const IGeometry::MeshIndexArrayType& triangles, const Float32Array& vertices)
{
  // Create output file writer in binary write out mode to ensure cross-compatibility
  FILE* file = fopen(path.string().c_str(), "wb");

  if(file == nullptr)
  {
    fclose(file);
    return {MakeWarningVoidResult(-27876, fmt::format("Error Opening STL File. Unable to create temp file at path '{}' for original file '{}'", path.string(), path.filename().string()))};
  }

  int32 triCount = 0;

  { // Scope header output processing to keep overhead low and increase readability
    if(header.size() >= 80)
    {
      fclose(file);
      return {MakeWarningVoidResult(-27874,
                                    fmt::format("Error Writing STL File '{}'. Header was over the 80 characters supported by STL. Length of header: {}.", path.filename().string(), header.length()))};
    }

    char h[80];
    size_t headLength = 80;
    if(header.length() < 80)
    {
      headLength = static_cast<size_t>(header.length());
    }

    std::string c_str = header;
    memset(h, 0, 80);
    memcpy(h, c_str.data(), headLength);
    // Return the number of bytes written - which should be 80
    fwrite(h, 1, 80, file);
  }

  fwrite(&triCount, 1, 4, file);
  triCount = 0; // Reset this to Zero. Increment for every triangle written

  size_t totalWritten = 0;
  float u[3] = {0.0f, 0.0f, 0.0f}, w[3] = {0.0f, 0.0f, 0.0f};

  unsigned char data[50];
  auto* normal = reinterpret_cast<float32*>(data);
  auto* vert1 = reinterpret_cast<float32*>(data + 12);
  auto* vert2 = reinterpret_cast<float32*>(data + 24);
  auto* vert3 = reinterpret_cast<float32*>(data + 36);
  auto* attrByteCount = reinterpret_cast<uint16*>(data + 48);
  *attrByteCount = 0;

  // Loop over all the triangles for this spin
  for(IGeometry::MeshIndexType triangle = 0; triangle < numTriangles; ++triangle)
  {
    // Get the true indices of the 3 nodes
    IGeometry::MeshIndexType nId0 = triangles[triangle * 3];
    IGeometry::MeshIndexType nId1 = triangles[triangle * 3 + 1];
    IGeometry::MeshIndexType nId2 = triangles[triangle * 3 + 2];

    vert1[0] = static_cast<float>(vertices[nId0 * 3]);
    vert1[1] = static_cast<float>(vertices[nId0 * 3 + 1]);
    vert1[2] = static_cast<float>(vertices[nId0 * 3 + 2]);

    vert2[0] = static_cast<float>(vertices[nId1 * 3]);
    vert2[1] = static_cast<float>(vertices[nId1 * 3 + 1]);
    vert2[2] = static_cast<float>(vertices[nId1 * 3 + 2]);

    vert3[0] = static_cast<float>(vertices[nId2 * 3]);
    vert3[1] = static_cast<float>(vertices[nId2 * 3 + 1]);
    vert3[2] = static_cast<float>(vertices[nId2 * 3 + 2]);

    // Compute the normal
    u[0] = vert2[0] - vert1[0];
    u[1] = vert2[1] - vert1[1];
    u[2] = vert2[2] - vert1[2];

    w[0] = vert3[0] - vert1[0];
    w[1] = vert3[1] - vert1[1];
    w[2] = vert3[2] - vert1[2];

    normal[0] = u[1] * w[2] - u[2] * w[1];
    normal[1] = u[2] * w[0] - u[0] * w[2];
    normal[2] = u[0] * w[1] - u[1] * w[0];

    float length = sqrtf(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
    normal[0] = normal[0] / length;
    normal[1] = normal[1] / length;
    normal[2] = normal[2] / length;

    totalWritten = fwrite(data, 1, 50, file);
    if(totalWritten != 50)
    {
      fclose(file);
      return {MakeWarningVoidResult(
          -27873, fmt::format("Error Writing STL File '{}'. Not enough elements written for Triangle {}. Wrote {} of 50. No file written.", path.filename().string(), triangle, totalWritten))};
    }
    triCount++;
  }

  fseek(file, 80L, SEEK_SET);
  fwrite(reinterpret_cast<char*>(&triCount), 1, 4, file);
  fclose(file);
  return {};
}

Result<> MultiWriteOutStl(const fs::path& path, const IGeometry::MeshIndexType numTriangles, const std::string&& header, const IGeometry::MeshIndexArrayType& triangles, const Float32Array& vertices,
                          const Int32Array& featureIds, const int32 featureId)
{
  // Create output file writer in binary write out mode to ensure cross-compatibility
  FILE* file = fopen(path.string().c_str(), "wb");

  if(file == nullptr)
  {
    fclose(file);
    return {MakeWarningVoidResult(-27876, fmt::format("Error Opening STL File. Unable to create temp file at path '{}' for original file '{}'", path.string(), path.filename().string()))};
  }

  int32 triCount = 0;

  { // Scope header output processing to keep overhead low and increase readability
    if(header.size() >= 80)
    {
      fclose(file);
      return {MakeWarningVoidResult(-27874,
                                    fmt::format("Error Writing STL File '{}'. Header was over the 80 characters supported by STL. Length of header: {}.", path.filename().string(), header.length()))};
    }

    char h[80];
    size_t headLength = 80;
    if(header.length() < 80)
    {
      headLength = static_cast<size_t>(header.length());
    }

    std::string c_str = header;
    memset(h, 0, 80);
    memcpy(h, c_str.data(), headLength);
    // Return the number of bytes written - which should be 80
    fwrite(h, 1, 80, file);
  }

  fwrite(&triCount, 1, 4, file);
  triCount = 0; // Reset this to Zero. Increment for every triangle written

  size_t totalWritten = 0;
  float u[3] = {0.0f, 0.0f, 0.0f}, w[3] = {0.0f, 0.0f, 0.0f};

  unsigned char data[50];
  auto* normal = reinterpret_cast<float32*>(data);
  auto* vert1 = reinterpret_cast<float32*>(data + 12);
  auto* vert2 = reinterpret_cast<float32*>(data + 24);
  auto* vert3 = reinterpret_cast<float32*>(data + 36);
  auto* attrByteCount = reinterpret_cast<uint16*>(data + 48);
  *attrByteCount = 0;

  // Loop over all the triangles for this spin
  for(IGeometry::MeshIndexType triangle = 0; triangle < numTriangles; ++triangle)
  {
    // Get the true indices of the 3 nodes
    IGeometry::MeshIndexType nId0 = triangles[triangle * 3];
    IGeometry::MeshIndexType nId1 = triangles[triangle * 3 + 1];
    IGeometry::MeshIndexType nId2 = triangles[triangle * 3 + 2];

    if(featureIds[triangle * 2] == featureId)
    {
      // winding = 0; // 0 = Write it using forward spin
    }
    else if(featureIds[triangle * 2 + 1] == featureId)
    {
      // winding = 1; // Write it using backward spin
      // Switch the 2 node indices
      IGeometry::MeshIndexType temp = nId1;
      nId1 = nId2;
      nId2 = temp;
    }
    else
    {
      continue; // We do not match either spin so move to the next triangle
    }

    vert1[0] = static_cast<float>(vertices[nId0 * 3]);
    vert1[1] = static_cast<float>(vertices[nId0 * 3 + 1]);
    vert1[2] = static_cast<float>(vertices[nId0 * 3 + 2]);

    vert2[0] = static_cast<float>(vertices[nId1 * 3]);
    vert2[1] = static_cast<float>(vertices[nId1 * 3 + 1]);
    vert2[2] = static_cast<float>(vertices[nId1 * 3 + 2]);

    vert3[0] = static_cast<float>(vertices[nId2 * 3]);
    vert3[1] = static_cast<float>(vertices[nId2 * 3 + 1]);
    vert3[2] = static_cast<float>(vertices[nId2 * 3 + 2]);

    // Compute the normal
    u[0] = vert2[0] - vert1[0];
    u[1] = vert2[1] - vert1[1];
    u[2] = vert2[2] - vert1[2];

    w[0] = vert3[0] - vert1[0];
    w[1] = vert3[1] - vert1[1];
    w[2] = vert3[2] - vert1[2];

    normal[0] = u[1] * w[2] - u[2] * w[1];
    normal[1] = u[2] * w[0] - u[0] * w[2];
    normal[2] = u[0] * w[1] - u[1] * w[0];

    float length = sqrtf(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
    normal[0] = normal[0] / length;
    normal[1] = normal[1] / length;
    normal[2] = normal[2] / length;

    totalWritten = fwrite(data, 1, 50, file);
    if(totalWritten != 50)
    {
      fclose(file);
      return {MakeWarningVoidResult(
          -27873, fmt::format("Error Writing STL File '{}'. Not enough elements written for Feature Id {}. Wrote {} of 50. No file written.", path.filename().string(), featureId, totalWritten))};
    }
    triCount++;
  }

  fseek(file, 80L, SEEK_SET);
  fwrite(reinterpret_cast<char*>(&triCount), 1, 4, file);
  fclose(file);
  return {};
}
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
  const Float32Array& vertices = triangleGeom.getVerticesRef();
  const IGeometry::MeshIndexArrayType& triangles = triangleGeom.getFacesRef();
  const IGeometry::MeshIndexType nTriangles = triangleGeom.getNumberOfFaces();

  auto groupingType = static_cast<GroupingType>(m_InputValues->GroupingType);

  if(groupingType == GroupingType::None)
  {
    AtomicFile atomicFile(m_InputValues->OutputStlFile.string(), false);

    if(atomicFile.getResult().invalid())
    {
      return atomicFile.getResult();
    }

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

    atomicFile.commit();
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

  // Store a list of Atomic Files, so we can clean up or finish depending on the outcome of all the writes
  std::vector<std::unique_ptr<AtomicFile>> fileList = {};

  { // Scope to cut overhead and ensure file lock is released on windows
    const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsPath);
    // Store all the unique Spins
    if(groupingType == GroupingType::Features)
    {
      // Faster and more memory efficient since we don't need phases
      std::unordered_set<int32> uniqueGrainIds(featureIds.cbegin(), featureIds.cend());

      fileList.reserve(uniqueGrainIds.size());

      usize fileIndex = 0;
      for(const auto featureId : uniqueGrainIds)
      {
        // Generate the output file
        fileList.push_back(
            std::make_unique<AtomicFile>(m_InputValues->OutputStlDirectory.string() + "/" + m_InputValues->OutputStlPrefix + "Feature_" + StringUtilities::number(featureId) + ".stl", false));

        if(fileList[fileIndex]->getResult().invalid())
        {
          return fileList[fileIndex]->getResult();
        }

        m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Writing STL for Feature Id {}", featureId));

        auto result =
            ::MultiWriteOutStl(fileList[fileIndex]->tempFilePath(), nTriangles, {"DREAM3D Generated For Feature ID " + StringUtilities::number(featureId)}, triangles, vertices, featureIds, featureId);
        // if valid Loop over all the triangles for this spin
        if(result.invalid())
        {
          return result;
        }

        fileIndex++;
      }
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
        fileList.push_back(std::make_unique<AtomicFile>(m_InputValues->OutputStlDirectory.string() + "/" + m_InputValues->OutputStlPrefix + "Ensemble_" + StringUtilities::number(value) + "_" +
                                                            "Feature_" + StringUtilities::number(featureId) + ".stl",
                                                        false));

        if(fileList[fileIndex]->getResult().invalid())
        {
          return fileList[fileIndex]->getResult();
        }

        m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Writing STL for Feature Id {}", featureId));

        auto result =
            ::MultiWriteOutStl(fileList[fileIndex]->tempFilePath(), nTriangles, {"DREAM3D Generated For Feature ID " + StringUtilities::number(featureId) + " Phase " + StringUtilities::number(value)},
                               triangles, vertices, featureIds, featureId);
        // if valid loop over all the triangles for this spin
        if(result.invalid())
        {
          return result;
        }

        fileIndex++;
      }
    }
  }

  for(const auto& atomicFile : fileList)
  {
    atomicFile->commit();
  }

  return {};
}
