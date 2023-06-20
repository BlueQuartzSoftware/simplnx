#include "TriangleDihedralAngleFilter.hpp"

#include "complex/Common/Constants.hpp"
#include "complex/Common/Range.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/IGeometry.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Utilities/Math/MatrixMath.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"

#include <algorithm>

using namespace complex;

namespace
{
constexpr float64 k_radToDeg = Constants::k_180OverPiD; // used for translating radians to degrees
constexpr complex::int32 k_MissingFeatureAttributeMatrix = -76970;

/**
 * @brief The CalculateAreasImpl class implements a threaded algorithm that computes the normal of each
 * triangle for a set of triangles
 */
class CalculateDihedralAnglesImpl
{
public:
  CalculateDihedralAnglesImpl(const TriangleGeom* triangleGeom, Float64Array& dihedralAngles, const std::atomic_bool& shouldCancel)
  : m_TriangleGeom(triangleGeom)
  , m_DihedralAngles(dihedralAngles)
  , m_ShouldCancel(shouldCancel)
  {
  }
  virtual ~CalculateDihedralAnglesImpl() = default;

  void generate(size_t start, size_t end) const
  {

    const IGeometry::SharedVertexList* mNodes = m_TriangleGeom->getVertices();
    const IGeometry::SharedTriList* mTriangles = m_TriangleGeom->getFaces();

    IGeometry::MeshIndexType nIdx0 = 0, nIdx1 = 0, nIdx2 = 0;
    // std::array<float64, 3> vectorEx = {x, y, z};  // coordinate example
    std::array<float64, 3> vecAB = {0.0f, 0.0f, 0.0f};
    std::array<float64, 3> vecAC = {0.0f, 0.0f, 0.0f};
    std::array<float64, 3> vecBC = {0.0f, 0.0f, 0.0f};

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

      float64 magAB = MatrixMath::Magnitude3x1(vecAB.data());
      float64 magAC = MatrixMath::Magnitude3x1(vecAC.data());
      float64 magBC = MatrixMath::Magnitude3x1(vecBC.data());

      if(magAB == 0.0f || magAC == 0.0f || magBC == 0.0f)
      {
        m_DihedralAngles[triangleIndex] = std::nan("0");
      }
      else
      {
        std::vector<float64> dihedralAnglesVec;
        dihedralAnglesVec.push_back(k_radToDeg * acos((std::fabs(MatrixMath::DotProduct3x1(vecAB.data(), vecAC.data())) / (magAB * magAC))));

        // 180 - angle because AB points out of vertex and BC points into vertex, so angle is actually angle outside of triangle
        dihedralAnglesVec.push_back(180.0 - (k_radToDeg * acos((std::fabs(MatrixMath::DotProduct3x1(vecAB.data(), vecBC.data())) / (magAB * magBC)))));
        dihedralAnglesVec.push_back(k_radToDeg * acos((std::fabs(MatrixMath::DotProduct3x1(vecBC.data(), vecAC.data())) / (magBC * magAC))));

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
  Float64Array& m_DihedralAngles;
  const std::atomic_bool& m_ShouldCancel;
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string TriangleDihedralAngleFilter::name() const
{
  return FilterTraits<TriangleDihedralAngleFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string TriangleDihedralAngleFilter::className() const
{
  return FilterTraits<TriangleDihedralAngleFilter>::className;
}

//------------------------------------------------------------------------------
Uuid TriangleDihedralAngleFilter::uuid() const
{
  return FilterTraits<TriangleDihedralAngleFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string TriangleDihedralAngleFilter::humanName() const
{
  return "Calculate Triangle Minimum Dihedral Angle";
}

//------------------------------------------------------------------------------
std::vector<std::string> TriangleDihedralAngleFilter::defaultTags() const
{
  return {"Surface Meshing", "Misc", "Statistics", "Triangle"};
}

//------------------------------------------------------------------------------
Parameters TriangleDihedralAngleFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Required Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TGeometryDataPath_Key, "Triangle Geometry", "The complete path to the Geometry for which to calculate the dihedral angles", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Triangle}));
  params.insertSeparator(Parameters::Separator{"Created Face Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_SurfaceMeshTriangleDihedralAnglesArrayName_Key, "Created Dihedral Angles", "The name of the array storing the calculated dihedral angles",
                                                          "Dihedral Angles"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer TriangleDihedralAngleFilter::clone() const
{
  return std::make_unique<TriangleDihedralAngleFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult TriangleDihedralAngleFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel) const
{
  auto pTriangleGeometryDataPath = filterArgs.value<DataPath>(k_TGeometryDataPath_Key);
  auto pMinDihedralAnglesName = filterArgs.value<std::string>(k_SurfaceMeshTriangleDihedralAnglesArrayName_Key);

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  const auto* triangleGeom = dataStructure.getDataAs<TriangleGeom>(pTriangleGeometryDataPath);
  if(triangleGeom == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-9860, fmt::format("Cannot find the selected Triangle Geometry at path '{}'", pTriangleGeometryDataPath.toString()))};
  }
  // Get the Face AttributeMatrix from the Geometry (It should have been set at construction of the Triangle Geometry)
  const AttributeMatrix* faceAttributeMatrix = triangleGeom->getFaceAttributeMatrix();
  if(faceAttributeMatrix == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-9861, fmt::format("Cannot find the face data Attribute Matrix for the selected Triangle Geometry at path '{}'", pTriangleGeometryDataPath.toString()))};
  }

  // Instantiate and move the action that will create the output array
  {
    DataPath createArrayDataPath = pTriangleGeometryDataPath.createChildPath(faceAttributeMatrix->getName()).createChildPath(pMinDihedralAnglesName);
    // Create the face areas DataArray Action and store it into the resultOutputActions
    auto createArrayAction = std::make_unique<CreateArrayAction>(complex::DataType::float64, std::vector<usize>{triangleGeom->getNumberOfFaces()}, std::vector<usize>{1}, createArrayDataPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> TriangleDihedralAngleFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  auto pTriangleGeometryDataPath = filterArgs.value<DataPath>(k_TGeometryDataPath_Key);
  auto pMinDihedralAnglesName = filterArgs.value<std::string>(k_SurfaceMeshTriangleDihedralAnglesArrayName_Key);

  const TriangleGeom* triangleGeom = dataStructure.getDataAs<TriangleGeom>(pTriangleGeometryDataPath);
  const AttributeMatrix* faceAttributeMatrix = triangleGeom->getFaceAttributeMatrix();
  const DataPath dihedralAnglesArrayPath = pTriangleGeometryDataPath.createChildPath(faceAttributeMatrix->getName()).createChildPath(pMinDihedralAnglesName);
  auto& dihedralAngles = dataStructure.getDataRefAs<Float64Array>(dihedralAnglesArrayPath);

  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0ULL, static_cast<size_t>(triangleGeom->getNumberOfFaces()));
  dataAlg.execute(CalculateDihedralAnglesImpl(triangleGeom, dihedralAngles, shouldCancel));

  return {};
}
} // namespace complex
