#include "WriteGBCDTriangleData.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
WriteGBCDTriangleData::WriteGBCDTriangleData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             WriteGBCDTriangleDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
WriteGBCDTriangleData::~WriteGBCDTriangleData() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& WriteGBCDTriangleData::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> WriteGBCDTriangleData::operator()()
{
  auto& faceLabels = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->SurfaceMeshFaceLabelsArrayPath);
  auto& faceNormals = m_DataStructure.getDataRefAs<Float64Array>(m_InputValues->SurfaceMeshFaceNormalsArrayPath);
  auto& faceAreas = m_DataStructure.getDataRefAs<Float64Array>(m_InputValues->SurfaceMeshFaceAreasArrayPath);
  auto& eulerAngles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->FeatureEulerAnglesArrayPath);
  usize numTriangles = faceAreas.getNumberOfTuples();

  FILE* f = fopen(m_InputValues->OutputFile.string().c_str(), "wb");
  if(nullptr == f)
  {
    return MakeErrorResult(-87000, fmt::format("Error opening output file '{}'", m_InputValues->OutputFile.string()));
  }

  // fprintf(f, "# Triangles Produced from DREAM3D version %s\n", ImportExport::Version::Package().toLatin1().data());
  fprintf(f, "# Column 1-3:    right hand average orientation (phi1, PHI, phi2 in RADIANS)\n");
  fprintf(f, "# Column 4-6:    left hand average orientation (phi1, PHI, phi2 in RADIANS)\n");
  fprintf(f, "# Column 7-9:    triangle normal\n");
  fprintf(f, "# Column 8:      surface area\n");

  int32 gid0 = 0; // Feature identifier 0
  int32 gid1 = 0; // Feature identifier 1
  for(int64 t = 0; t < numTriangles; ++t)
  {
    // Get the Feature Ids for the triangle
    gid0 = faceLabels[t * 2];
    gid1 = faceLabels[t * 2 + 1];

    if(gid0 < 0)
    {
      continue;
    }
    if(gid1 < 0)
    {
      continue;
    }

    // Now get the Euler Angles for that feature identifier
    float32 euAngRightHand0 = eulerAngles[gid0 * 3];
    float32 euAngRightHand1 = eulerAngles[gid0 * 3 + 1];
    float32 euAngRightHand2 = eulerAngles[gid0 * 3 + 2];
    float32 euAngLeftHand0 = eulerAngles[gid1 * 3];
    float32 euAngLeftHand1 = eulerAngles[gid1 * 3 + 1];
    float32 euAngLeftHand2 = eulerAngles[gid1 * 3 + 2];

    // Get the Triangle Normal
    float64 tNorm0 = faceNormals[t * 3];
    float64 tNorm1 = faceNormals[t * 3 + 1];
    float64 tNorm2 = faceNormals[t * 3 + 2];

    fprintf(f, "%0.4f %0.4f %0.4f %0.4f %0.4f %0.4f %0.4f %0.4f %0.4f %0.4f\n", euAngRightHand0, euAngRightHand1, euAngRightHand2, euAngLeftHand0, euAngLeftHand1, euAngLeftHand2, tNorm0, tNorm1,
            tNorm2, faceAreas[t]);
  }

  fclose(f);
  return {};
}
