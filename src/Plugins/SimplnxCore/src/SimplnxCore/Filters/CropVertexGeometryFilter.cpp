#include "CropVertexGeometryFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateVertexGeometryAction.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

namespace nx::core
{
namespace
{
struct CopyDataToCroppedGeometryFunctor
{
  template <typename T>
  void operator()(const IDataArray* inDataRef, IDataArray* outDataRef, const std::vector<int64>& croppedPoints)
  {
    const auto& inputData = inDataRef->template getIDataStoreRefAs<AbstractDataStore<T>>();
    auto& croppedData = outDataRef->template getIDataStoreRefAs<AbstractDataStore<T>>();

    usize nComps = inDataRef->getNumberOfComponents();

    for(std::vector<int64>::size_type i = 0; i < croppedPoints.size(); i++)
    {
      for(usize d = 0; d < nComps; d++)
      {
        usize tmpIndex = nComps * i + d;
        usize ptrIndex = nComps * croppedPoints[i] + d;
        croppedData[tmpIndex] = inputData[ptrIndex];
      }
    }
  }
};
} // namespace

//------------------------------------------------------------------------------
std::string CropVertexGeometryFilter::name() const
{
  return FilterTraits<CropVertexGeometryFilter>::name;
}

//------------------------------------------------------------------------------
std::string CropVertexGeometryFilter::className() const
{
  return FilterTraits<CropVertexGeometryFilter>::className;
}

//------------------------------------------------------------------------------
Uuid CropVertexGeometryFilter::uuid() const
{
  return FilterTraits<CropVertexGeometryFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string CropVertexGeometryFilter::humanName() const
{
  return "Crop Geometry (Vertex)";
}

//------------------------------------------------------------------------------
std::vector<std::string> CropVertexGeometryFilter::defaultTags() const
{
  return {className(), "Crop", "Vertex Geometry", "Geometry", "Memory Management", "Cut"};
}

//------------------------------------------------------------------------------
Parameters CropVertexGeometryFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedVertexGeometryPath_Key, "Vertex Geometry to Crop", "DataPath to target VertexGeom", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Vertex}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedVertexGeometryPath_Key, "Cropped Vertex Geometry", "Created VertexGeom path", DataPath{}));
  params.insert(std::make_unique<DataObjectNameParameter>(k_VertexAttributeMatrixName_Key, "Vertex Data Name", "Name of the vertex data AttributeMatrix", INodeGeometry0D::k_VertexDataName));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_MinPos_Key, "Min Pos", "Minimum vertex position", std::vector<float32>{0.0f, 0.0f, 0.0f}, std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_MaxPos_Key, "Max Pos", "Maximum vertex position", std::vector<float32>{0.0f, 0.0f, 0.0f}, std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_TargetArrayPaths_Key, "Vertex Data Arrays to crop", "The complete path to all the vertex data arrays to crop", std::vector<DataPath>(),
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, nx::core::GetAllDataTypes()));
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CropVertexGeometryFilter::clone() const
{
  return std::make_unique<CropVertexGeometryFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CropVertexGeometryFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler,
                                                                 const std::atomic_bool& shouldCancel) const
{
  auto vertexGeomPath = args.value<DataPath>(k_SelectedVertexGeometryPath_Key);
  auto croppedGeomPath = args.value<DataPath>(k_CreatedVertexGeometryPath_Key);
  auto posMin = args.value<std::vector<float32>>(k_MinPos_Key);
  auto posMax = args.value<std::vector<float32>>(k_MaxPos_Key);
  auto targetArrays = args.value<std::vector<DataPath>>(k_TargetArrayPaths_Key);
  auto vertexDataName = args.value<std::string>(k_VertexAttributeMatrixName_Key);

  auto xMin = posMin[0];
  auto yMin = posMin[1];
  auto zMin = posMin[2];
  auto xMax = posMax[0];
  auto yMax = posMax[1];
  auto zMax = posMax[2];

  const auto& vertexGeom = dataStructure.getDataRefAs<VertexGeom>(vertexGeomPath);

  OutputActions actions;

  {
    std::vector<Error> errors;
    bool xDimError = (xMax == xMin);
    bool yDimError = (yMax == yMin);
    bool zDimError = (zMax == zMin);
    if(xDimError)
    {
      std::string ss = fmt::format("X Max ({}) is equal to X Min ({}). There would be no points remaining in the geometry.", xMax, xMin);
      errors.push_back(Error{-58550, std::move(ss)});
    }
    if(yDimError)
    {
      std::string ss = fmt::format("Y Max ({}) is equal to Y Min ({}). There would be no points remaining in the geometry.", yMax, yMin);
      errors.push_back(Error{-58551, std::move(ss)});
    }
    if(zDimError)
    {
      std::string ss = fmt::format("Z Max ({}) is equal to Z Min ({}). There would be no points remaining in the geometry.", zMax, zMin);
      errors.push_back(Error{-58552, std::move(ss)});
    }
    if(xDimError || yDimError || zDimError)
    {
      return PreflightResult{{nonstd::make_unexpected(errors)}};
    }
  }

  {
    std::vector<Error> errors;
    bool xDimError = (xMax < xMin);
    bool yDimError = (yMax < yMin);
    bool zDimError = (zMax < zMin);
    if(xDimError)
    {
      std::string ss = fmt::format("X Max ({}) less than X Min ({})", xMax, xMin);
      errors.push_back(Error{-58553, std::move(ss)});
    }
    if(yDimError)
    {
      std::string ss = fmt::format("Y Max ({}) less than Y Min ({})", yMax, yMin);
      errors.push_back(Error{-58554, std::move(ss)});
    }
    if(zDimError)
    {
      std::string ss = fmt::format("Z Max ({}) less than Z Min ({})", zMax, zMin);
      errors.push_back(Error{-58555, std::move(ss)});
    }
    if(xDimError || yDimError || zDimError)
    {
      return PreflightResult{{nonstd::make_unexpected(errors)}};
    }
  }

  auto* vertexAM = vertexGeom.getVertexAttributeMatrix();
  if(vertexAM == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-58556, "Could not find vertex data AttributeMatrix in selected Vertex Geometry"), {}};
  }
  auto tupleShape = vertexAM->getShape();
  usize numTuples = std::accumulate(tupleShape.cbegin(), tupleShape.cend(), static_cast<usize>(1), std::multiplies<>());
  auto action = std::make_unique<CreateVertexGeometryAction>(croppedGeomPath, numTuples, vertexDataName, CreateVertexGeometryAction::k_SharedVertexListName);
  DataPath croppedVertexDataPath = action->getVertexDataPath();
  actions.appendAction(std::move(action));

  for(auto&& targetArrayPath : targetArrays)
  {
    auto& targetArray = dataStructure.getDataRefAs<IDataArray>(targetArrayPath);

    DataType type = targetArray.getDataType();
    auto tDims = targetArray.getNumberOfTuples();
    auto cDims = targetArray.getNumberOfComponents();
    auto createArrayAction = std::make_unique<CreateArrayAction>(type, std::vector<usize>{tDims}, std::vector<usize>{cDims}, croppedVertexDataPath.createChildPath(targetArrayPath.getTargetName()));
    actions.appendAction(std::move(createArrayAction));
  }

  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> CropVertexGeometryFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel) const
{
  auto vertexGeomPath = args.value<DataPath>(k_SelectedVertexGeometryPath_Key);
  auto croppedGeomPath = args.value<DataPath>(k_CreatedVertexGeometryPath_Key);
  auto posMin = args.value<std::vector<float32>>(k_MinPos_Key);
  auto posMax = args.value<std::vector<float32>>(k_MaxPos_Key);
  auto targetArrays = args.value<std::vector<DataPath>>(k_TargetArrayPaths_Key);
  auto vertexDataName = args.value<std::string>(k_VertexAttributeMatrixName_Key);

  auto xMin = posMin[0];
  auto yMin = posMin[1];
  auto zMin = posMin[2];
  auto xMax = posMax[0];
  auto yMax = posMax[1];
  auto zMax = posMax[2];

  auto& vertices = dataStructure.getDataRefAs<VertexGeom>(vertexGeomPath);
  auto numVerts = static_cast<int64>(vertices.getNumberOfVertices());
  auto* verticesPtr = vertices.getVertices();
  auto& allVerts = verticesPtr->getDataStoreRef();
  std::vector<int64> croppedPoints;
  croppedPoints.reserve(numVerts);

  for(int64 i = 0; i < numVerts; i++)
  {
    if(shouldCancel)
    {
      return {};
    }
    if(allVerts[3 * i + 0] >= xMin && allVerts[3 * i + 0] <= xMax && allVerts[3 * i + 1] >= yMin && allVerts[3 * i + 1] <= yMax && allVerts[3 * i + 2] >= zMin && allVerts[3 * i + 2] <= zMax)
    {
      croppedPoints.push_back(i);
    }
  }

  croppedPoints.shrink_to_fit();

  auto& crop = dataStructure.getDataRefAs<VertexGeom>(croppedGeomPath);
  usize numTuples = croppedPoints.size();
  crop.resizeVertexList(numTuples);
  std::vector<usize> tDims = {numTuples};

  DataPath croppedVertexDataPath = croppedGeomPath.createChildPath(vertexDataName);
  auto& vertedDataAttMatrix = dataStructure.getDataRefAs<AttributeMatrix>(croppedVertexDataPath);
  vertedDataAttMatrix.resizeTuples(tDims);

  for(usize i = 0; i < numTuples; i++)
  {
    if(shouldCancel)
    {
      return {};
    }
    auto coords = vertices.getVertexCoordinate(croppedPoints[i]);
    crop.setVertexCoordinate(i, coords);
  }

  for(auto&& targetArrayPath : targetArrays)
  {
    DataPath destArrayPath(croppedVertexDataPath.createChildPath(targetArrayPath.getTargetName()));

    const auto* srcArray = dataStructure.getDataAs<IDataArray>(targetArrayPath);
    auto* destArray = dataStructure.getDataAs<IDataArray>(destArrayPath);

    ExecuteDataFunction(CopyDataToCroppedGeometryFunctor{}, srcArray->getDataType(), srcArray, destArray, croppedPoints);
  }

  return {};
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_DataContainerNameKey = "DataContainerName";
constexpr StringLiteral k_XMinKey = "XMin";
constexpr StringLiteral k_YMinKey = "YMin";
constexpr StringLiteral k_ZMinKey = "ZMin";
constexpr StringLiteral k_XMaxKey = "XMax";
constexpr StringLiteral k_YMaxKey = "YMax";
constexpr StringLiteral k_ZMaxKey = "ZMax";
constexpr StringLiteral k_CroppedDataContainerNameKey = "CroppedDataContainerName";
} // namespace SIMPL
} // namespace

Result<Arguments> CropVertexGeometryFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = CropVertexGeometryFilter().getDefaultArguments();

  std::vector<Result<>> results;

  // Convert 3 numeric inputs into Vec3 inputs
  results.push_back(SIMPLConversion::Convert3Parameters<SIMPLConversion::FloatToVec3FilterParameterConverter>(args, json, SIMPL::k_XMinKey, SIMPL::k_YMinKey, SIMPL::k_ZMinKey, k_MinPos_Key));
  results.push_back(SIMPLConversion::Convert3Parameters<SIMPLConversion::FloatToVec3FilterParameterConverter>(args, json, SIMPL::k_XMaxKey, SIMPL::k_YMaxKey, SIMPL::k_ZMaxKey, k_MaxPos_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_DataContainerNameKey, k_SelectedVertexGeometryPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerCreationFilterParameterConverter>(args, json, SIMPL::k_CroppedDataContainerNameKey, k_CreatedVertexGeometryPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
