#include "IterativeClosestPointFilter.hpp"

#include "SimplnxCore/utils/nanoflann.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataPathSelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <Eigen/Geometry>

namespace nx::core
{
namespace
{
constexpr int32 k_MissingMovingVertex = -4500;
constexpr int32 k_MissingTargetVertex = -4501;
constexpr int32 k_BadNumIterations = -4502;
constexpr int32 k_MissingVertices = -4503;
constexpr int32 k_EmpyVertices = -4506;

template <typename Derived>
struct VertexGeomAdaptor
{
  const Derived& obj;
  Float32Array* verts;
  size_t m_NumComponents = 0;
  size_t m_NumTuples = 0;

  explicit VertexGeomAdaptor(const Derived& obj_)
  : obj(obj_)
  {
    // These values never change for the lifetime of this object so cache them now.
    verts = derived()->getVertices();
    m_NumComponents = verts->getNumberOfComponents();
    m_NumTuples = verts->getNumberOfTuples();
  }

  inline const Derived& derived() const
  {
    return obj;
  }

  inline usize kdtree_get_point_count() const
  {
    return m_NumTuples;
  }

  inline float kdtree_get_pt(const usize idx, const usize dim) const
  {
    auto offset = idx * m_NumComponents;
    return (*verts)[offset + dim];
  }

  template <class BBOX>
  bool kdtree_get_bbox(BBOX& /*bb*/) const
  {
    return false;
  }
};
} // namespace

//------------------------------------------------------------------------------
std::string IterativeClosestPointFilter::name() const
{
  return FilterTraits<IterativeClosestPointFilter>::name;
}

//------------------------------------------------------------------------------
std::string IterativeClosestPointFilter::className() const
{
  return FilterTraits<IterativeClosestPointFilter>::className;
}

//------------------------------------------------------------------------------
Uuid IterativeClosestPointFilter::uuid() const
{
  return FilterTraits<IterativeClosestPointFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string IterativeClosestPointFilter::humanName() const
{
  return "Iterative Closest Point";
}

//------------------------------------------------------------------------------
std::vector<std::string> IterativeClosestPointFilter::defaultTags() const
{
  return {className(), "Transformation", "Align", "Geometry", "ICP"};
}

//------------------------------------------------------------------------------
Parameters IterativeClosestPointFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<UInt64Parameter>(k_NumIterations_Key, "Number of Iterations", "The number of times to run the algorithm [more increases accuracy]", 1));
  params.insert(std::make_unique<BoolParameter>(k_ApplyTransformation_Key, "Apply Transformation to Moving Geometry", "If checked, geometry will be updated implicitly", false));

  params.insertSeparator(Parameters::Separator{"Required Data Objects"});
  params.insert(std::make_unique<DataPathSelectionParameter>(k_MovingVertexPath_Key, "Moving Vertex Geometry", "The geometry to align [mutable]", DataPath()));
  params.insert(std::make_unique<DataPathSelectionParameter>(k_TargetVertexPath_Key, "Target Vertex Geometry", "The geometry to be matched against [immutable]", DataPath()));

  params.insertSeparator(Parameters::Separator{"Created Data Objects"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_TransformArrayPath_Key, "Output Transform Array", "This is the array to store the transform matrix in", DataPath()));
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer IterativeClosestPointFilter::clone() const
{
  return std::make_unique<IterativeClosestPointFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult IterativeClosestPointFilter::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto movingVertexPath = args.value<DataPath>(k_MovingVertexPath_Key);
  auto targetVertexPath = args.value<DataPath>(k_TargetVertexPath_Key);
  auto numIterations = args.value<uint64>(k_NumIterations_Key);
  auto transformArrayPath = args.value<DataPath>(k_TransformArrayPath_Key);

  if(data.getDataAs<VertexGeom>(movingVertexPath) == nullptr)
  {
    auto ss = fmt::format("Moving Vertex Geometry not found at path: {}", movingVertexPath.toString());
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingMovingVertex, ss}})};
  }
  if(data.getDataAs<VertexGeom>(targetVertexPath) == nullptr)
  {
    auto ss = fmt::format("Target Vertex Geometry not found at path: {}", targetVertexPath.toString());
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingTargetVertex, ss}})};
  }

  if(numIterations < 1)
  {
    auto ss = fmt::format("Must perform at least 1 iterations");
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_BadNumIterations, ss}})};
  }

  usize numTuples = 1;
  auto action = std::make_unique<CreateArrayAction>(DataType::float32, std::vector<usize>{numTuples}, std::vector<usize>{16}, transformArrayPath);

  OutputActions actions;
  actions.appendAction(std::move(action));

  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> IterativeClosestPointFilter::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  auto movingVertexPath = args.value<DataPath>(k_MovingVertexPath_Key);
  auto targetVertexPath = args.value<DataPath>(k_TargetVertexPath_Key);
  auto numIterations = args.value<uint64>(k_NumIterations_Key);
  auto applyTransformation = args.value<bool>(k_ApplyTransformation_Key);
  auto transformArrayPath = args.value<DataPath>(k_TransformArrayPath_Key);

  auto movingVertexGeom = data.getDataAs<VertexGeom>(movingVertexPath);
  auto targetVertexGeom = data.getDataAs<VertexGeom>(targetVertexPath);

  if(movingVertexGeom == nullptr)
  {
    auto ss = fmt::format("Moving Vertex Geometry not found at path '{}'", movingVertexPath.toString());
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingVertices, ss}})};
  }
  if(targetVertexGeom == nullptr)
  {
    auto ss = fmt::format("Target Vertex Geometry not found at path '{}'", targetVertexPath.toString());
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingVertices, ss}})};
  }

  if(movingVertexGeom->getVertices() == nullptr)
  {
    auto ss = fmt::format("Moving Vertex Geometry does not contain a vertex array");
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingVertices, ss}})};
  }
  if(targetVertexGeom->getVertices() == nullptr)
  {
    auto ss = fmt::format("Target Vertex Geometry does not contain a vertex array");
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingVertices, ss}})};
  }

  Float32Array& movingVerticesRef = *(movingVertexGeom->getVertices());
  if(movingVerticesRef.empty())
  {
    auto ss = fmt::format("Moving Vertex Geometry does not contain any vertices");
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_EmpyVertices, ss}})};
  }
  Float32Array& targetVerticesRef = *(targetVertexGeom->getVertices());
  if(targetVerticesRef.empty())
  {
    auto ss = fmt::format("Target Vertex Geometry does not contain any vertices");
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_EmpyVertices, ss}})};
  }
  auto* movingStore = movingVerticesRef.getDataStore();
  std::vector<float32> movingVector(movingStore->begin(), movingStore->end());
  float32* movingCopyPtr = movingVector.data();
  DataStructure tmp;

  usize numMovingVerts = movingVertexGeom->getNumberOfVertices();
  std::vector<float32> dynTarget(numMovingVerts * 3, 0.0F);
  float* dynTargetPtr = dynTarget.data();

  using Adaptor = VertexGeomAdaptor<VertexGeom*>;
  const Adaptor adaptor(targetVertexGeom);

  messageHandler("Building kd-tree index...");

  using KDtree = nanoflann::KDTreeSingleIndexAdaptor<nanoflann::L2_Adaptor<float32, Adaptor>, Adaptor, 3>;
  KDtree index(3, adaptor, nanoflann::KDTreeSingleIndexAdaptorParams(30));
  index.buildIndex();

  usize iters = numIterations;
  const usize nn = 1;

  typedef Eigen::Matrix<float, 3, Eigen::Dynamic, Eigen::ColMajor> PointCloud;
  typedef Eigen::Matrix<float, 4, 4, Eigen::ColMajor> UmeyamaTransform;

  UmeyamaTransform globalTransform;
  globalTransform << 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1;

  int64 progIncrement = iters / 100;
  int64 prog = 1;
  int64 progressInt = 0;
  int64 counter = 0;

  for(usize i = 0; i < iters; i++)
  {
    if(shouldCancel)
    {
      return {};
    }

    for(usize j = 0; j < numMovingVerts; j++)
    {
      usize identifier;
      float dist;
      nanoflann::KNNResultSet<float> results(nn);
      results.init(&identifier, &dist);
      index.findNeighbors(results, movingCopyPtr + (3 * j), nanoflann::SearchParams());
      dynTargetPtr[3 * j + 0] = targetVerticesRef[3 * identifier + 0];
      dynTargetPtr[3 * j + 1] = targetVerticesRef[3 * identifier + 1];
      dynTargetPtr[3 * j + 2] = targetVerticesRef[3 * identifier + 2];
    }

    Eigen::Map<PointCloud> moving_(movingCopyPtr, 3, numMovingVerts);
    Eigen::Map<PointCloud> target_(dynTargetPtr, 3, numMovingVerts);

    UmeyamaTransform transform = Eigen::umeyama(moving_, target_, false);

    for(usize j = 0; j < numMovingVerts; j++)
    {
      Eigen::Vector4f position(movingCopyPtr[3 * j + 0], movingCopyPtr[3 * j + 1], movingCopyPtr[3 * j + 2], 1);
      Eigen::Vector4f transformedPosition = transform * position;
      std::memcpy(movingCopyPtr + (3 * j), transformedPosition.data(), sizeof(float) * 3);
    }
    // Update the global transform
    globalTransform = transform * globalTransform;

    if(counter > prog)
    {
      progressInt = static_cast<int64>((static_cast<float>(counter) / iters) * 100.0f);
      std::string ss = fmt::format("Performing Registration Iterations || {}% Completed", progressInt);
      messageHandler(ss);
      prog = prog + progIncrement;
    }
    counter++;
  }

  auto* transformPtr = data.getDataAs<Float32Array>(transformArrayPath)->getDataStore();

  if(applyTransformation)
  {
    for(usize j = 0; j < numMovingVerts; j++)
    {
      Eigen::Vector4f position(movingVerticesRef[3 * j + 0], movingVerticesRef[3 * j + 1], movingVerticesRef[3 * j + 2], 1);
      Eigen::Vector4f transformedPosition = globalTransform * position;
      for(usize k = 0; k < 3; k++)
      {
        movingVerticesRef[3 * j + k] = transformedPosition.data()[k];
      }
    }
  }

  globalTransform.transposeInPlace();
  for(usize j = 0; j < 16; j++)
  {
    (*transformPtr)[j] = globalTransform.data()[j];
  }

  return {};
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_MovingVertexGeometryKey = "MovingVertexGeometry";
constexpr StringLiteral k_TargetVertexGeometryKey = "TargetVertexGeometry";
constexpr StringLiteral k_IterationsKey = "Iterations";
constexpr StringLiteral k_ApplyTransformKey = "ApplyTransform";
constexpr StringLiteral k_TransformAttributeMatrixNameKey = "TransformAttributeMatrixName";
constexpr StringLiteral k_TransformArrayNameKey = "TransformArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> IterativeClosestPointFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = IterativeClosestPointFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_MovingVertexGeometryKey, k_MovingVertexPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_TargetVertexGeometryKey, k_TargetVertexPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<uint64>>(args, json, SIMPL::k_IterationsKey, k_NumIterations_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_ApplyTransformKey, k_ApplyTransformation_Key));
  // Transform attribute matrix parameter is not applicable in NX
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringToDataPathFilterParameterConverter>(args, json, SIMPL::k_TransformArrayNameKey, k_TransformArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
