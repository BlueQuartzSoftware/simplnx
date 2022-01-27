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
  return "Calculate Feature Sizes (Find Feature Sizes)";
}

//------------------------------------------------------------------------------
std::vector<std::string> CalculateFeatureSizesFilter::defaultTags() const
{
  return {"#Statistics", "#Morphological", "#Feature Calculation", "#Find Feature Sizes"};
}

Parameters CalculateFeatureSizesFilter::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<BoolParameter>(k_SaveElementSizes_Key, "Save Element Sizes", "Save element sizes", false));
  params.insert(std::make_unique<DataPathSelectionParameter>(k_GeometryPath_Key, "Target Geometry", "DataPath to target geometry", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsPath_Key, "Feature IDs", "DataPath to Feature IDs array", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_VolumesPath_Key, "Volumes array", "DataPath to volumes array", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_EquivalentDiametersPath_Key, "Equivalent Diameters", "DataPath to equivalent diameters array", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NumElementsPath_Key, "Num Elements", "DataPath to Num Elements array", DataPath{}));
  return params;
}

IFilter::UniquePointer CalculateFeatureSizesFilter::clone() const
{
  return std::make_unique<CalculateFeatureSizesFilter>();
}

IFilter::PreflightResult CalculateFeatureSizesFilter::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  auto geometryPath = args.value<DataPath>(k_GeometryPath_Key);

  auto featureIdsPath = args.value<DataPath>(k_FeatureIdsPath_Key);
  auto volumesPath = args.value<DataPath>(k_VolumesPath_Key);
  auto equivalentDiametersPath = args.value<DataPath>(k_EquivalentDiametersPath_Key);
  auto numElementsPath = args.value<DataPath>(k_NumElementsPath_Key);

  auto featureIdsArray = data.getDataAs<Int32Array>(featureIdsPath);

  auto geometry = data.getDataAs<AbstractGeometry>(geometryPath);

  if(geometry == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingGeometry, "Could not find the target geometry."}})};
  }

  if(featureIdsArray == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingFeatureIds, "Could not find Feature IDs array."}})};
  }

  std::vector<usize> tupleDimensions = {featureIdsArray->getNumberOfTuples()};
  uint64 numberOfComponents = 1;

  auto createVolumesAction = std::make_unique<CreateArrayAction>(NumericType::float32, tupleDimensions, std::vector<usize>{numberOfComponents}, volumesPath);
  auto createEquivalentDiametersAction = std::make_unique<CreateArrayAction>(NumericType::float32, tupleDimensions, std::vector<usize>{numberOfComponents}, equivalentDiametersPath);
  auto createNumElementsAction = std::make_unique<CreateArrayAction>(NumericType::int32, tupleDimensions, std::vector<usize>{numberOfComponents}, numElementsPath);

  OutputActions actions;
  actions.actions.push_back(std::move(createVolumesAction));
  actions.actions.push_back(std::move(createEquivalentDiametersAction));
  actions.actions.push_back(std::move(createNumElementsAction));

  return {std::move(actions)};
}

Result<> CalculateFeatureSizesFilter::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  return std::move(findSizes(data, args));
}

Result<> CalculateFeatureSizesFilter::findSizesImage(DataStructure& data, const Arguments& args, ImageGeom* image) const
{
  auto saveElementSizes = args.value<bool>(k_SaveElementSizes_Key);

  auto featureIdsPath = args.value<DataPath>(k_FeatureIdsPath_Key);
  auto volumesPath = args.value<DataPath>(k_VolumesPath_Key);
  auto equivalentDiametersPath = args.value<DataPath>(k_EquivalentDiametersPath_Key);
  auto numElementsPath = args.value<DataPath>(k_NumElementsPath_Key);

  auto featureIdsArray = data.getDataAs<Int32Array>(featureIdsPath);
  auto volumesArray = data.getDataAs<Float32Array>(volumesPath);
  auto equivalentDiametersArray = data.getDataAs<Float32Array>(equivalentDiametersPath);
  auto numElementsArray = data.getDataAs<Int32Array>(numElementsPath);

  usize totalPoints = featureIdsArray->getNumberOfTuples();
  usize numfeatures = volumesArray->getNumberOfTuples();

  auto featureIds = featureIdsArray->getDataStore();
  auto volumes = volumesArray->getDataStore();
  auto equivalentDiameters = equivalentDiametersArray->getDataStore();
  auto numElements = numElementsArray->getDataStore();

  DataStructure tempStructure;
  auto featureCounts = UInt64Array::CreateWithStore<DataStore<uint64>>(tempStructure, std::string("_INTERNAL_USE_ONLY_FeatureCounts"), std::vector<usize>{numfeatures}, std::vector<usize>{1});
  featureCounts->fill(0);
  auto featurecounts = featureCounts->getDataStore();

  float rad = 0.0f;
  float diameter = 0.0f;
  float res_scalar = 0.0f;

  for(size_t j = 0; j < totalPoints; j++)
  {
    int32_t gnum = featureIds->getValue(j);
    auto temp = featurecounts->getValue(gnum) + 1;
    featurecounts->setValue(gnum, temp);
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
      numElements->setValue(i, static_cast<int32_t>(featurecounts->getValue(i)));
      if(featurecounts->getValue(i) > 9007199254740992ULL)
      {
        std::string ss = fmt::format("Number of voxels belonging to feature {} ({}) is greater than 9007199254740992", i, featurecounts->getValue(i));
        return {nonstd::make_unexpected(std::vector<Error>{Error{k_BadFeatureCount, ss}})};
      }
      volumes->setValue(i, static_cast<double>(featurecounts->getValue(i)) * static_cast<double>(res_scalar));

      rad = volumes->getValue(i) / k_PI;
      diameter = (2 * sqrtf(rad));
      equivalentDiameters->setValue(i, diameter);
    }
  }
  else
  {
    res_scalar = spacing[0] * spacing[1] * spacing[2];
    float vol_term = (4.0f / 3.0f) * k_PI;
    for(usize i = 1; i < numfeatures; i++)
    {
      numElements->setValue(i, static_cast<int32>(featurecounts->getValue(i)));
      if(featurecounts->getValue(i) > 9007199254740992ULL)
      {
        std::string ss = fmt::format("Number of voxels belonging to feature {} ({}) is greater than 9007199254740992", i, featurecounts->getValue(i));
        return {nonstd::make_unexpected(std::vector<Error>{Error{k_BadFeatureCount, ss}})};
      }

      volumes->setValue(i, static_cast<double>(featurecounts->getValue(i)) * static_cast<double>(res_scalar));

      rad = volumes->getValue(i) / vol_term;
      diameter = 2.0f * powf(rad, 0.3333333333f);
      equivalentDiameters->setValue(i, diameter);
    }
  }

  if(saveElementSizes)
  {
    if(!image->getElementSizes())
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

  auto featureIdsPath = args.value<DataPath>(k_FeatureIdsPath_Key);
  auto volumesPath = args.value<DataPath>(k_VolumesPath_Key);
  auto equivalentDiametersPath = args.value<DataPath>(k_EquivalentDiametersPath_Key);
  auto numElementsPath = args.value<DataPath>(k_NumElementsPath_Key);

  auto featureIdsArray = data.getDataAs<Int32Array>(featureIdsPath);
  auto volumesArray = data.getDataAs<Float32Array>(volumesPath);
  auto equivalentDiametersArray = data.getDataAs<Float32Array>(equivalentDiametersPath);
  auto numElementsArray = data.getDataAs<Int32Array>(numElementsPath);

  usize totalPoints = featureIdsArray->getNumberOfTuples();
  usize numfeatures = volumesArray->getNumberOfTuples();

  auto featureIds = featureIdsArray->getDataStore();
  auto volumes = volumesArray->getDataStore();
  auto equivalentDiameters = equivalentDiametersArray->getDataStore();
  auto numElements = numElementsArray->getDataStore();

  if(!igeom->getElementSizes())
  {
    int32_t err = igeom->findElementSizes();
    if(err < 0)
    {
      std::string ss = fmt::format("Error computing Element sizes for Geometry type {}", igeom->getGeometryTypeAsString());
      return {nonstd::make_unexpected(std::vector<Error>{Error{err, ss}})};
    }
  }

  auto elemSizes = igeom->getElementSizes();
  auto sizes = elemSizes->getDataStore();

  DataStructure tempStructure;
  auto featureCountsArray = Float32Array::CreateWithStore<DataStore<float32>>(tempStructure, std::string("_INTERNAL_USE_ONLY_FeatureCounts"), std::vector<usize>{numfeatures}, std::vector<usize>{1});
  featureCountsArray->fill(0);
  auto featureCounts = featureCountsArray->getDataStore();

  float rad = 0.0f;
  float diameter = 0.0f;

  for(size_t j = 0; j < totalPoints; j++)
  {
    int32 gnum = featureIds->getValue(j);
    auto temp = featureCounts->getValue(gnum) + 1;
    featureCounts->setValue(gnum, temp);
    auto temp2 = volumes->getValue(gnum);
    volumes->setValue(gnum, temp2 + sizes->getValue(j));
  }
  float vol_term = (4.0f / 3.0f) * k_PI;
  for(size_t i = 1; i < numfeatures; i++)
  {
    numElements->setValue(i, static_cast<int32>(featureCounts->getValue(i)));
    rad = volumes->getValue(i) / vol_term;
    diameter = 2.0f * powf(rad, 0.3333333333f);
    equivalentDiameters->setValue(i, diameter);
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
  auto geom = data.getDataAs<AbstractGeometry>(geomPath);

  // If the geometry is an Image, it may be 2D;
  // if so, call the specialized findSizesImage() function
  if(auto image = dynamic_cast<ImageGeom*>(geom))
  {
    return findSizesImage(data, args, image);
  }
  else
  {
    return findSizesUnstructured(data, args, geom);
  }
}
} // namespace complex
