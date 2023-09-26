#include "CalculateFeatureSizesFilter.hpp"

#include "complex/Common/Numbers.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"

#include <cmath>

namespace complex
{
namespace
{
constexpr complex::int32 k_MissingGeometry = -73225;
constexpr complex::int32 k_MissingFeatureIds = -74789;
constexpr complex::int32 k_MissingFeatureAttributeMatrix = -74769;
constexpr complex::int32 k_BadFeatureCount = -78231;
constexpr complex::float32 k_PI = numbers::pi_v<complex::float32>;
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

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<BoolParameter>(k_SaveElementSizes_Key, "Generate Missing Element Sizes",
                                                "If checked this will generate and store the element sizes ONLY if the geometry does not already contain them.", false));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<DataPathSelectionParameter>(k_GeometryPath_Key, "Target Geometry", "DataPath to target geometry", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which Feature each Element belongs", DataPath({"FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellFeatureAttributeMatrixPath_Key, "Feature Attribute Matrix", "Feature Attribute Matrix of the selected Feature Ids",
                                                                    DataPath({"CellFeatureData"})));

  params.insertSeparator(Parameters::Separator{"Created Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_EquivalentDiametersPath_Key, "Equivalent Diameters", "DataPath to equivalent diameters array", "EquivalentDiameters"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_NumElementsPath_Key, "Number of Elements", "DataPath to Num Elements array", "NumElements"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_VolumesPath_Key, "Volumes", "DataPath to volumes array", "Volumes"));

  return params;
}

IFilter::UniquePointer CalculateFeatureSizesFilter::clone() const
{
  return std::make_unique<CalculateFeatureSizesFilter>();
}

IFilter::PreflightResult CalculateFeatureSizesFilter::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto geometryPath = args.value<DataPath>(k_GeometryPath_Key);

  auto featureIdsPath = args.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto featureAttributeMatrixPath = args.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);
  auto volumesName = args.value<std::string>(k_VolumesPath_Key);
  auto equivalentDiametersName = args.value<std::string>(k_EquivalentDiametersPath_Key);
  auto numElementsName = args.value<std::string>(k_NumElementsPath_Key);
  DataPath volumesPath = featureAttributeMatrixPath.createChildPath(volumesName);
  DataPath equivalentDiametersPath = featureAttributeMatrixPath.createChildPath(equivalentDiametersName);
  DataPath numElementsPath = featureAttributeMatrixPath.createChildPath(numElementsName);

  const auto* featureIdsArray = data.getDataAs<Int32Array>(featureIdsPath);

  const auto* geometry = data.getDataAs<IGeometry>(geometryPath);

  if(geometry == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingGeometry, "Could not find the target geometry."}})};
  }

  if(featureIdsArray == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingFeatureIds, "Could not find Feature IDs array."}})};
  }

  const std::string arrayDataFormat = featureIdsArray->getDataFormat();

  const auto* featAttributeMatrix = data.getDataAs<AttributeMatrix>(featureAttributeMatrixPath);
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

  OutputActions actions;
  actions.appendAction(std::move(createVolumesAction));
  actions.appendAction(std::move(createEquivalentDiametersAction));
  actions.appendAction(std::move(createNumElementsAction));

  return {std::move(actions)};
}

Result<> CalculateFeatureSizesFilter::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  auto saveElementSizes = args.value<bool>(k_SaveElementSizes_Key);
  auto featureIdsPath = args.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  const auto& featureIdsArray = data.getDataRefAs<Int32Array>(featureIdsPath);
  const auto& featureIds = featureIdsArray.getDataStoreRef();

  usize totalPoints = featureIdsArray.getNumberOfTuples();

  auto geomPath = args.value<DataPath>(k_GeometryPath_Key);
  auto* geom = data.getDataAs<IGeometry>(geomPath);

  // If the geometry is an ImageGeometry or a RectilinearGeometry
  auto* imageGeom = dynamic_cast<ImageGeom*>(geom);
  if(nullptr != imageGeom)
  {
    auto featureAttributeMatrixPath = args.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);
    auto volumesName = args.value<std::string>(k_VolumesPath_Key);
    auto equivalentDiametersName = args.value<std::string>(k_EquivalentDiametersPath_Key);
    auto numElementsName = args.value<std::string>(k_NumElementsPath_Key);

    DataPath volumesPath = featureAttributeMatrixPath.createChildPath(volumesName);
    DataPath equivalentDiametersPath = featureAttributeMatrixPath.createChildPath(equivalentDiametersName);
    DataPath numElementsPath = featureAttributeMatrixPath.createChildPath(numElementsName);
    auto& volumes = data.getDataRefAs<Float32Array>(volumesPath);
    auto& equivalentDiameters = data.getDataRefAs<Float32Array>(equivalentDiametersPath);
    auto& numElements = data.getDataRefAs<Int32Array>(numElementsPath);

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
  else
  {
    auto volumesPath = args.value<DataPath>(k_VolumesPath_Key);
    auto equivalentDiametersPath = args.value<DataPath>(k_EquivalentDiametersPath_Key);
    auto numElementsPath = args.value<DataPath>(k_NumElementsPath_Key);

    auto& volumes = data.getDataRefAs<Float32Array>(volumesPath);
    auto& equivalentDiameters = data.getDataRefAs<Float32Array>(equivalentDiametersPath);
    auto& numElements = data.getDataRefAs<Int32Array>(numElementsPath);

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

  return {};
}
} // namespace complex
