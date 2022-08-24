#include "TriangleDihedralAngleFilter.hpp"

#include "complex/Common/ComplexRange.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/AbstractGeometry.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Utilities/Math/MatrixMath.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"

#include <algorithm>

using namespace complex;

namespace
{
constexpr float64 k_radToDeg = 180.0; // used for translating radians to degrees
/**
 * @brief The CalculateAreasImpl class implements a threaded algorithm that computes the normal of each
 * triangle for a set of triangles
 */
class CalculateDihedralAnglesImpl
{
public:
  CalculateDihedralAnglesImpl(const AbstractGeometry::SharedVertexList& nodes, const AbstractGeometry::SharedTriList& triangles, Float64Array& dihedralAngles)
  : m_Nodes(nodes)
  , m_Triangles(triangles)
  , m_DihedralAngles(dihedralAngles)
  {
  }
  virtual ~CalculateDihedralAnglesImpl() = default;

  void generate(size_t start, size_t end) const
  {
    AbstractGeometry::MeshIndexType nIdx0 = 0, nIdx1 = 0, nIdx2 = 0;
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

      auto magAB = sqrtf(sumOfMultiplyCoords(vecAB, vecAB));
      auto magAC = sqrtf(sumOfMultiplyCoords(vecAC, vecAC));
      auto magBC = sqrtf(sumOfMultiplyCoords(vecBC, vecBC));

      std::vector<float64> dihedralAnglesVec;
      dihedralAnglesVec.push_back(k_radToDeg * acos((sumOfMultiplyCoords(vecAB, vecAC) / (magAB * magAC))));
      // 180 - angle because AB points out of vertex and BC points into vertex, so angle is actually angle outside of triangle
      dihedralAnglesVec.push_back(180.0 - (k_radToDeg * acos((sumOfMultiplyCoords(vecAB, vecBC) / (magAB * magBC)))));
      dihedralAnglesVec.push_back(k_radToDeg * acos((sumOfMultiplyCoords(vecBC, vecAC) / (magBC * magAC))));

      m_DihedralAngles[i] = *std::min_element(dihedralAnglesVec.begin(), dihedralAnglesVec.end());
    }
  }

  void operator()(const ComplexRange& range) const
  {
    generate(range.min(), range.max());
  }

private:
  const AbstractGeometry::SharedVertexList& m_Nodes;
  const AbstractGeometry::SharedTriList& m_Triangles;
  Float64Array& m_DihedralAngles;

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
  return {"#Surface Meshing", "#Misc"};
}

//------------------------------------------------------------------------------
Parameters TriangleDihedralAngleFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TGeometryDataPath_Key, "Triangle Geometry", "The complete path to the Geometry for which to calculate the dihedral angles", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{AbstractGeometry::Type::Triangle}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SurfaceMeshTriangleDihedralAnglesArrayPath_Key, "Face Dihedral Angles",
                                                         "The complete path to the array storing the calculated dihedral angles", DataPath{}));

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
  auto pSurfaceMeshTriangleDihedralAnglesArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshTriangleDihedralAnglesArrayPath_Key);

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  const TriangleGeom* triangleGeom = dataStructure.getDataAs<TriangleGeom>(pTriangleGeometryDataPath);
  if(triangleGeom != nullptr)
  {
    auto createArrayAction =
        std::make_unique<CreateArrayAction>(complex::DataType::float64, std::vector<usize>{triangleGeom->getNumberOfFaces()}, std::vector<usize>{3}, pSurfaceMeshTriangleDihedralAnglesArrayPathValue);
    resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> TriangleDihedralAngleFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  auto pTriangleGeometryDataPath = filterArgs.value<DataPath>(k_TGeometryDataPath_Key);
  auto pSurfaceMeshTriangleDihedralAnglesArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshTriangleDihedralAnglesArrayPath_Key);

  TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(pTriangleGeometryDataPath);
  Float64Array& dihedralAngles = dataStructure.getDataRefAs<Float64Array>(pSurfaceMeshTriangleDihedralAnglesArrayPathValue);
  // Associate the calculated normals with the Face Data in the Triangle Geometry
  triangleGeom.getLinkedGeometryData().addFaceData(pSurfaceMeshTriangleDihedralAnglesArrayPathValue);

  // Parallel algorithm to find duplicate nodes
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0ULL, static_cast<size_t>(triangleGeom.getNumberOfFaces()));
  dataAlg.execute(::CalculateDihedralAnglesImpl(*(triangleGeom.getVertices()), *(triangleGeom.getFaces()), dihedralAngles));

  return {};
}
} // namespace complex
