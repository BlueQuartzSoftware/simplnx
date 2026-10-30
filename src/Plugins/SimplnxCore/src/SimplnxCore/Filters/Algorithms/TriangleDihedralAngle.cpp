#include "TriangleDihedralAngle.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/Common/Range.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <algorithm>

using namespace nx::core;

namespace
{
constexpr float64 k_radToDeg = Constants::k_180OverPiD; // used for translating radians to degrees

/**
 * @brief The CalculateAreasImpl class implements a threaded algorithm that computes the normal of each
 * triangle for a set of triangles
 */
class CalculateDihedralAnglesImpl
{
public:
  CalculateDihedralAnglesImpl(const TriangleGeom* triangleGeom, Float64AbstractDataStore& dihedralAngles, const std::atomic_bool& shouldCancel)
  : m_TriangleGeom(triangleGeom)
  , m_DihedralAngles(dihedralAngles)
  , m_ShouldCancel(shouldCancel)
  {
  }
  virtual ~CalculateDihedralAnglesImpl() = default;

  void generate(size_t start, size_t end) const
  {
    // std::array<float64, 3> vectorEx = {x, y, z};  // coordinate example
    nx::core::Vec3<float64> vecAB = {0.0f, 0.0f, 0.0f};
    nx::core::Vec3<float64> vecAC = {0.0f, 0.0f, 0.0f};
    nx::core::Vec3<float64> vecBC = {0.0f, 0.0f, 0.0f};

    for(size_t triangleIndex = start; triangleIndex < end; triangleIndex++)
    {
      if(m_ShouldCancel)
      {
        break;
      }
      std::array<Point3Df, 3> vertCoords;
      m_TriangleGeom->getFaceCoordinates(triangleIndex, vertCoords);

      for(usize i = 0; i < 3; i++)
      {
        vecAB[i] = vertCoords[0][i] - vertCoords[1][i];
        vecAC[i] = vertCoords[0][i] - vertCoords[2][i];
        vecBC[i] = vertCoords[1][i] - vertCoords[2][i];
      }

      float64 magAB = vecAB.magnitude();
      float64 magAC = vecAC.magnitude();
      float64 magBC = vecBC.magnitude();

      if(magAB == 0.0f || magAC == 0.0f || magBC == 0.0f)
      {
        m_DihedralAngles[triangleIndex] = std::nan("0");
      }
      else
      {
        std::vector<float64> dihedralAnglesVec;
        dihedralAnglesVec.push_back(k_radToDeg * acos(std::clamp(vecAB.dot(vecAC) / (magAB * magAC), -1.0, 1.0)));

        // vecAB points out of B and vecBC points into B, so subtract their angle from 180 to get the interior angle at B.
        dihedralAnglesVec.push_back(180.0 - (k_radToDeg * acos(std::clamp(vecAB.dot(vecBC) / (magAB * magBC), -1.0, 1.0))));
        dihedralAnglesVec.push_back(k_radToDeg * acos(std::clamp(vecBC.dot(vecAC) / (magBC * magAC), -1.0, 1.0)));

        m_DihedralAngles[triangleIndex] = *std::min_element(dihedralAnglesVec.begin(), dihedralAnglesVec.end());
      }

      if(m_ShouldCancel)
      {
        return;
      }
    }
  }

  void operator()(const Range& range) const
  {
    generate(range.min(), range.max());
  }

private:
  const TriangleGeom* m_TriangleGeom = nullptr;
  Float64AbstractDataStore& m_DihedralAngles;
  const std::atomic_bool& m_ShouldCancel;
};
} // namespace

// -----------------------------------------------------------------------------
TriangleDihedralAngle::TriangleDihedralAngle(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             TriangleDihedralAngleInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
TriangleDihedralAngle::~TriangleDihedralAngle() noexcept = default;

// -----------------------------------------------------------------------------
Result<> TriangleDihedralAngle::operator()()
{
  auto pTriangleGeometryDataPath = m_InputValues->InputTriangleGeometryPath;
  auto pMinDihedralAnglesName = m_InputValues->SurfaceMeshTriangleDihedralAnglesArrayName;

  const auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(pTriangleGeometryDataPath);
  const AttributeMatrix* faceAttributeMatrix = triangleGeom.getFaceAttributeMatrix();
  const DataPath dihedralAnglesArrayPath = pTriangleGeometryDataPath.createChildPath(faceAttributeMatrix->getName()).createChildPath(pMinDihedralAnglesName);
  auto& dihedralAnglesRef = m_DataStructure.getDataAs<Float64Array>(dihedralAnglesArrayPath)->getDataStoreRef();

  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0ULL, static_cast<size_t>(triangleGeom.getNumberOfFaces()));
  dataAlg.execute(CalculateDihedralAnglesImpl(&triangleGeom, dihedralAnglesRef, m_ShouldCancel));

  return {};
}
