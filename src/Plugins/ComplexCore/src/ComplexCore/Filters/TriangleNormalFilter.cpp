#include "TriangleNormalFilter.hpp"

#include "complex/Common/Range.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Utilities/Math/MatrixMath.hpp"
#include "complex/Utilities/MessageUtilities.hpp"
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
  CalculateNormalsImpl(const TriangleGeom* triangleGeom, Float64Array& normals, const std::atomic_bool& shouldCancel, ThreadSafeMessenger& messenger)
  : m_TriangleGeom(triangleGeom)
  , m_Normals(normals)
  , m_ShouldCancel(shouldCancel)
  , m_Messenger(messenger)
  {
  }
  virtual ~CalculateNormalsImpl() = default;

  void generate(size_t start, size_t end) const
  {
    const auto progressIncrement = m_Messenger.getProgressIncrement();
    usize progressCount = 0;

    std::array<float32, 3> normal = {0.0f, 0.0f, 0.0f};
    for(size_t triangleIndex = start; triangleIndex < end; triangleIndex++)
    {
      if(m_ShouldCancel)
      {
        break;
      }

      std::array<Point3Df, 3> vertCoords;
      m_TriangleGeom->getFaceCoordinates(triangleIndex, vertCoords);

      auto vecA = (vertCoords[1] - vertCoords[0]).toArray();
      auto vecB = (vertCoords[2] - vertCoords[0]).toArray();

      MatrixMath::CrossProduct(vecA.data(), vecB.data(), normal.data());
      MatrixMath::Normalize3x1(normal.data());

      m_Normals[triangleIndex * 3] = static_cast<float64>(normal[0]);
      m_Normals[triangleIndex * 3 + 1] = static_cast<float64>(normal[1]);
      m_Normals[triangleIndex * 3 + 2] = static_cast<float64>(normal[2]);

      progressCount++;
      if(progressCount > progressIncrement)
      {
        m_Messenger.updateProgress(progressCount);
      }
    }
    m_Messenger.updateProgress(progressCount);
  }

  void operator()(const Range& range) const
  {
    generate(range.min(), range.max());
  }

private:
  const TriangleGeom* m_TriangleGeom = nullptr;
  Float64Array& m_Normals;
  const std::atomic_bool& m_ShouldCancel;
  ThreadSafeMessenger& m_Messenger;
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
  return "Calculate Triangle Normals";
}

//------------------------------------------------------------------------------
std::vector<std::string> TriangleNormalFilter::defaultTags() const
{
  return {"Surface Meshing", "Misc"};
}

//------------------------------------------------------------------------------
Parameters TriangleNormalFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Required Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriGeometryDataPath_Key, "Triangle Geometry", "The complete path to the Geometry for which to calculate the normals", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Triangle}));
  params.insertSeparator(Parameters::Separator{"Created Face Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_SurfaceMeshTriangleNormalsArrayPath_Key, "Created Face Normals", "The complete path to the array storing the calculated normals", "Face Normals"));

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
  auto pNormalsName = filterArgs.value<std::string>(k_SurfaceMeshTriangleNormalsArrayPath_Key);

  const TriangleGeom* triangleGeom = dataStructure.getDataAs<TriangleGeom>(pTriangleGeometryDataPath);
  const AttributeMatrix* faceAttributeMatrix = triangleGeom->getFaceAttributeMatrix();

  DataPath pNormalsArrayPath = pTriangleGeometryDataPath.createChildPath(faceAttributeMatrix->getName()).createChildPath(pNormalsName);
  auto& normals = dataStructure.getDataRefAs<Float64Array>(pNormalsArrayPath);

  usize totalElements = triangleGeom->getNumberOfFaces();
  ThreadSafeMessenger messenger(messageHandler, "Finding Normals...");
  messenger.setTotalElements(totalElements);
  messenger.setProgressIncrement(totalElements / 100);

  // Parallel algorithm to find duplicate nodes
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0ULL, totalElements);
  dataAlg.execute(CalculateNormalsImpl(triangleGeom, normals, shouldCancel, messenger));

  return {};
}
} // namespace complex
