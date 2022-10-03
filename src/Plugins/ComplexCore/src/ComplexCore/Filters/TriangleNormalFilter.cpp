#include "TriangleNormalFilter.hpp"

#include "complex/Common/Range.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Utilities/Math/MatrixMath.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"

using namespace complex;

namespace
{

constexpr complex::int32 k_MissingFeatureAttributeMatrix = -75969;

/**
 * @brief The CalculateAreasImpl class implements a threaded algorithm that computes the normal of each
 * triangle for a set of triangles
 */
class CalculateNormalsImpl
{
public:
  CalculateNormalsImpl(const IGeometry::SharedVertexList& nodes, const IGeometry::SharedTriList& triangles, Float64Array& normals)
  : m_Nodes(nodes)
  , m_Triangles(triangles)
  , m_Normals(normals)
  {
  }
  virtual ~CalculateNormalsImpl() = default;

  void generate(size_t start, size_t end) const
  {
    IGeometry::MeshIndexType nIdx0 = 0, nIdx1 = 0, nIdx2 = 0;
    std::array<float64, 3> vecA = {0.0f, 0.0f, 0.0f};
    std::array<float64, 3> vecB = {0.0f, 0.0f, 0.0f};
    std::array<float64, 3> normal = {0.0f, 0.0f, 0.0f};
    for(size_t i = start; i < end; i++)
    {
      nIdx0 = m_Triangles[i * 3];
      nIdx1 = m_Triangles[i * 3 + 1];
      nIdx2 = m_Triangles[i * 3 + 2];
      std::array<float64, 3> n0 = {m_Nodes[nIdx0 * 3], m_Nodes[nIdx0 * 3 + 1], m_Nodes[nIdx0 * 3 + 2]};
      std::array<float64, 3> n1 = {m_Nodes[nIdx1 * 3], m_Nodes[nIdx1 * 3 + 1], m_Nodes[nIdx1 * 3 + 2]};
      std::array<float64, 3> n2 = {m_Nodes[nIdx2 * 3], m_Nodes[nIdx2 * 3 + 1], m_Nodes[nIdx2 * 3 + 2]};

      MatrixMath::Subtract3x1s(n1.data(), n0.data(), vecA.data());
      MatrixMath::Subtract3x1s(n2.data(), n0.data(), vecB.data());
      MatrixMath::CrossProduct(vecA.data(), vecB.data(), normal.data());
      MatrixMath::Normalize3x1(normal.data());
      for(int32 count = 0; count < normal.size(); count++)
      {
        m_Normals[i * 3 + count] = static_cast<float64>(normal[count]);
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
  Float64Array& m_Normals;
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string TriangleNormalFilter::name() const
{
  return FilterTraits<TriangleNormalFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string TriangleNormalFilter::className() const
{
  return FilterTraits<TriangleNormalFilter>::className;
}

//------------------------------------------------------------------------------
Uuid TriangleNormalFilter::uuid() const
{
  return FilterTraits<TriangleNormalFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string TriangleNormalFilter::humanName() const
{
  return "Generate Triangle Normals";
}

//------------------------------------------------------------------------------
std::vector<std::string> TriangleNormalFilter::defaultTags() const
{
  return {"#Surface Meshing", "#Misc"};
}

//------------------------------------------------------------------------------
Parameters TriangleNormalFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter

  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriGeometryDataPath_Key, "Triangle Geometry", "The complete path to the Geometry for which to calculate the normals", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Triangle}));
  params.insertSeparator(Parameters::Separator{"Created Face Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_SurfaceMeshTriangleNormalsArrayPath_Key, "Face Normals", "The complete path to the array storing the calculated normals", "Face Normals"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer TriangleNormalFilter::clone() const
{
  return std::make_unique<TriangleNormalFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult TriangleNormalFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
{
  auto pTriangleGeometryDataPath = filterArgs.value<DataPath>(k_TriGeometryDataPath_Key);
  auto pNormalsArrayName = filterArgs.value<std::string>(k_SurfaceMeshTriangleNormalsArrayPath_Key);

  std::vector<PreflightValue> preflightUpdatedValues;

  complex::Result<OutputActions> resultOutputActions;

  const auto* triangleGeom = dataStructure.getDataAs<TriangleGeom>(pTriangleGeometryDataPath);
  // Get the Face AttributeMatrix from the Geometry (It should have been set at construction of the Triangle Geometry)
  const AttributeMatrix* faceAttributeMatrix = triangleGeom->getFaceAttributeMatrix();
  if(faceAttributeMatrix == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{
        Error{k_MissingFeatureAttributeMatrix, fmt::format("Could not find Triangle Face Attribute Matrix with in the Triangle Geometry '{}'", pTriangleGeometryDataPath.toString())}})};
  }
  // Instantiate and move the action that will create the output array
  {
    DataPath createArrayDataPath = pTriangleGeometryDataPath.createChildPath(faceAttributeMatrix->getName()).createChildPath(pNormalsArrayName);
    // Create the face areas DataArray Action and store it into the resultOutputActions
    auto createArrayAction = std::make_unique<CreateArrayAction>(complex::DataType::float64, std::vector<usize>{triangleGeom->getNumberOfFaces()}, std::vector<usize>{3}, createArrayDataPath);
    resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> TriangleNormalFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  auto pTriangleGeometryDataPath = filterArgs.value<DataPath>(k_TriGeometryDataPath_Key);
  auto pSurfaceMeshTriangleNormalsName = filterArgs.value<std::string>(k_SurfaceMeshTriangleNormalsArrayPath_Key);

  const TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(pTriangleGeometryDataPath);
  const AttributeMatrix& faceAttributeMatrix = triangleGeom.getFaceAttributeMatrixRef();

  DataPath pSurfaceMeshTriangleNormalsArrayPath = pTriangleGeometryDataPath.createChildPath(faceAttributeMatrix.getName()).createChildPath(pSurfaceMeshTriangleNormalsName);
  auto& normals = dataStructure.getDataRefAs<Float64Array>(pSurfaceMeshTriangleNormalsArrayPath);

  // Parallel algorithm to find duplicate nodes
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0ULL, static_cast<size_t>(triangleGeom.getNumberOfFaces()));
  dataAlg.execute(::CalculateNormalsImpl(*(triangleGeom.getVertices()), *(triangleGeom.getFaces()), normals));

  return {};
}
} // namespace complex
