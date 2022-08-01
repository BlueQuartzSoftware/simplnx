#include "CalculateFeatureSizesFilter.hpp"

#include "complex/Common/Numbers.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"

#include <cmath>

namespace complex
{
namespace
{
constexpr complex::int32 k_MissingGeometry = -73225;
constexpr complex::int32 k_MissingFeatureIds = -74789;
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
  return {"#Statistics", "#Morphological", "#Feature Calculation", "#Find Feature Sizes"};
}

Parameters CalculateFeatureSizesFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<BoolParameter>(k_SaveElementSizes_Key, "Save Element Sizes", "Save element sizes", false));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<DataPathSelectionParameter>(k_GeometryPath_Key, "Target Geometry", "DataPath to target geometry", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "", DataPath({"FeatureIds"}), ArraySelectionParameter::AllowedTypes{DataType::int32}));

  params.insertSeparator(Parameters::Separator{"Created Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_EquivalentDiametersPath_Key, "Equivalent Diameters", "DataPath to equivalent diameters array", DataPath({"EquivalentDiameters"})));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NumElementsPath_Key, "Number of Elements", "DataPath to Num Elements array", DataPath({"NumElements"})));
  params.insert(std::make_unique<ArrayCreationParameter>(k_VolumesPath_Key, "Volumes", "DataPath to volumes array", DataPath({"Volumes"})));

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
  auto volumesPath = args.value<DataPath>(k_VolumesPath_Key);
  auto equivalentDiametersPath = args.value<DataPath>(k_EquivalentDiametersPath_Key);
  auto numElementsPath = args.value<DataPath>(k_NumElementsPath_Key);

  const auto* featureIdsArray = data.getDataAs<Int32Array>(featureIdsPath);

  const auto* geometry = data.getDataAs<AbstractGeometry>(geometryPath);

  if(geometry == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingGeometry, "Could not find the target geometry."}})};
  }

  if(featureIdsArray == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingFeatureIds, "Could not find Feature IDs array."}})};
  }

  std::vector<usize> tupleDimensions = {1ULL};
  uint64 numberOfComponents = 1;

  auto createVolumesAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleDimensions, std::vector<usize>{numberOfComponents}, volumesPath);
  auto createEquivalentDiametersAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleDimensions, std::vector<usize>{numberOfComponents}, equivalentDiametersPath);
  auto createNumElementsAction = std::make_unique<CreateArrayAction>(DataType::int32, tupleDimensions, std::vector<usize>{numberOfComponents}, numElementsPath);

  OutputActions actions;
  actions.actions.push_back(std::move(createVolumesAction));
  actions.actions.push_back(std::move(createEquivalentDiametersAction));
  actions.actions.push_back(std::move(createNumElementsAction));

  return {std::move(actions)};
}

Result<> CalculateFeatureSizesFilter::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  return std::move(findSizes(data, args));
}

Result<> CalculateFeatureSizesFilter::findSizesImage(DataStructure& data, const Arguments& args, ImageGeom* image) const
{
  auto saveElementSizes = args.value<bool>(k_SaveElementSizes_Key);

  auto featureIdsPath = args.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto volumesPath = args.value<DataPath>(k_VolumesPath_Key);
  auto equivalentDiametersPath = args.value<DataPath>(k_EquivalentDiametersPath_Key);
  auto numElementsPath = args.value<DataPath>(k_NumElementsPath_Key);

  auto& featureIds = data.getDataRefAs<Int32Array>(featureIdsPath);
  auto& volumes = data.getDataRefAs<Float32Array>(volumesPath);
  auto& equivalentDiameters = data.getDataRefAs<Float32Array>(equivalentDiametersPath);
  auto& numElements = data.getDataRefAs<Int32Array>(numElementsPath);

  usize totalPoints = featureIds.getNumberOfTuples();
  std::set<int32_t> uniqueFeatureIds;
  for(size_t i = 0; i < totalPoints; i++)
  {
    uniqueFeatureIds.insert((featureIds)[i]);
  }

  usize numfeatures = uniqueFeatureIds.size();

  volumes.getDataStoreRef().reshapeTuples({numfeatures});
  equivalentDiameters.getDataStoreRef().reshapeTuples({numfeatures});
  numElements.getDataStoreRef().reshapeTuples({numfeatures});

  std::vector<uint64> featureCountsStore(numfeatures, 0);

  float res_scalar = 0.0f;

  for(size_t j = 0; j < totalPoints; j++)
  {
    int32_t gnum = featureIds[j];
    auto temp = featureCountsStore[gnum] + 1;
    featureCountsStore[gnum] = temp;
  }

  FloatVec3 spacing = image->getSpacing();

  if(image->getNumXPoints() == 1 || image->getNumYPoints() == 1 || image->getNumZPoints() == 1)
  {
    if(image->getNumXPoints() == 1)
    {
      res_scalar = spacing[1] * spacing[2];
    }
    else if(image->getNumYPoints() == 1)
    {
      res_scalar = spacing[0] * spacing[2];
    }
    else if(image->getNumZPoints() == 1)
    {
      res_scalar = spacing[0] * spacing[1];
    }

    for(size_t i = 1; i < numfeatures; i++)
    {
      numElements[i] = static_cast<int32_t>(featureCountsStore[i]);
      if(featureCountsStore[i] > 9007199254740992ULL)
      {
        std::string ss = fmt::format("Number of voxels belonging to feature {} ({}) is greater than 9007199254740992", i, featureCountsStore[i]);
        return {nonstd::make_unexpected(std::vector<Error>{Error{k_BadFeatureCount, ss}})};
      }
      volumes[i] = static_cast<double>(featureCountsStore[i]) * static_cast<double>(res_scalar);

      float rad = volumes[i] / k_PI;
      float diameter = (2 * sqrtf(rad));
      equivalentDiameters[i] = diameter;
    }
  }
  else
  {
    res_scalar = spacing[0] * spacing[1] * spacing[2];
    float vol_term = (4.0f / 3.0f) * k_PI;
    for(usize i = 1; i < numfeatures; i++)
    {
      numElements[i] = static_cast<int32>(featureCountsStore[i]);
      if(featureCountsStore[i] > 9007199254740992ULL)
      {
        std::string ss = fmt::format("Number of voxels belonging to feature {} ({}) is greater than 9007199254740992", i, featureCountsStore[i]);
        return {nonstd::make_unexpected(std::vector<Error>{Error{k_BadFeatureCount, ss}})};
      }

      volumes[i] = static_cast<double>(featureCountsStore[i]) * static_cast<double>(res_scalar);

      float rad = volumes[i] / vol_term;
      float diameter = 2.0f * powf(rad, 0.3333333333f);
      equivalentDiameters[i] = diameter;
    }
  }

  if(saveElementSizes)
  {
    if(nullptr != image->getElementSizes())
    {
      int32 err = image->findElementSizes();
      if(err < 0)
      {
        std::string ss = fmt::format("Error computing Element sizes for Geometry type {}", image->getGeometryTypeAsString());
        return {nonstd::make_unexpected(std::vector<Error>{Error{err, ss}})};
      }
    }
  }

  return {};
}

Result<> CalculateFeatureSizesFilter::findSizesUnstructured(DataStructure& data, const Arguments& args, AbstractGeometry* igeom) const
{
  auto saveElementSizes = args.value<bool>(k_SaveElementSizes_Key);

  auto featureIdsPath = args.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto volumesPath = args.value<DataPath>(k_VolumesPath_Key);
  auto equivalentDiametersPath = args.value<DataPath>(k_EquivalentDiametersPath_Key);
  auto numElementsPath = args.value<DataPath>(k_NumElementsPath_Key);

  const auto& featureIds = data.getDataRefAs<Int32Array>(featureIdsPath);
  auto& volumes = data.getDataRefAs<Float32Array>(volumesPath);
  auto& equivalentDiameters = data.getDataRefAs<Float32Array>(equivalentDiametersPath);
  auto& numElements = data.getDataRefAs<Int32Array>(numElementsPath);

  usize totalPoints = featureIds.getNumberOfTuples();
  usize numfeatures = volumes.getNumberOfTuples();

  if(!igeom->getElementSizes())
  {
    int32_t err = igeom->findElementSizes();
    if(err < 0)
    {
      std::string ss = fmt::format("Error computing Element sizes for Geometry type {}", igeom->getGeometryTypeAsString());
      return {nonstd::make_unexpected(std::vector<Error>{Error{err, ss}})};
    }
  }

  const Float32Array* elemSizes = igeom->getElementSizes();

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
    igeom->deleteElementSizes();
  }

  return {};
}

Result<> CalculateFeatureSizesFilter::findSizes(DataStructure& data, const Arguments& args) const
{
  auto geomPath = args.value<DataPath>(k_GeometryPath_Key);
  auto* geom = data.getDataAs<AbstractGeometry>(geomPath);

  // If the geometry is an ImageGeometry or a RectilinearGeometry
  auto* imageGeom = dynamic_cast<ImageGeom*>(geom);
  if(nullptr != imageGeom)
  {
    return findSizesImage(data, args, imageGeom);
  }
  return findSizesUnstructured(data, args, geom);
}
} // namespace complex
