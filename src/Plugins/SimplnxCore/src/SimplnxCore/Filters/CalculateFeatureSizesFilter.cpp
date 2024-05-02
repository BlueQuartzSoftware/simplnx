#include "CalculateFeatureSizesFilter.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "SimplnxCore/Filters/Algorithms/CalculateFeatureBoundingBoxes.hpp"

#include <cmath>

namespace nx::core
{
namespace
{
constexpr nx::core::int32 k_MissingGeometry = -73225;
constexpr nx::core::int32 k_MissingFeatureIds = -74789;
constexpr nx::core::int32 k_MissingFeatureAttributeMatrix = -74769;
constexpr nx::core::int32 k_BadFeatureCount = -78231;
constexpr nx::core::float32 k_PI = numbers::pi_v<nx::core::float32>;

} // namespace

std::string CalculateFeatureSizesFilter::name() const
{
  return FilterTraits<CalculateFeatureSizesFilter>::name;
}

std::string CalculateFeatureSizesFilter::className() const
{
  return FilterTraits<CalculateFeatureSizesFilter>::className;
}

Uuid CalculateFeatureSizesFilter::uuid() const
{
  return FilterTraits<CalculateFeatureSizesFilter>::uuid;
}

std::string CalculateFeatureSizesFilter::humanName() const
{
  return "Find Feature Sizes";
}

//------------------------------------------------------------------------------
std::vector<std::string> CalculateFeatureSizesFilter::defaultTags() const
{
  return {className(), "Statistics", "Morphological", "Feature Calculation", "Find Feature Sizes"};
}

Parameters CalculateFeatureSizesFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<BoolParameter>(k_SaveElementSizes_Key, "Generate Missing Element Sizes",
                                                "If checked this will generate and store the element sizes ONLY if the geometry does not already contain them.", false));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_GeometryPath_Key, "Input Image Geometry", "DataPath to input Image Geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));

  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which Feature each Element belongs", DataPath({"FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellFeatureAttributeMatrixPath_Key, "Feature Attribute Matrix", "Feature Attribute Matrix of the selected Feature Ids",
                                                                    DataPath({"CellFeatureData"})));

  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_EquivalentDiametersName_Key, "Equivalent Diameters", "DataPath to equivalent diameters array", "EquivalentDiameters"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_NumElementsName_Key, "Number of Elements", "DataPath to Num Elements array", "NumElements"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_VolumesName_Key, "Volumes", "DataPath to volumes array", "Volumes"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeretDiameterName_Key, "Feret Diameter (Caliper Diameter)", "Output Feret diameter array name", "Feret Diameter"));

  return params;
}

IFilter::UniquePointer CalculateFeatureSizesFilter::clone() const
{
  return std::make_unique<CalculateFeatureSizesFilter>();
}

IFilter::PreflightResult CalculateFeatureSizesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel) const
{
  auto geometryPath = args.value<DataPath>(k_GeometryPath_Key);

  auto featureIdsPath = args.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto featureAttributeMatrixPath = args.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);
  auto volumesName = args.value<std::string>(k_VolumesName_Key);
  auto equivalentDiametersName = args.value<std::string>(k_EquivalentDiametersName_Key);
  auto numElementsName = args.value<std::string>(k_NumElementsName_Key);
  auto feretName = args.value<std::string>(k_FeretDiameterName_Key);

  DataPath volumesPath = featureAttributeMatrixPath.createChildPath(volumesName);
  DataPath equivalentDiametersPath = featureAttributeMatrixPath.createChildPath(equivalentDiametersName);
  DataPath numElementsPath = featureAttributeMatrixPath.createChildPath(numElementsName);
  DataPath feretDiameterPath = featureAttributeMatrixPath.createChildPath(feretName);

  const auto* featureIdsArray = dataStructure.getDataAs<Int32Array>(featureIdsPath);

  const auto* geometry = dataStructure.getDataAs<IGeometry>(geometryPath);

  if(geometry == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingGeometry, "Could not find the target geometry."}})};
  }

  if(featureIdsArray == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingFeatureIds, "Could not find Feature IDs array."}})};
  }

  const std::string arrayDataFormat = featureIdsArray->getDataFormat();

  const auto* featAttributeMatrix = dataStructure.getDataAs<AttributeMatrix>(featureAttributeMatrixPath);
  if(featAttributeMatrix == nullptr)
  {
    return {nonstd::make_unexpected(
        std::vector<Error>{Error{k_MissingFeatureAttributeMatrix, fmt::format("Could not find Feature Attribute Matrix at path '{}'", featureAttributeMatrixPath.toString())}})};
  }

  std::vector<usize> tupleDimensions = featAttributeMatrix->getShape();
  uint64 numberOfComponents = 1;

  auto createVolumesAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleDimensions, std::vector<usize>{numberOfComponents}, volumesPath, arrayDataFormat);
  auto createEquivalentDiametersAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleDimensions, std::vector<usize>{numberOfComponents}, equivalentDiametersPath, arrayDataFormat);
  auto createNumElementsAction = std::make_unique<CreateArrayAction>(DataType::int32, tupleDimensions, std::vector<usize>{numberOfComponents}, numElementsPath, arrayDataFormat);
  auto createFeretDiametersAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleDimensions, std::vector<usize>{7}, feretDiameterPath, arrayDataFormat);

  OutputActions actions;
  actions.appendAction(std::move(createVolumesAction));
  actions.appendAction(std::move(createEquivalentDiametersAction));
  actions.appendAction(std::move(createNumElementsAction));
  actions.appendAction(std::move(createFeretDiametersAction));

  return {std::move(actions)};
}

Result<> CalculateFeatureSizesFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  auto saveElementSizes = args.value<bool>(k_SaveElementSizes_Key);
  auto featureIdsPath = args.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  const auto& featureIdsArray = dataStructure.getDataRefAs<Int32Array>(featureIdsPath);
  const auto& featureIds = featureIdsArray.getDataStoreRef();

  auto featureAttributeMatrixPath = args.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);
  auto volumesName = args.value<std::string>(k_VolumesName_Key);
  auto equivalentDiametersName = args.value<std::string>(k_EquivalentDiametersName_Key);
  auto numElementsName = args.value<std::string>(k_NumElementsName_Key);
  auto feretDiamName = args.value<std::string>(k_FeretDiameterName_Key);

  DataPath volumesPath = featureAttributeMatrixPath.createChildPath(volumesName);
  DataPath equivalentDiametersPath = featureAttributeMatrixPath.createChildPath(equivalentDiametersName);
  DataPath numElementsPath = featureAttributeMatrixPath.createChildPath(numElementsName);
  DataPath feretDiamPath = featureAttributeMatrixPath.createChildPath(feretDiamName);

  auto& volumes = dataStructure.getDataRefAs<Float32Array>(volumesPath);
  auto& equivalentDiameters = dataStructure.getDataRefAs<Float32Array>(equivalentDiametersPath);
  auto& numElements = dataStructure.getDataRefAs<Int32Array>(numElementsPath);
  auto& feretDiameters = dataStructure.getDataRefAs<Float32Array>(feretDiamPath);

  usize totalPoints = featureIdsArray.getNumberOfTuples();

  auto geomPath = args.value<DataPath>(k_GeometryPath_Key);
  auto* geom = dataStructure.getDataAs<IGridGeometry>(geomPath);

  // If the geometry is an ImageGeometry or a RectilinearGeometry
  auto* imageGeom = dynamic_cast<ImageGeom*>(geom);
  if(nullptr != imageGeom) // Image Geometry
  {

    usize featureIdsMaxIdx = std::distance(featureIds.begin(), std::max_element(featureIds.cbegin(), featureIds.cend()));
    usize maxValue = featureIds[featureIdsMaxIdx];
    usize numFeatures = maxValue + 1;

    std::vector<uint64> featureCounts(numFeatures, 0);

    for(size_t j = 0; j < totalPoints; j++)
    {
      int32_t gnum = featureIds[j];
      auto temp = featureCounts[gnum] + 1;
      featureCounts[gnum] = temp;
    }

    FloatVec3 spacing = imageGeom->getSpacing();
    // 2D Case
    if(imageGeom->getNumXCells() == 1 || imageGeom->getNumYCells() == 1 || imageGeom->getNumZCells() == 1)
    {
      float res_scalar = 0.0f;
      if(imageGeom->getNumXCells() == 1)
      {
        res_scalar = spacing[1] * spacing[2];
      }
      else if(imageGeom->getNumYCells() == 1)
      {
        res_scalar = spacing[0] * spacing[2];
      }
      else if(imageGeom->getNumZCells() == 1)
      {
        res_scalar = spacing[0] * spacing[1];
      }

      for(size_t i = 1; i < numFeatures; i++)
      {
        numElements[i] = static_cast<int32_t>(featureCounts[i]);
        if(featureCounts[i] > 9007199254740992ULL)
        {
          std::string ss = fmt::format("Number of voxels belonging to feature {} ({}) is greater than 9007199254740992", i, featureCounts[i]);
          return {nonstd::make_unexpected(std::vector<Error>{Error{k_BadFeatureCount, ss}})};
        }
        volumes[i] = static_cast<double>(featureCounts[i]) * static_cast<double>(res_scalar);

        float32 rad = volumes[i] / k_PI;
        float32 diameter = (2 * sqrtf(rad));
        equivalentDiameters[i] = diameter;
      }
    }
    else
    {
      float32 res_scalar = spacing[0] * spacing[1] * spacing[2];
      float vol_term = (4.0f / 3.0f) * k_PI;
      for(usize i = 1; i < numFeatures; i++)
      {
        numElements[i] = static_cast<int32>(featureCounts[i]);
        if(featureCounts[i] > 9007199254740992ULL)
        {
          std::string ss = fmt::format("Number of voxels belonging to feature {} ({}) is greater than 9007199254740992", i, featureCounts[i]);
          return {nonstd::make_unexpected(std::vector<Error>{Error{k_BadFeatureCount, ss}})};
        }

        volumes[i] = static_cast<double>(featureCounts[i]) * static_cast<double>(res_scalar);

        float32 rad = volumes[i] / vol_term;
        float32 diameter = 2.0f * powf(rad, 0.3333333333f);
        equivalentDiameters[i] = diameter;
      }
    }

    if(saveElementSizes)
    {
      if(!imageGeom->getElementSizes())
      {
        int32 err = imageGeom->findElementSizes();
        if(err < 0)
        {
          std::string ss = fmt::format("Error computing Element sizes for Geometry type {}", imageGeom->getTypeName());
          return {nonstd::make_unexpected(std::vector<Error>{Error{err, ss}})};
        }
      }
    }
  }
  else // Rectilinear Grid
  {
    usize numfeatures = volumes.getNumberOfTuples();

    if(!geom->getElementSizes())
    {
      int32_t err = geom->findElementSizes();
      if(err < 0)
      {
        std::string ss = fmt::format("Error computing Element sizes for Geometry type {}", geom->getTypeName());
        return {nonstd::make_unexpected(std::vector<Error>{Error{err, ss}})};
      }
    }

    const Float32Array* elemSizes = geom->getElementSizes();

    std::vector<float> featureCounts(numfeatures, 1);

    for(size_t j = 0; j < totalPoints; j++)
    {
      int32 gnum = featureIds[j];
      auto temp = featureCounts[gnum] + 1;
      featureCounts[gnum] = temp;
      auto temp2 = volumes[gnum];
      volumes[gnum] = temp2 + (*elemSizes)[j];
    }
    float vol_term = (4.0f / 3.0f) * k_PI;
    for(size_t i = 1; i < numfeatures; i++)
    {
      numElements[i] = static_cast<int32>(featureCounts[i]);
      float rad = volumes[i] / vol_term;
      float diameter = 2.0f * powf(rad, 0.3333333333f);
      equivalentDiameters[i] = diameter;
    }

    if(!saveElementSizes)
    {
      geom->deleteElementSizes();
    }
  }

  // Calculate the Feret Diameters (Caliper diameters)
  {
    auto featureAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(featureAttributeMatrixPath);
    auto numFeatures = featureAttrMat.getNumTuples();

    std::vector<uint32> corners(numFeatures * 6);
    // Create corners array, which stores pixel coordinates for the top-left and bottom-right coordinates of each feature object
    for(usize i = 0; i < numFeatures; i++)
    {
      corners[i * 6 + 0] = std::numeric_limits<uint32>::max();
      corners[i * 6 + 1] = std::numeric_limits<uint32>::max();
      corners[i * 6 + 2] = std::numeric_limits<uint32>::max();
      corners[i * 6 + 3] = std::numeric_limits<uint32>::min();
      corners[i * 6 + 4] = std::numeric_limits<uint32>::min();
      corners[i * 6 + 5] = std::numeric_limits<uint32>::min();
    }
    auto imageDims = geom->getDimensions();

    const usize xDim = imageDims[0];
    const usize yDim = imageDims[1];
    const usize zDim = imageDims[2];

    usize index = 0;
    // Store the coordinates in the corners array
    for(uint32 z = 0; z < zDim; z++)
    {
      if(shouldCancel)
      {
        return {};
      }

      for(uint32 y = 0; y < yDim; y++)
      {
        for(uint32 x = 0; x < xDim; x++)
        {
          index = (imageDims[1] * imageDims[0] * z) + (imageDims[0] * y) + x; // Index into featureIds array

          const int32 featureId = featureIds[index];
          if(featureId == 0)
          {
            continue;
          }

          if(featureId >= numFeatures)
          {
            return MakeErrorResult(
                -31000, fmt::format("The largest featureId is '{}' but the Feature Attribute Matrix '{}' only allows '{}' features.", featureId, featureAttributeMatrixPath.toString(), numFeatures));
          }

          const uint32 indices[3] = {x, y, z}; // Sequence dependent DO NOT REORDER
          const int32 featureShift = featureId * 6;
          for(uint8 l = 0; l < 6; l++) // unsigned is faster with modulo
          {
            if(l > 2)
            {
              corners[featureShift + l] = std::max(corners[featureShift + l], indices[l - 3]);
            }
            else
            {
              corners[featureShift + l] = std::min(corners[featureShift + l], indices[l]);
            }
          }
        }
      }
    }

    for(size_t featureIdx = 1; featureIdx < numFeatures; featureIdx++)
    {
      auto cellBounds = geom->getCellBounds(static_cast<usize>(corners[featureIdx * 6 + 0]), static_cast<usize>(corners[featureIdx * 6 + 1]), static_cast<usize>(corners[featureIdx * 6 + 2]));
      Point3Df minCoord = cellBounds.first;
      cellBounds = geom->getCellBounds(static_cast<usize>(corners[featureIdx * 6 + 3]), static_cast<usize>(corners[featureIdx * 6 + 4]), static_cast<usize>(corners[featureIdx * 6 + 5]));
      Point3Df maxCoord = cellBounds.second;

      float xDist = maxCoord[0] - minCoord[0];
      float yDist = maxCoord[1] - minCoord[1];
      float zDist = maxCoord[2] - minCoord[2];

      float xyDist = (Point3Df(maxCoord[0], maxCoord[1], minCoord[2]) - Point3Df(minCoord[0], minCoord[1], minCoord[2])).magnitude();
      float xzDist = (Point3Df(maxCoord[0], minCoord[1], maxCoord[2]) - Point3Df(minCoord[0], minCoord[1], minCoord[2])).magnitude();
      float yzDist = (Point3Df(minCoord[0], maxCoord[1], maxCoord[2]) - Point3Df(minCoord[0], minCoord[1], minCoord[2])).magnitude();

      float spaceDiag = (Point3Df(maxCoord[0], maxCoord[1], maxCoord[2]) - Point3Df(minCoord[0], minCoord[1], minCoord[2])).magnitude();

      feretDiameters.setComponent(featureIdx, 0, xDist);
      feretDiameters.setComponent(featureIdx, 1, yDist);
      feretDiameters.setComponent(featureIdx, 2, zDist);

      feretDiameters.setComponent(featureIdx, 3, xyDist);
      feretDiameters.setComponent(featureIdx, 4, xzDist);
      feretDiameters.setComponent(featureIdx, 5, yzDist);

      feretDiameters.setComponent(featureIdx, 6, spaceDiag);
    }
  }

  return {};
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_SaveElementSizesKey = "SaveElementSizes";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_FeatureAttributeMatrixNameKey = "FeatureAttributeMatrixName";
constexpr StringLiteral k_EquivalentDiametersArrayNameKey = "EquivalentDiametersArrayName";
constexpr StringLiteral k_NumElementsArrayNameKey = "NumElementsArrayName";
constexpr StringLiteral k_VolumesArrayNameKey = "VolumesArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> CalculateFeatureSizesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = CalculateFeatureSizesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_SaveElementSizesKey, k_SaveElementSizes_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_GeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureAttributeMatrixNameKey, k_CellFeatureAttributeMatrixPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_EquivalentDiametersArrayNameKey, k_EquivalentDiametersName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_NumElementsArrayNameKey, k_NumElementsName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_VolumesArrayNameKey, k_VolumesName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
