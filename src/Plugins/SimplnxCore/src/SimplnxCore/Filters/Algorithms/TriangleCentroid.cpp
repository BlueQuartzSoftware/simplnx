#include "TriangleCentroid.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/Math/MatrixMath.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

using namespace nx::core;

namespace
{

/**
 * @brief The CalculateAreasImpl class implements a threaded algorithm that computes the normal of each
 * triangle for a set of triangles
 */
class CalculateCentroidsImpl
{
public:
  CalculateCentroidsImpl(const TriangleGeom* triangleGeom, Float32Array* centroids, const std::atomic_bool& shouldCancel)
  : m_TriangleGeom(triangleGeom)
  , m_Centroids(centroids)
  , m_ShouldCancel(shouldCancel)
  {
  }
  virtual ~CalculateCentroidsImpl() = default;

  void generate(size_t start, size_t end) const
  {
    for(usize triangleIndex = start; triangleIndex < end; triangleIndex++)
    {
      if(m_ShouldCancel)
      {
        break;
      }

      std::array<Point3Df, 3> vertCoords;
      m_TriangleGeom->getFaceCoordinates(triangleIndex, vertCoords);

      (*m_Centroids)[triangleIndex * 3] = (vertCoords[0].getX() + vertCoords[1].getX() + vertCoords[2].getX()) / 3.0F;
      (*m_Centroids)[triangleIndex * 3 + 1] = (vertCoords[0].getY() + vertCoords[1].getY() + vertCoords[2].getY()) / 3.0F;
      (*m_Centroids)[triangleIndex * 3 + 2] = (vertCoords[0].getZ() + vertCoords[1].getZ() + vertCoords[2].getZ()) / 3.0F;
    }
  }

  void operator()(const Range& range) const
  {
    generate(range.min(), range.max());
  }

private:
  const TriangleGeom* m_TriangleGeom = nullptr;
  Float32Array* m_Centroids = nullptr;
  const std::atomic_bool& m_ShouldCancel;
};
} // namespace

// -----------------------------------------------------------------------------
TriangleCentroid::TriangleCentroid(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, TriangleCentroidInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
TriangleCentroid::~TriangleCentroid() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& TriangleCentroid::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> TriangleCentroid::operator()()
{

  const TriangleGeom* triangleGeom = m_DataStructure.getDataAs<TriangleGeom>(m_InputValues->TriangleGeometryDataPath);
  const AttributeMatrix& faceAttributeMatrix = triangleGeom->getFaceAttributeMatrixRef();

  const DataPath pCentroidsPath = m_InputValues->TriangleGeometryDataPath.createChildPath(faceAttributeMatrix.getName()).createChildPath(m_InputValues->CentroidsArrayName);
  auto* centroidsArray = m_DataStructure.getDataAs<Float32Array>(pCentroidsPath);

  // Parallel algorithm to calculate the centroids
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0ULL, static_cast<size_t>(triangleGeom->getNumberOfFaces()));
  dataAlg.execute(CalculateCentroidsImpl(triangleGeom, centroidsArray, m_ShouldCancel));

  return {};
}
