#include "TriangleDihedralAngleFilter.hpp"

#include "complex/Common/Constants.hpp"
#include "complex/Common/Range.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/IGeometry.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
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
  CalculateDihedralAnglesImpl(const IGeometry::SharedVertexList& nodes, const IGeometry::SharedTriList& triangles, Float64Array& dihedralAngles, const std::atomic_bool& shouldCancel)
  : m_Nodes(nodes)
  , m_Triangles(triangles)
  , m_DihedralAngles(dihedralAngles)
  , m_ShouldCancel(shouldCancel)
  {
  }
  virtual ~CalculateDihedralAnglesImpl() = default;

  void generate(size_t start, size_t end) const
  {
    IGeometry::MeshIndexType nIdx0 = 0, nIdx1 = 0, nIdx2 = 0;
    // std::array<float64, 3> vectorEx = {x, y, z};  // coordintate example
    std::array<float64, 3> vecAB = {0.0f, 0.0f, 0.0f};
    std::array<float64, 3> vecAC = {0.0f, 0.0f, 0.0f};
    std::array<float64, 3> vecBC = {0.0f, 0.0f, 0.0f};
    for(size_t i = start; i < end; i++)
    {
      nIdx0 = m_Triangles[i * 3];
      nIdx1 = m_Triangles[i * 3 + 1];
      nIdx2 = m_Triangles[i * 3 + 2];
      std::array<float64, 3> node0 = {m_Nodes[nIdx0 * 3], m_Nodes[nIdx0 * 3 + 1], m_Nodes[nIdx0 * 3 + 2]};
      std::array<float64, 3> node1 = {m_Nodes[nIdx1 * 3], m_Nodes[nIdx1 * 3 + 1], m_Nodes[nIdx1 * 3 + 2]};
      std::array<float64, 3> node2 = {m_Nodes[nIdx2 * 3], m_Nodes[nIdx2 * 3 + 1], m_Nodes[nIdx2 * 3 + 2]};

      MatrixMath::Subtract3x1s(node0.data(), node1.data(), vecAB.data());
      MatrixMath::Subtract3x1s(node0.data(), node2.data(), vecAC.data());
      MatrixMath::Subtract3x1s(node1.data(), node2.data(), vecBC.data());

      float64 magAB = sqrtf(sumOfMultiplyCoords(vecAB, vecAB));
      float64 magAC = sqrtf(sumOfMultiplyCoords(vecAC, vecAC));
      float64 magBC = sqrtf(sumOfMultiplyCoords(vecBC, vecBC));

      if(magAB == 0.0f || magAC == 0.0f || magBC == 0.0f)
      {
        m_DihedralAngles[i] = std::nan("0");
      }
      else
      {
        std::vector<float64> dihedralAnglesVec;
        dihedralAnglesVec.push_back(k_radToDeg * acos((std::fabs(sumOfMultiplyCoords(vecAB, vecAC)) / (magAB * magAC))));
        // 180 - angle because AB points out of vertex and BC points into vertex, so angle is actually angle outside of triangle
        dihedralAnglesVec.push_back(180.0 - (k_radToDeg * acos((std::fabs(sumOfMultiplyCoords(vecAB, vecBC)) / (magAB * magBC)))));
        dihedralAnglesVec.push_back(k_radToDeg * acos((std::fabs(sumOfMultiplyCoords(vecBC, vecAC)) / (magBC * magAC))));

        m_DihedralAngles[i] = *std::min_element(dihedralAnglesVec.begin(), dihedralAnglesVec.end());
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
  const IGeometry::SharedVertexList& m_Nodes;
  const IGeometry::SharedTriList& m_Triangles;
  Float64Array& m_DihedralAngles;
  const std::atomic_bool& m_ShouldCancel;

  float64 sumOfMultiplyCoords(std::array<float64, 3> vec1, std::array<float64, 3> vec2) const
  {
    float64 result = 0.0f;
    for(int32 index = 0; index < 3; index++)
    {
      result += vec1.at(index) * vec2.at(index);
    }
    return result;
  }
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
  return "Find Minimum Triangle Dihedral Angle";
}

//------------------------------------------------------------------------------
std::vector<std::string> TriangleDihedralAngleFilter::defaultTags() const
{
  return {"#Surface Meshing", "#Misc", "#Statistics", "#Triangle"};
}

//------------------------------------------------------------------------------
Parameters TriangleDihedralAngleFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TGeometryDataPath_Key, "Triangle Geometry", "The complete path to the Geometry for which to calculate the dihedral angles", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Triangle}));
  params.insertSeparator(Parameters::Separator{"Created Face Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_SurfaceMeshTriangleDihedralAnglesArrayName_Key, "Face Dihedral Angles", "The name of the array storing the calculated dihedral angles",
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
  // Get the Face AttributeMatrix from the Geometry (It should have been set at construction of the Triangle Geometry)
  const AttributeMatrix* faceAttributeMatrix = triangleGeom->getFaceData();
  if(faceAttributeMatrix == nullptr)
  {
<<<<<<< HEAD
    return {nonstd::make_unexpected(std::vector<Error>{
        Error{k_MissingFeatureAttributeMatrix, fmt::format("Could not find Triangle Face Attribute Matrix with in the Triangle Geometry '{}'", pTriangleGeometryDataPath.toString())}})};
=======
    return {MakeErrorResult<OutputActions>(-9860, fmt::format("Cannot find the selected Triangle Geometry at path '{}'", pTriangleGeometryDataPath.toString()))};
  }
  const AttributeMatrix* faceData = triangleGeom->getFaceAttributeMatrix();
  if(faceData == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-9861, fmt::format("Cannot find the face data Attribute Matrix for the selected Triangle Geometry at path '{}'", pTriangleGeometryDataPath.toString()))};
>>>>>>> f788a2be (Geometry API: Rename methods to better convey what they do.)
  }

  // Instantiate and move the action that will create the output array
  {
    DataPath createArrayDataPath = pTriangleGeometryDataPath.createChildPath(faceAttributeMatrix->getName()).createChildPath(pMinDihedralAnglesName);
    // Create the face areas DataArray Action and store it into the resultOutputActions
    auto createArrayAction = std::make_unique<CreateArrayAction>(complex::DataType::float64, std::vector<usize>{triangleGeom->getNumberOfFaces()}, std::vector<usize>{1}, createArrayDataPath);
    resultOutputActions.value().actions.push_back(std::move(createArrayAction));
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

<<<<<<< HEAD
  const TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(pTriangleGeometryDataPath);
  const AttributeMatrix& faceAttributeMatrix = triangleGeom.getFaceDataRef();

  DataPath dihedralAnglesArrayPath = pTriangleGeometryDataPath.createChildPath(faceAttributeMatrix.getName()).createChildPath(pMinDihedralAnglesName);
  auto& dihedralAngles = dataStructure.getDataRefAs<Float64Array>(dihedralAnglesArrayPath);
=======
  TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(pTriangleGeometryDataPath);
  AttributeMatrix* faceData = triangleGeom.getFaceAttributeMatrix();
  DataPath dihedralAnglesArrayPath = pTriangleGeometryDataPath.createChildPath(faceData->getName()).createChildPath(pSurfaceMeshTriangleDihedralAnglesName);
  Float64Array& dihedralAngles = dataStructure.getDataRefAs<Float64Array>(dihedralAnglesArrayPath);
>>>>>>> f788a2be (Geometry API: Rename methods to better convey what they do.)

  ParallelDataAlgorithm dataAlg;
  dataAlg.setParallelizationEnabled(false);
  dataAlg.setRange(0ULL, static_cast<size_t>(triangleGeom.getNumberOfFaces()));
  dataAlg.execute(::CalculateDihedralAnglesImpl(*(triangleGeom.getVertices()), *(triangleGeom.getFaces()), dihedralAngles, shouldCancel));

  return {};
}
} // namespace complex
