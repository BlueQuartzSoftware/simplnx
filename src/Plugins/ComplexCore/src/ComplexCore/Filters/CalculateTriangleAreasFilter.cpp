#include "CalculateTriangleAreasFilter.hpp"

#include "complex/Common/ComplexRange.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/IGeometry.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Utilities/Math/MatrixMath.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"

using namespace complex;

namespace
{
/**
 * @brief The CalculateAreasImpl class implements a threaded algorithm that computes the area of each
 * triangle for a set of triangles
 */
class CalculateAreasImpl
{
public:
  CalculateAreasImpl(const IGeometry::SharedVertexList& nodes, const IGeometry::SharedTriList& triangles, Float64Array& Areas)
  : m_Nodes(nodes)
  , m_Triangles(triangles)
  , m_Areas(Areas)
  {
  }
  virtual ~CalculateAreasImpl() = default;

  void convert(size_t start, size_t end) const
  {
    IGeometry::MeshIndexType nIdx0 = 0;
    IGeometry::MeshIndexType nIdx1 = 0;
    IGeometry::MeshIndexType nIdx2 = 0;
    std::array<float, 3> vecA = {0.0f, 0.0f, 0.0f};
    std::array<float, 3> vecB = {0.0f, 0.0f, 0.0f};
    std::array<float, 3> cross = {0.0f, 0.0f, 0.0f};
    for(size_t i = start; i < end; i++)
    {
      nIdx0 = m_Triangles[i * 3];
      nIdx1 = m_Triangles[i * 3 + 1];
      nIdx2 = m_Triangles[i * 3 + 2];

      std::array<float, 3> A = {m_Nodes[nIdx0 * 3], m_Nodes[nIdx0 * 3 + 1], m_Nodes[nIdx0 * 3 + 2]};
      std::array<float, 3> B = {m_Nodes[nIdx1 * 3], m_Nodes[nIdx1 * 3 + 1], m_Nodes[nIdx1 * 3 + 2]};
      std::array<float, 3> C = {m_Nodes[nIdx2 * 3], m_Nodes[nIdx2 * 3 + 1], m_Nodes[nIdx2 * 3 + 2]};

      MatrixMath::Subtract3x1s(A.data(), B.data(), vecA.data());
      MatrixMath::Subtract3x1s(A.data(), C.data(), vecB.data());
      MatrixMath::CrossProduct(vecA.data(), vecB.data(), cross.data());
      float area = 0.5F * MatrixMath::Magnitude3x1(cross.data());
      m_Areas[i] = area;
    }
  }

  void operator()(const ComplexRange& range) const
  {
    convert(range.min(), range.max());
  }

private:
  const IGeometry::SharedVertexList& m_Nodes;
  const IGeometry::SharedTriList& m_Triangles;
  Float64Array& m_Areas;
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string CalculateTriangleAreasFilter::name() const
{
  return FilterTraits<CalculateTriangleAreasFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string CalculateTriangleAreasFilter::className() const
{
  return FilterTraits<CalculateTriangleAreasFilter>::className;
}

//------------------------------------------------------------------------------
Uuid CalculateTriangleAreasFilter::uuid() const
{
  return FilterTraits<CalculateTriangleAreasFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string CalculateTriangleAreasFilter::humanName() const
{
  return "Calculate Triangle Areas";
}

//------------------------------------------------------------------------------
std::vector<std::string> CalculateTriangleAreasFilter::defaultTags() const
{
  return {"#Surface Meshing", "#Misc", "#Triangle Geometry"};
}

//------------------------------------------------------------------------------
Parameters CalculateTriangleAreasFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriangleGeometryDataPath_Key, "Triangle Geometry", "The complete path to the Geometry for which to calculate the face areas", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Triangle}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_CalculatedAreasDataPath_Key, "Calculated Face Areas", "The complete path to the array storing the calculated face areas", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CalculateTriangleAreasFilter::clone() const
{
  return std::make_unique<CalculateTriangleAreasFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CalculateTriangleAreasFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                     const std::atomic_bool& shouldCancel) const
{
  auto pCalculatedAreasDataPath = filterArgs.value<DataPath>(k_CalculatedAreasDataPath_Key);
  auto pTriangleGeometryDataPath = filterArgs.value<DataPath>(k_TriangleGeometryDataPath_Key);

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  const TriangleGeom* triangleGeom = dataStructure.getDataAs<TriangleGeom>(pTriangleGeometryDataPath);
  if(triangleGeom != nullptr)
  {
    // Create the face areas DataArray Action and store it into the resultOutputActions
    auto createArrayAction = std::make_unique<CreateArrayAction>(complex::DataType::float64, std::vector<usize>{triangleGeom->getNumberOfFaces()}, std::vector<usize>{1}, pCalculatedAreasDataPath);
    resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> CalculateTriangleAreasFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel) const
{
  auto pCalculatedAreasDataPath = filterArgs.value<DataPath>(k_CalculatedAreasDataPath_Key);
  auto pTriangleGeometryDataPath = filterArgs.value<DataPath>(k_TriangleGeometryDataPath_Key);

  TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(pTriangleGeometryDataPath);
  Float64Array& faceAreas = dataStructure.getDataRefAs<Float64Array>(pCalculatedAreasDataPath);
  // Associate the calculated face areas with the Face Data in the Triangle Geometry
  triangleGeom.getLinkedGeometryData().addFaceData(pCalculatedAreasDataPath);

  // Parallel algorithm to find duplicate nodes
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0ULL, static_cast<size_t>(triangleGeom.getNumberOfFaces()));
  dataAlg.execute(::CalculateAreasImpl(*(triangleGeom.getVertices()), *(triangleGeom.getFaces()), faceAreas));

  return {};
}
} // namespace complex
