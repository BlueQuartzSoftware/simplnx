#include "CalculateTriangleAreasFilter.hpp"

#include "complex/Common/Range.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/IGeometry.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Utilities/Math/MatrixMath.hpp"

#include "complex/Utilities/SIMPLConversion.hpp"

#include "complex/Utilities/ParallelDataAlgorithm.hpp"

using namespace complex;

namespace
{
constexpr complex::int32 k_MissingFeatureAttributeMatrix = -75769;

/**
 * @brief The CalculateAreasImpl class implements a threaded algorithm that computes the area of each
 * triangle for a set of triangles
 */
class CalculateAreasImpl
{
public:
  CalculateAreasImpl(const TriangleGeom* triangleGeom, Float64Array& areas, const std::atomic_bool& shouldCancel)
  : m_TriangleGeom(triangleGeom)
  , m_Areas(areas)
  , m_ShouldCancel(shouldCancel)
  {
  }
  virtual ~CalculateAreasImpl() = default;

  void convert(size_t start, size_t end) const
  {
    std::array<float, 3> cross = {0.0f, 0.0f, 0.0f};
    for(size_t triangleIndex = start; triangleIndex < end; triangleIndex++)
    {
      if(m_ShouldCancel)
      {
        break;
      }
      std::array<Point3Df, 3> vertCoords;
      m_TriangleGeom->getFaceCoordinates(triangleIndex, vertCoords);

      auto vecA = (vertCoords[0] - vertCoords[1]).toArray();
      auto vecB = (vertCoords[0] - vertCoords[2]).toArray();

      MatrixMath::CrossProduct(vecA.data(), vecB.data(), cross.data());

      m_Areas[triangleIndex] = 0.5F * MatrixMath::Magnitude3x1(cross.data());
    }
  }

  void operator()(const Range& range) const
  {
    convert(range.min(), range.max());
  }

private:
  const TriangleGeom* m_TriangleGeom = nullptr;
  Float64Array& m_Areas;
  const std::atomic_bool& m_ShouldCancel;
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
  return {className(), "Surface Meshing", "Misc", "Triangle Geometry"};
}

//------------------------------------------------------------------------------
Parameters CalculateTriangleAreasFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Required Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriangleGeometryDataPath_Key, "Triangle Geometry", "The complete path to the Geometry for which to calculate the face areas", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Triangle}));
  params.insertSeparator(Parameters::Separator{"Created Face Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CalculatedAreasDataPath_Key, "Created Face Areas", "The complete path to the array storing the calculated face areas", "Face Areas"));

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
  auto pTriangleGeometryDataPath = filterArgs.value<DataPath>(k_TriangleGeometryDataPath_Key);
  auto pCalculatedAreasName = filterArgs.value<std::string>(k_CalculatedAreasDataPath_Key);

  std::vector<PreflightValue> preflightUpdatedValues;

  complex::Result<OutputActions> resultOutputActions;

  // The parameter will have validated that the Triangle Geometry exists and is the correct type
  const TriangleGeom* triangleGeom = dataStructure.getDataAs<TriangleGeom>(pTriangleGeometryDataPath);

  // Get the Face AttributeMatrix from the Geometry (It should have been set at construction of the Triangle Geometry)
  const AttributeMatrix* faceAttributeMatrix = triangleGeom->getFaceAttributeMatrix();
  if(faceAttributeMatrix == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{
        Error{k_MissingFeatureAttributeMatrix, fmt::format("Could not find Triangle Face Attribute Matrix with in the Triangle Geometry '{}'", pTriangleGeometryDataPath.toString())}})};
  }
  // Instantiate and move the action that will create the output array
  {
    DataPath createArrayDataPath = pTriangleGeometryDataPath.createChildPath(faceAttributeMatrix->getName()).createChildPath(pCalculatedAreasName);
    // Create the face areas DataArray Action and store it into the resultOutputActions
    auto createArrayAction = std::make_unique<CreateArrayAction>(complex::DataType::float64, std::vector<usize>{triangleGeom->getNumberOfFaces()}, std::vector<usize>{1}, createArrayDataPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> CalculateTriangleAreasFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel) const
{
  auto pCalculatedAreasName = filterArgs.value<std::string>(k_CalculatedAreasDataPath_Key);
  auto pTriangleGeometryDataPath = filterArgs.value<DataPath>(k_TriangleGeometryDataPath_Key);

  const TriangleGeom* triangleGeom = dataStructure.getDataAs<TriangleGeom>(pTriangleGeometryDataPath);
  const AttributeMatrix* faceAttributeMatrix = triangleGeom->getFaceAttributeMatrix();

  DataPath pCalculatedAreasDataPath = pTriangleGeometryDataPath.createChildPath(faceAttributeMatrix->getName()).createChildPath(pCalculatedAreasName);
  auto& faceAreas = dataStructure.getDataRefAs<Float64Array>(pCalculatedAreasDataPath);

  // Parallel algorithm to find duplicate nodes
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0ULL, static_cast<size_t>(triangleGeom->getNumberOfFaces()));
  dataAlg.execute(CalculateAreasImpl(triangleGeom, faceAreas, shouldCancel));

  return {};
}
} // namespace complex

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_SurfaceMeshTriangleAreasArrayPathKey = "SurfaceMeshTriangleAreasArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> CalculateTriangleAreasFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = CalculateTriangleAreasFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayCreationFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshTriangleAreasArrayPathKey, k_CalculatedAreasDataPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
