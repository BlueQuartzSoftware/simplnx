#include "MinNeighbors.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

namespace nx::core
{
namespace
{
constexpr int64 k_TupleCountInvalidError = -250;
constexpr int64 k_MissingFeaturePhasesError = -251;
constexpr int32 k_InconsistentTupleCount = -252;

void assignBadPoints(DataStructure& data, const Arguments& args, const std::atomic_bool& shouldCancel)
{
  auto imageGeomPath = args.value<DataPath>(MinNeighbors::k_ImageGeom_Key);
  auto featureIdsPath = args.value<DataPath>(MinNeighbors::k_FeatureIdsPath_Key);
  auto numNeighborsPath = args.value<DataPath>(MinNeighbors::k_NumNeighborsPath_Key);
  auto ignoredVoxelArrayPaths = args.value<std::vector<DataPath>>(MinNeighbors::k_IgnoredVoxelArrays_Key);
  auto cellDataAttrMatrix = args.value<DataPath>(MinNeighbors::k_CellDataAttributeMatrixPath_Key);

  auto& featureIdsArray = data.getDataRefAs<Int32Array>(featureIdsPath);
  auto& numNeighborsArray = data.getDataRefAs<Int32Array>(numNeighborsPath);

  auto& featureIds = featureIdsArray.getDataStoreRef();

  auto applyToSinglePhase = args.value<bool>(MinNeighbors::k_ApplyToSinglePhase_Key);
  Int32Array* featurePhasesArray = nullptr;
  if(applyToSinglePhase)
  {
    auto featurePhasesPath = args.value<DataPath>(MinNeighbors::k_FeaturePhasesPath_Key);
    featurePhasesArray = data.getDataAs<Int32Array>(featurePhasesPath);
  }

  usize totalPoints = featureIdsArray.getNumberOfTuples();
  SizeVec3 udims = data.getDataRefAs<ImageGeom>(imageGeomPath).getDimensions();

  // This was checked up in the execute function (which is called before this function)
  // so if we got this far then all should be good with the return. We might get
  // an empty vector<> but that is OK.
  std::vector<DataPath> cellDataArrayPaths = nx::core::GetAllChildDataPaths(data, cellDataAttrMatrix, DataObject::Type::DataArray).value();

  std::array<int64, 3> dims = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  std::vector<int32_t> neighbors(totalPoints, -1);

  int32 good = 1;
  int32 current = 0;
  int32 most = 0;
  int64 neighpoint = 0;
  usize numfeatures = numNeighborsArray.getNumberOfTuples();

  int64 neighpoints[6] = {0, 0, 0, 0, 0, 0};
  neighpoints[0] = -dims[0] * dims[1];
  neighpoints[1] = -dims[0];
  neighpoints[2] = -1;
  neighpoints[3] = 1;
  neighpoints[4] = dims[0];
  neighpoints[5] = dims[0] * dims[1];

  usize counter = 1;
  int64 count = 0;
  int64 kstride = 0, jstride = 0;
  int32 featurename = 0, feature = 0;
  int32 neighbor = 0;
  std::vector<int32> n(numfeatures + 1, 0);
  std::vector<size_t> badFeatureIdIndexes;
  while(counter != 0)
  {
    if(shouldCancel)
    {
      return;
    }
    counter = 0;
    badFeatureIdIndexes.clear();
    for(int64 k = 0; k < dims[2]; k++)
    {
      kstride = dims[0] * dims[1] * k;
      for(int64 j = 0; j < dims[1]; j++)
      {
        jstride = dims[0] * j;
        for(int64 i = 0; i < dims[0]; i++)
        {
          count = kstride + jstride + i;
          featurename = featureIds[count];
          if(featurename < 0)
          {
            badFeatureIdIndexes.push_back(count);
            counter++;
            current = 0;
            most = 0;
            for(int32 l = 0; l < 6; l++)
            {
              good = 1;
              neighpoint = count + neighpoints[l];
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
                feature = featureIds[neighpoint];
                if(feature >= 0)
                {
                  n[feature]++;
                  current = n[feature];
                  if(current > most)
                  {
                    most = current;
                    neighbors[count] = neighpoint;
                  }
                }
              }
            }
            for(int32 l = 0; l < 6; l++)
            {
              good = 1;
              neighpoint = count + neighpoints[l];
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
                feature = featureIds[neighpoint];
                if(feature >= 0)
                {
                  n[feature] = 0;
                }
              }
            }
          }
        }
      }
    }

    // TODO This can be parallelized much like NeighborOrientationCorrelation
    // Only iterate over the cell data with a featureId = -1;
    for(const auto& featureIdIndex : badFeatureIdIndexes)
    {
      featurename = featureIds[featureIdIndex];
      neighbor = neighbors[featureIdIndex];
      if(featurename < 0 && neighbor >= 0 && featureIds[neighbor] >= 0)
      {
        for(const auto& cellArrayPath : cellDataArrayPaths)
        {
          auto* voxelArray = data.getDataAs<IDataArray>(cellArrayPath);
          dynamic_cast<IDataArray*>(voxelArray)->copyTuple(neighbor, featureIdIndex);
        }
      }
    }
  }
}

nonstd::expected<std::vector<bool>, Error> mergeContainedFeatures(DataStructure& data, const Arguments& args, const std::atomic_bool& shouldCancel)
{
  auto imageGeomPath = args.value<DataPath>(MinNeighbors::k_ImageGeom_Key);
  auto featureIdsPath = args.value<DataPath>(MinNeighbors::k_FeatureIdsPath_Key);
  auto numNeighborsPath = args.value<DataPath>(MinNeighbors::k_NumNeighborsPath_Key);
  auto minNumNeighbors = args.value<uint64>(MinNeighbors::k_MinNumNeighbors_Key);

  auto phaseNumber = args.value<uint64>(MinNeighbors::k_PhaseNumber_Key);

  auto& featureIdsArray = data.getDataRefAs<Int32Array>(featureIdsPath);
  auto& numNeighborsArray = data.getDataRefAs<Int32Array>(numNeighborsPath);

  auto& featureIds = featureIdsArray.getDataStoreRef();
  auto& numNeighbors = numNeighborsArray.getDataStoreRef();

  auto applyToSinglePhase = args.value<bool>(MinNeighbors::k_ApplyToSinglePhase_Key);
  Int32Array* featurePhasesArray = nullptr;
  if(applyToSinglePhase)
  {
    auto featurePhasesPath = args.value<DataPath>(MinNeighbors::k_FeaturePhasesPath_Key);
    featurePhasesArray = data.getDataAs<Int32Array>(featurePhasesPath);
  }

  bool good = false;
  usize totalPoints = data.getDataRefAs<ImageGeom>(imageGeomPath).getNumberOfCells();
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
std::string MinNeighbors::name() const
{
  return FilterTraits<MinNeighbors>::name;
}

//------------------------------------------------------------------------------
std::string MinNeighbors::className() const
{
  return FilterTraits<MinNeighbors>::className;
}

//------------------------------------------------------------------------------
Uuid MinNeighbors::uuid() const
{
  return FilterTraits<MinNeighbors>::uuid;
}

//------------------------------------------------------------------------------
std::string MinNeighbors::humanName() const
{
  return "Minimum Number of Neighbors";
}

//------------------------------------------------------------------------------
std::vector<std::string> MinNeighbors::defaultTags() const
{
  return {className(), "Minimum", "Neighbors", "Memory Management", "Cleanup", "Remove Features"};
}

//------------------------------------------------------------------------------
Parameters MinNeighbors::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<UInt64Parameter>(k_MinNumNeighbors_Key, "Minimum Number Neighbors", "Number of neighbors a Feature must have to remain as a Feature", 0));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ApplyToSinglePhase_Key, "Apply to Single Phase Only", "Whether to apply minimum to single ensemble or all ensembles", false));
  params.insert(std::make_unique<UInt64Parameter>(k_PhaseNumber_Key, "Phase Index", "Which Ensemble to apply minimum to. Only needed if Apply to Single Phase Only is checked", 0));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeom_Key, "Image Geometry", "The target geometry", DataPath{}, GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellDataAttributeMatrixPath_Key, "Cell Attribute Matrix",
                                                                    "The cell data attribute matrix in which to apply the minimum neighbors algorithm", DataPath({"Data Container", "CellData"})));

  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsPath_Key, "Feature Ids", "Specifies to which Feature each Element belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Required Input Feature Data"});
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
IFilter::UniquePointer MinNeighbors::clone() const
{
  return std::make_unique<MinNeighbors>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult MinNeighbors::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = args.value<DataPath>(k_ImageGeom_Key);
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

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> MinNeighbors::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
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
    auto& featurePhasesArray = data.getDataRefAs<Int32Array>(featurePhasesPath);
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

  auto activeObjectsResult = mergeContainedFeatures(data, args, shouldCancel);
  if(!activeObjectsResult.has_value())
  {
    return {nonstd::make_unexpected(std::vector<Error>{activeObjectsResult.error()})};
  }

  auto cellDataAttrMatrix = args.value<DataPath>(MinNeighbors::k_CellDataAttributeMatrixPath_Key);
  auto result = nx::core::GetAllChildDataPaths(data, cellDataAttrMatrix, DataObject::Type::DataArray);
  if(!result.has_value())
  {
    return MakeErrorResult(-5556, fmt::format("Error fetching all Data Arrays from Group '{}'", cellDataAttrMatrix.toString()));
  }

  // Run the algorithm.
  assignBadPoints(data, args, shouldCancel);

  auto featureIdsPath = args.value<DataPath>(MinNeighbors::k_FeatureIdsPath_Key);
  auto& featureIdsArray = data.getDataRefAs<Int32Array>(featureIdsPath);

  auto numNeighborsPath = args.value<DataPath>(MinNeighbors::k_NumNeighborsPath_Key);
  auto& numNeighborsArray = data.getDataRefAs<Int32Array>(numNeighborsPath);

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

  nx::core::RemoveInactiveObjects(data, cellFeatureGroupPath, activeObjects, featureIdsArray, currentFeatureCount);

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

Result<Arguments> MinNeighbors::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = MinNeighbors().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<uint64>>(args, json, SIMPL::k_MinNumNeighborsKey, k_MinNumNeighbors_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_ApplyToSinglePhaseKey, k_ApplyToSinglePhase_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<uint64>>(args, json, SIMPL::k_PhaseNumberKey, k_PhaseNumber_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_ImageGeom_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_CellDataAttributeMatrixPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_FeatureIdsPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeaturePhasesArrayPathKey, k_FeaturePhasesPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_NumNeighborsArrayPathKey, k_NumNeighborsPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::MultiDataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_IgnoredDataArrayPathsKey, k_IgnoredVoxelArrays_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
