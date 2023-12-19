#include "WriteStlFile.hpp"

#include "complex/Common/AtomicFile.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Utilities/FilterUtilities.hpp"
#include "complex/Utilities/StringUtilities.hpp"

using namespace complex;

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
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsPath);

  const std::filesystem::path outputPath = m_InputValues->OutputStlDirectory;
  // Make sure any directory path is also available as the user may have just typed
  // in a path without actually creating the full path
  Result<> createDirectoriesResult = complex::CreateOutputDirectories(outputPath);
  if(createDirectoriesResult.invalid())
  {
    return createDirectoriesResult;
  }

  // Store all the unique Spins
  std::map<int32, int32> uniqueGrainIdToPhase;
  if(m_InputValues->GroupByFeature)
  {
    const auto& featurePhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeaturePhasesPath);
    for(IGeometry::MeshIndexType i = 0; i < nTriangles; i++)
    {
      uniqueGrainIdToPhase.emplace(featureIds[i * 2], featurePhases[i * 2]);
      uniqueGrainIdToPhase.emplace(featureIds[i * 2 + 1], featurePhases[i * 2 + 1]);
    }
  }
  else
  {
    for(IGeometry::MeshIndexType i = 0; i < nTriangles; i++)
    {
      uniqueGrainIdToPhase.emplace(featureIds[i * 2], 0);
      uniqueGrainIdToPhase.emplace(featureIds[i * 2 + 1], 0);
    }
  }

  unsigned char data[50];
  auto* normal = reinterpret_cast<float32*>(data);
  auto* vert1 = reinterpret_cast<float32*>(data + 12);
  auto* vert2 = reinterpret_cast<float32*>(data + 24);
  auto* vert3 = reinterpret_cast<float32*>(data + 36);
  auto* attrByteCount = reinterpret_cast<uint16*>(data + 48);
  *attrByteCount = 0;

  size_t totalWritten = 0;
  float u[3] = {0.0f, 0.0f, 0.0f}, w[3] = {0.0f, 0.0f, 0.0f};
  float length = 0.0f;

  int32_t triCount = 0;

  // Loop over the unique feature Ids
  for(const auto& [featureId, value] : uniqueGrainIdToPhase)
  {
    // Generate the output file name
    std::string filename = m_InputValues->OutputStlDirectory.string() + "/" + m_InputValues->OutputStlPrefix;
    if(m_InputValues->GroupByFeature)
    {
      filename += "Ensemble_" + StringUtilities::number(value) + "_";
    }

    filename += "Feature_" + StringUtilities::number(featureId) + ".stl";

    AtomicFile atomicFile(filename, true);

    FILE* f = fopen(atomicFile.tempFilePath().string().c_str(), "wb");

    if(f == nullptr)
    {
      fclose(f);
      atomicFile.setAutoCommit(false); // Set this to false otherwise
      return {MakeWarningVoidResult(-27875, fmt::format("Error Opening STL File. Unable to create temp file at path '{}' for original file '{}'", atomicFile.tempFilePath().string(), filename))};
    }

    m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Writing STL for Feature Id {}", featureId));

    std::string header = "DREAM3D Generated For Feature ID " + StringUtilities::number(featureId);
    if(m_InputValues->GroupByFeature)
    {
      header += " Phase " + StringUtilities::number(value);
    }

    if(header.size() >= 80)
    {
      fclose(f);
      atomicFile.setAutoCommit(false); // Set this to false otherwise
      atomicFile.removeTempFile();     // Remove the temp file
      return {MakeWarningVoidResult(-27874, fmt::format("Error Writing STL File '{}'. Header was over the 80 characters supported by STL. Length of header: {}.", filename, header.length()))};
    }

    char h[80];
    size_t headLength = 80;
    if(header.length() < 80)
    {
      headLength = static_cast<size_t>(header.length());
    }

    std::string c_str = header;
    ::memset(h, 0, 80);
    ::memcpy(h, c_str.data(), headLength);
    // Return the number of bytes written - which should be 80
    fwrite(h, 1, 80, f);
    fwrite(&triCount, 1, 4, f);
    triCount = 0; // Reset this to Zero. Increment for every triangle written

    // Loop over all the triangles for this spin
    for(IGeometry::MeshIndexType t = 0; t < nTriangles; ++t)
    {
      // Get the true indices of the 3 nodes
      IGeometry::MeshIndexType nId0 = triangles[t * 3];
      IGeometry::MeshIndexType nId1 = triangles[t * 3 + 1];
      IGeometry::MeshIndexType nId2 = triangles[t * 3 + 2];

      vert1[0] = static_cast<float>(vertices[nId0 * 3]);
      vert1[1] = static_cast<float>(vertices[nId0 * 3 + 1]);
      vert1[2] = static_cast<float>(vertices[nId0 * 3 + 2]);

      if(featureIds[t * 2] == featureId)
      {
        // winding = 0; // 0 = Write it using forward spin
      }
      else if(featureIds[t * 2 + 1] == featureId)
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

      length = sqrtf(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
      normal[0] = normal[0] / length;
      normal[1] = normal[1] / length;
      normal[2] = normal[2] / length;

      totalWritten = fwrite(data, 1, 50, f);
      if(totalWritten != 50)
      {
        fclose(f);
        atomicFile.setAutoCommit(false); // Set this to false otherwise
        atomicFile.removeTempFile();     // Remove the temp file
        return {MakeWarningVoidResult(-27873,
                                      fmt::format("Error Writing STL File '{}'. Not enough elements written for Feature Id {}. Wrote {} of 50. No file written.", filename, featureId, totalWritten))};
      }
      triCount++;
    }

    fseek(f, 80L, SEEK_SET);
    fwrite(reinterpret_cast<char*>(&triCount), 1, 4, f);
    fclose(f);
  }

  return {};
}
