#include "MinNeighborsFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

namespace nx::core
{
namespace
{
constexpr int64 k_TupleCountInvalidError = -250;
constexpr int64 k_MissingFeaturePhasesError = -251;
constexpr int32 k_InconsistentTupleCount = -252;
constexpr int32 k_FetchChildArrayError = -5559;

Result<> assignBadPoints(DataStructure& dataStructure, const Arguments& args, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
{
  auto imageGeomPath = args.value<DataPath>(MinNeighborsFilter::k_SelectedImageGeometryPath_Key);
  auto featureIdsPath = args.value<DataPath>(MinNeighborsFilter::k_FeatureIdsPath_Key);
  auto numNeighborsPath = args.value<DataPath>(MinNeighborsFilter::k_NumNeighborsPath_Key);
  auto ignoredVoxelArrayPaths = args.value<std::vector<DataPath>>(MinNeighborsFilter::k_IgnoredVoxelArrays_Key);
  auto cellDataAttrMatrix = featureIdsPath.getParent();

  auto& featureIdsArray = dataStructure.getDataRefAs<Int32Array>(featureIdsPath);
  auto& numNeighborsArray = dataStructure.getDataRefAs<Int32Array>(numNeighborsPath);
  auto& featureIds = featureIdsArray.getDataStoreRef();

  auto applyToSinglePhase = args.value<bool>(MinNeighborsFilter::k_ApplyToSinglePhase_Key);
  Int32Array* featurePhasesArray = nullptr;
  if(applyToSinglePhase)
  {
    auto featurePhasesPath = args.value<DataPath>(MinNeighborsFilter::k_FeaturePhasesPath_Key);
    featurePhasesArray = dataStructure.getDataAs<Int32Array>(featurePhasesPath);
  }

  usize totalPoints = featureIdsArray.getNumberOfTuples();
  SizeVec3 udims = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath).getDimensions();

  // This was checked up in the execute function (which is called before this function)
  // so if we got this far then all should be good with the return. We might get
  // an empty vector<> but that is OK.
  std::vector<DataPath> cellDataArrayPaths = nx::core::GetAllChildDataPaths(dataStructure, cellDataAttrMatrix, DataObject::Type::DataArray).value();

  std::array<int64, 3> dims = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  std::vector<int32_t> neighbors(totalPoints, -1);

  int32 good = 1;
  int32 current = 0;
  int32 most = 0;
  int64 neighborPoint = 0;
  usize numFeatures = numNeighborsArray.getNumberOfTuples();

  int64 neighborPointIdx[6] = {0, 0, 0, 0, 0, 0};
  neighborPointIdx[0] = -dims[0] * dims[1];
  neighborPointIdx[1] = -dims[0];
  neighborPointIdx[2] = -1;
  neighborPointIdx[3] = 1;
  neighborPointIdx[4] = dims[0];
  neighborPointIdx[5] = dims[0] * dims[1];

  usize counter = 1;
  int64 voxelIndex = 0;
  int64 kStride = 0;
  int64 jStride = 0;
  int32 featureName = 0;
  int32 feature = 0;
  int32 neighbor = 0;
  std::vector<int32> n(numFeatures + 1, 0);
  std::vector<size_t> badFeatureIdIndexes;

  int32_t progInt = 0;
  auto start = std::chrono::steady_clock::now();

  while(counter != 0)
  {
    auto now = std::chrono::steady_clock::now();
    // Only send updates every 1 second
    if(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 1000)
    {
      std::string message = fmt::format("Finding voxels to be assigned Counter = {}", counter);
      messageHandler(nx::core::IFilter::ProgressMessage{nx::core::IFilter::Message::Type::Info, message, progInt});
      start = now;
    }

    if(shouldCancel)
    {
      return {};
    }
    counter = 0;
    badFeatureIdIndexes.clear();
    for(int64 k = 0; k < dims[2]; k++)
    {
      kStride = dims[0] * dims[1] * k;
      for(int64 j = 0; j < dims[1]; j++)
      {
        jStride = dims[0] * j;
        for(int64 i = 0; i < dims[0]; i++)
        {
          voxelIndex = kStride + jStride + i;
          featureName = featureIds[voxelIndex];
          if(featureName < 0)
          {
            badFeatureIdIndexes.push_back(voxelIndex);
            counter++;
            current = 0;
            most = 0;
            for(int32 l = 0; l < 6; l++)
            {
              good = 1;
              neighborPoint = voxelIndex + neighborPointIdx[l];
              if(l == 0 && k == 0)
              {
                good = 0;
              }
              if(l == 5 && k == (dims[2] - 1))
              {
                good = 0;
              }
              if(l == 1 && j == 0)
              {
                good = 0;
              }
              if(l == 4 && j == (dims[1] - 1))
              {
                good = 0;
              }
              if(l == 2 && i == 0)
              {
                good = 0;
              }
              if(l == 3 && i == (dims[0] - 1))
              {
                good = 0;
              }
              if(good == 1)
              {
                feature = featureIds[neighborPoint];
                if(feature >= 0)
                {
                  n[feature]++;
                  current = n[feature];
                  if(current > most)
                  {
                    most = current;
                    neighbors[voxelIndex] = neighborPoint;
                  }
                }
              }
            }
            for(int32 l = 0; l < 6; l++)
            {
              good = 1;
              neighborPoint = voxelIndex + neighborPointIdx[l];
              if(l == 0 && k == 0)
              {
                good = 0;
              }
              if(l == 5 && k == (dims[2] - 1))
              {
                good = 0;
              }
              if(l == 1 && j == 0)
              {
                good = 0;
              }
              if(l == 4 && j == (dims[1] - 1))
              {
                good = 0;
              }
              if(l == 2 && i == 0)
              {
                good = 0;
              }
              if(l == 3 && i == (dims[0] - 1))
              {
                good = 0;
              }
              if(good == 1)
              {
                feature = featureIds[neighborPoint];
                if(feature >= 0)
                {
                  n[feature] = 0;
                }
              }
            }
          }
          else if(featureName >= numFeatures)
          {
            std::string message = fmt::format("Error: Found a feature Id '{}' that is >= the number of features '{}' at voxel index X={},Y={},Z={}.", featureName, numFeatures, i, j, k);
            messageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, message});
            return MakeErrorResult(-55567, message);
          }
        }
      }
    }

    // TODO This can be parallelized much like NeighborOrientationCorrelation
    // Only iterate over the cell data with a featureId = -1;
    for(const auto& cellArrayPath : cellDataArrayPaths)
    {
      if(shouldCancel)
      {
        return {};
      }
      auto* voxelArray = dataStructure.getDataAs<IDataArray>(cellArrayPath);
      size_t arraySize = voxelArray->size();
      for(const auto& featureIdIndex : badFeatureIdIndexes)
      {
        featureName = featureIds[featureIdIndex];
        neighbor = neighbors[featureIdIndex];
        if((neighbor >= arraySize || featureIdIndex >= arraySize) && (featureName < 0 && neighbor >= 0 && featureIds[neighbor] >= 0))
        {
          std::string message =
              fmt::format("Out of range: While trying to copy a tuple from index {} to index {}\n  Array Name: {}\n  Num. Tuples: {}", neighbor, featureIdIndex, cellArrayPath.toString(), arraySize);
          messageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, message});
          return MakeErrorResult(-55568, message);
        }

        if(featureName < 0 && neighbor >= 0 && featureIds[neighbor] >= 0)
        {
          voxelArray->copyTuple(neighbor, featureIdIndex);
        }
      }
    }
  }
  return {};
}

nonstd::expected<std::vector<bool>, Error> mergeContainedFeatures(DataStructure& dataStructure, const Arguments& args, const std::atomic_bool& shouldCancel)
{
  auto imageGeomPath = args.value<DataPath>(MinNeighborsFilter::k_SelectedImageGeometryPath_Key);
  auto featureIdsPath = args.value<DataPath>(MinNeighborsFilter::k_FeatureIdsPath_Key);
  auto numNeighborsPath = args.value<DataPath>(MinNeighborsFilter::k_NumNeighborsPath_Key);
  auto minNumNeighbors = args.value<uint64>(MinNeighborsFilter::k_MinNumNeighbors_Key);

  auto phaseNumber = args.value<uint64>(MinNeighborsFilter::k_PhaseNumber_Key);

  auto& featureIdsArray = dataStructure.getDataRefAs<Int32Array>(featureIdsPath);
  auto& numNeighborsArray = dataStructure.getDataRefAs<Int32Array>(numNeighborsPath);

  auto& featureIds = featureIdsArray.getDataStoreRef();
  auto& numNeighbors = numNeighborsArray.getDataStoreRef();

  auto applyToSinglePhase = args.value<bool>(MinNeighborsFilter::k_ApplyToSinglePhase_Key);
  Int32Array* featurePhasesArray = nullptr;
  if(applyToSinglePhase)
  {
    auto featurePhasesPath = args.value<DataPath>(MinNeighborsFilter::k_FeaturePhasesPath_Key);
    featurePhasesArray = dataStructure.getDataAs<Int32Array>(featurePhasesPath);
  }

  bool good = false;
  usize totalPoints = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath).getNumberOfCells();
  usize totalFeatures = numNeighborsArray.getNumberOfTuples();

  std::vector<bool> activeObjects(totalFeatures, true);

  for(usize i = 1; i < totalFeatures; i++)
  {
    if(!applyToSinglePhase)
    {
      if(numNeighbors[i] >= minNumNeighbors)
      {
        good = true;
      }
      else
      {
        activeObjects[i] = false;
      }
    }
    else
    {
      if(numNeighbors[i] >= minNumNeighbors || (*featurePhasesArray)[i] != phaseNumber)
      {
        good = true;
      }
      else
      {
        activeObjects[i] = false;
      }
    }
  }
  if(!good)
  {
    return nonstd::make_unexpected<Error>({-1, "The minimum number of neighbors is larger than the Feature with the most neighbors.  All Features would be removed"});
  }
  if(shouldCancel)
  {
    return {};
  }
  for(usize i = 0; i < totalPoints; i++)
  {
    int32 featureId = featureIds[i];
    if(!activeObjects[featureId])
    {
      featureIds[i] = -1;
    }
  }
  return activeObjects;
}
} // namespace

//------------------------------------------------------------------------------
std::string MinNeighborsFilter::name() const
{
  return FilterTraits<MinNeighborsFilter>::name;
}

//------------------------------------------------------------------------------
std::string MinNeighborsFilter::className() const
{
  return FilterTraits<MinNeighborsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid MinNeighborsFilter::uuid() const
{
  return FilterTraits<MinNeighborsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string MinNeighborsFilter::humanName() const
{
  return "Minimum Number of Neighbors";
}

//------------------------------------------------------------------------------
std::vector<std::string> MinNeighborsFilter::defaultTags() const
{
  return {className(), "Minimum", "Neighbors", "Memory Management", "Cleanup", "Remove Features"};
}

//------------------------------------------------------------------------------
Parameters MinNeighborsFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<UInt64Parameter>(k_MinNumNeighbors_Key, "Minimum Number Neighbors", "Number of neighbors a Feature must have to remain as a Feature", 0));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ApplyToSinglePhase_Key, "Apply to Single Phase Only", "Whether to apply minimum to single ensemble or all ensembles", false));
  params.insert(std::make_unique<UInt64Parameter>(k_PhaseNumber_Key, "Phase Index", "Which Ensemble to apply minimum to. Only needed if Apply to Single Phase Only is checked", 0));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Image Geometry", "The target geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));

  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsPath_Key, "Cell Feature Ids", "Specifies to which Feature each Element belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_NumNeighborsPath_Key, "Number of Neighbors", "Number of contiguous neighboring Features for each Feature",
                                                          DataPath({"Data Container", "Feature Data", "NumNeighbors"}), ArraySelectionParameter::AllowedTypes{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(
      k_FeaturePhasesPath_Key, "Feature Phases", "Specifies to which Ensemble each Feature belongs. Only required if Apply to Single Phase Only is checked",
      DataPath({"Data Container", "Feature Data", "Phases"}), ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  // Attribute Arrays to Ignore
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_IgnoredVoxelArrays_Key, "Cell Arrays to Ignore", "The arrays to ignore when applying the minimum neighbors algorithm",
                                                               std::vector<DataPath>{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray},
                                                               MultiArraySelectionParameter::AllowedDataTypes{}));

  params.linkParameters(k_ApplyToSinglePhase_Key, k_PhaseNumber_Key, std::make_any<bool>(true));
  params.linkParameters(k_ApplyToSinglePhase_Key, k_FeaturePhasesPath_Key, std::make_any<bool>(true));
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer MinNeighborsFilter::clone() const
{
  return std::make_unique<MinNeighborsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult MinNeighborsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = args.value<DataPath>(k_SelectedImageGeometryPath_Key);
  auto applyToSinglePhase = args.value<bool>(k_ApplyToSinglePhase_Key);
  auto phaseNumber = args.value<uint64>(k_PhaseNumber_Key);
  auto featureIdsPath = args.value<DataPath>(k_FeatureIdsPath_Key);
  auto featurePhasesPath = args.value<DataPath>(k_FeaturePhasesPath_Key);
  auto numNeighborsPath = args.value<DataPath>(k_NumNeighborsPath_Key);
  auto minNumNeighbors = args.value<uint64>(k_MinNumNeighbors_Key);

  nx::core::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  std::vector<DataPath> dataArrayPaths;

  std::vector<usize> cDims = {1};
  auto& featureIdsArray = dataStructure.getDataRefAs<Int32Array>(featureIdsPath);

  auto& numNeighborsArray = dataStructure.getDataRefAs<Int32Array>(numNeighborsPath);
  dataArrayPaths.push_back(numNeighborsPath);

  if(applyToSinglePhase)
  {
    auto* featurePhasesArray = dataStructure.getDataAs<Int32Array>(featurePhasesPath);
    if(featurePhasesArray == nullptr)
    {
      std::string ss = fmt::format("Could not find Feature Phases array at path '{}'", featurePhasesPath.toString());
      return {MakeErrorResult<OutputActions>(k_MissingFeaturePhasesError, ss)};
    }

    dataArrayPaths.push_back(featurePhasesPath);
  }

  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(dataArrayPaths);
  if(!tupleValidityCheck)
  {
    return {MakeErrorResult<OutputActions>(k_InconsistentTupleCount,
                                           fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  // Inform users that the following arrays are going to be modified in place
  // Cell Data is going to be modified
  nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modifiedActions, featureIdsPath.getParent(), {});
  // Feature Data is going to be modified
  nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modifiedActions, numNeighborsPath.getParent(), {});

  // This section will warn the user about the removal of NeighborLists
  auto result = nx::core::NeighborListRemovalPreflightCode(dataStructure, featureIdsPath, numNeighborsPath, resultOutputActions);
  if(result.outputActions.invalid())
  {
    return result;
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> MinNeighborsFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                         const std::atomic_bool& shouldCancel) const
{
  auto featurePhasesPath = args.value<DataPath>(k_FeaturePhasesPath_Key);
  auto applyToSinglePhase = args.value<bool>(k_ApplyToSinglePhase_Key);
  auto phaseNumber = args.value<uint64>(k_PhaseNumber_Key);

  // If running on a single phase, validate that the user has not entered a phase number
  // that is not in the system ; the filter would not crash otherwise, but the user should
  // be notified of unanticipated behavior ; this cannot be done in the dataCheck since
  // we don't have access to the data yet
  if(applyToSinglePhase)
  {
    auto& featurePhasesArray = dataStructure.getDataRefAs<Int32Array>(featurePhasesPath);
    auto& featurePhases = featurePhasesArray.getDataStoreRef();

    usize numFeatures = featurePhasesArray.getNumberOfTuples();
    bool unavailablePhase = true;
    for(usize i = 0; i < numFeatures; i++)
    {
      if(featurePhases[i] == phaseNumber)
      {
        unavailablePhase = false;
        break;
      }
    }

    if(unavailablePhase)
    {
      std::string ss = fmt::format("The phase number ({}) is not available in the supplied Feature phases array with path ({})", phaseNumber, featurePhasesPath.toString());
      return MakeErrorResult(-5555, ss);
    }
  }

  auto activeObjectsResult = mergeContainedFeatures(dataStructure, args, shouldCancel);
  if(!activeObjectsResult.has_value())
  {
    return {nonstd::make_unexpected(std::vector<Error>{activeObjectsResult.error()})};
  }

  auto featureIdsPath = args.value<DataPath>(MinNeighborsFilter::k_FeatureIdsPath_Key);

  // The Cell Attribute Matrix is the parent of the "Feature Ids" array. Always.
  auto cellDataAttrMatrixPath = featureIdsPath.getParent();
  auto result = nx::core::GetAllChildDataPaths(dataStructure, cellDataAttrMatrixPath, DataObject::Type::DataArray);
  if(!result.has_value())
  {
    return MakeErrorResult(-5556, fmt::format("Error fetching all Data Arrays from Attribute Matrix '{}'", cellDataAttrMatrixPath.toString()));
  }

  // Run the algorithm.
  auto assignBadPointsResult = assignBadPoints(dataStructure, args, messageHandler, shouldCancel);
  if(assignBadPointsResult.invalid())
  {
    return assignBadPointsResult;
  }

  auto& featureIdsArray = dataStructure.getDataRefAs<Int32Array>(featureIdsPath);

  auto numNeighborsPath = args.value<DataPath>(MinNeighborsFilter::k_NumNeighborsPath_Key);
  auto& numNeighborsArray = dataStructure.getDataRefAs<Int32Array>(numNeighborsPath);

  DataPath cellFeatureGroupPath = numNeighborsPath.getParent();
  size_t currentFeatureCount = numNeighborsArray.getNumberOfTuples();

  auto activeObjects = activeObjectsResult.value();
  int32 count = 0;
  for(const auto& value : activeObjects)
  {
    value ? count++ : count = count;
  }
  std::string message = fmt::format("Feature Count Changed: Previous: {} New: {}", currentFeatureCount, count);
  messageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, message});

  nx::core::RemoveInactiveObjects(dataStructure, cellFeatureGroupPath, activeObjects, featureIdsArray, currentFeatureCount, messageHandler, shouldCancel);

  return {};
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_MinNumNeighborsKey = "MinNumNeighbors";
constexpr StringLiteral k_ApplyToSinglePhaseKey = "ApplyToSinglePhase";
constexpr StringLiteral k_PhaseNumberKey = "PhaseNumber";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_FeaturePhasesArrayPathKey = "FeaturePhasesArrayPath";
constexpr StringLiteral k_NumNeighborsArrayPathKey = "NumNeighborsArrayPath";
constexpr StringLiteral k_IgnoredDataArrayPathsKey = "IgnoredDataArrayPaths";
} // namespace SIMPL
} // namespace

Result<Arguments> MinNeighborsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = MinNeighborsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<uint64>>(args, json, SIMPL::k_MinNumNeighborsKey, k_MinNumNeighbors_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_ApplyToSinglePhaseKey, k_ApplyToSinglePhase_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<uint64>>(args, json, SIMPL::k_PhaseNumberKey, k_PhaseNumber_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_SelectedImageGeometryPath_Key));

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_FeatureIdsPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeaturePhasesArrayPathKey, k_FeaturePhasesPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_NumNeighborsArrayPathKey, k_NumNeighborsPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::MultiDataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_IgnoredDataArrayPathsKey, k_IgnoredVoxelArrays_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
