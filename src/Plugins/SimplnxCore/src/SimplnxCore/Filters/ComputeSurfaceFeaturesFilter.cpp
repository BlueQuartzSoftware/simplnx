#include "ComputeSurfaceFeaturesFilter.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Utilities/DataArrayUtilities.hpp"

using namespace nx::core;

namespace
{
bool IsPointASurfaceFeature(const Point2D<usize>& point, usize xPoints, usize yPoints, bool markFeature0Neighbors, const Int32Array& featureIds)
{
  const usize yStride = point.getY() * xPoints;

  if(point.getX() <= 0 || point.getX() >= xPoints - 1)
  {
    return true;
  }
  if(point.getY() <= 0 || point.getY() >= yPoints - 1)
  {
    return true;
  }

  if(markFeature0Neighbors)
  {
    if(featureIds[yStride + point.getX() - 1] == 0)
    {
      return true;
    }
    if(featureIds[yStride + point.getX() + 1] == 0)
    {
      return true;
    }
    if(featureIds[yStride + point.getX() - xPoints] == 0)
    {
      return true;
    }
    if(featureIds[yStride + point.getX() + xPoints] == 0)
    {
      return true;
    }
  }

  return false;
}

bool IsPointASurfaceFeature(const Point3D<usize>& point, usize xPoints, usize yPoints, usize zPoints, bool markFeature0Neighbors, const Int32Array& featureIds)
{
  usize yStride = point.getY() * xPoints;
  usize zStride = point.getZ() * xPoints * yPoints;

  if(point.getX() <= 0 || point.getX() >= xPoints - 1)
  {
    return true;
  }
  if(point.getY() <= 0 || point.getY() >= yPoints - 1)
  {
    return true;
  }
  if(point.getZ() <= 0 || point.getZ() >= zPoints - 1)
  {
    return true;
  }

  if(markFeature0Neighbors)
  {
    if(featureIds[zStride + yStride + point.getX() - 1] == 0)
    {
      return true;
    }
    if(featureIds[zStride + yStride + point.getX() + 1] == 0)
    {
      return true;
    }
    if(featureIds[zStride + yStride + point.getX() - xPoints] == 0)
    {
      return true;
    }
    if(featureIds[zStride + yStride + point.getX() + xPoints] == 0)
    {
      return true;
    }
    if(featureIds[zStride + yStride + point.getX() - (xPoints * yPoints)] == 0)
    {
      return true;
    }
    if(featureIds[zStride + yStride + point.getX() + (xPoints * yPoints)] == 0)
    {
      return true;
    }
  }

  return false;
}

void findSurfaceFeatures3D(DataStructure& dataStructure, const DataPath& featureGeometryPathValue, const DataPath& featureIdsArrayPathValue, const DataPath& surfaceFeaturesArrayPathValue,
                           bool markFeature0Neighbors, const std::atomic_bool& shouldCancel)
{
  const auto& featureGeometry = dataStructure.getDataRefAs<ImageGeom>(featureGeometryPathValue);
  const auto& featureIds = dataStructure.getDataRefAs<Int32Array>(featureIdsArrayPathValue);
  auto& surfaceFeatures = dataStructure.getDataRefAs<BoolArray>(surfaceFeaturesArrayPathValue);

  const usize xPoints = featureGeometry.getNumXCells();
  const usize yPoints = featureGeometry.getNumYCells();
  const usize zPoints = featureGeometry.getNumZCells();

  for(usize z = 0; z < zPoints; z++)
  {
    const usize zStride = z * xPoints * yPoints;
    for(usize y = 0; y < yPoints; y++)
    {
      const usize yStride = y * xPoints;
      for(usize x = 0; x < xPoints; x++)
      {
        if(shouldCancel)
        {
          return;
        }

        const int32 gNum = featureIds[zStride + yStride + x];
        if(gNum != 0 && !surfaceFeatures[gNum])
        {
          if(IsPointASurfaceFeature(Point3D<usize>{x, y, z}, xPoints, yPoints, zPoints, markFeature0Neighbors, featureIds))
          {
            surfaceFeatures[gNum] = true;
          }
        }
      }
    }
  }
}

void findSurfaceFeatures2D(DataStructure& dataStructure, const DataPath& featureGeometryPathValue, const DataPath& featureIdsArrayPathValue, const DataPath& surfaceFeaturesArrayPathValue,
                           bool markFeature0Neighbors, const std::atomic_bool& shouldCancel)
{
  const auto& featureGeometry = dataStructure.getDataRefAs<ImageGeom>(featureGeometryPathValue);
  const auto& featureIds = dataStructure.getDataRefAs<Int32Array>(featureIdsArrayPathValue);
  auto& surfaceFeatures = dataStructure.getDataRefAs<BoolArray>(surfaceFeaturesArrayPathValue);

  usize xPoints = 0;
  usize yPoints = 0;

  if(featureGeometry.getNumXCells() == 1)
  {
    xPoints = featureGeometry.getNumYCells();
    yPoints = featureGeometry.getNumZCells();
  }
  if(featureGeometry.getNumYCells() == 1)
  {
    xPoints = featureGeometry.getNumXCells();
    yPoints = featureGeometry.getNumZCells();
  }
  if(featureGeometry.getNumZCells() == 1)
  {
    xPoints = featureGeometry.getNumXCells();
    yPoints = featureGeometry.getNumYCells();
  }

  for(usize y = 0; y < yPoints; y++)
  {
    const usize yStride = y * xPoints;

    for(usize x = 0; x < xPoints; x++)
    {
      if(shouldCancel)
      {
        return;
      }

      const int32 gNum = featureIds[yStride + x];
      if(gNum != 0 && surfaceFeatures[gNum] == 0)
      {
        if(IsPointASurfaceFeature(Point2D<usize>{x, y}, xPoints, yPoints, markFeature0Neighbors, featureIds))
        {
          surfaceFeatures[gNum] = true;
        }
      }
    }
  }
}
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeSurfaceFeaturesFilter::name() const
{
  return FilterTraits<ComputeSurfaceFeaturesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeSurfaceFeaturesFilter::className() const
{
  return FilterTraits<ComputeSurfaceFeaturesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeSurfaceFeaturesFilter::uuid() const
{
  return FilterTraits<ComputeSurfaceFeaturesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeSurfaceFeaturesFilter::humanName() const
{
  return "Compute Surface Features";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeSurfaceFeaturesFilter::defaultTags() const
{
  return {className(), "Generic", "Spatial", "Find", "Generate", "Calculate", "Determine"};
}

//------------------------------------------------------------------------------
Parameters ComputeSurfaceFeaturesFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<BoolParameter>(k_MarkFeature0Neighbors, "Mark Feature 0 Neighbors",
                                                "Marks features that are neighbors with feature 0.  If this option is off, only features that reside on the edge of the geometry will be marked.",
                                                true));
  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_FeatureGeometryPath_Key, "Feature Geometry", "The geometry in which to find surface features", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which Feature each cell belongs", DataPath({"Cell Data", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Cell Feature Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_CellFeatureAttributeMatrixPath_Key, "Feature Attribute Matrix",
                                                              "The path to the cell feature attribute matrix associated with the input feature ids array", DataPath{},
                                                              DataGroupSelectionParameter::AllowedTypes{BaseGroup::GroupType::AttributeMatrix}));
  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_SurfaceFeaturesArrayName_Key, "Surface Features",
                                                          "The created surface features array. Flag of 1 if Feature touches an outer surface or of 0 if it does not", "SurfaceFeatures"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeSurfaceFeaturesFilter::clone() const
{
  return std::make_unique<ComputeSurfaceFeaturesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeSurfaceFeaturesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                     const std::atomic_bool& shouldCancel) const
{
  auto pFeatureGeometryPathValue = filterArgs.value<DataPath>(k_FeatureGeometryPath_Key);
  auto pCellFeaturesAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);
  auto pSurfaceFeaturesArrayNameValue = filterArgs.value<std::string>(k_SurfaceFeaturesArrayName_Key);

  const auto& featureGeometry = dataStructure.getDataRefAs<ImageGeom>(pFeatureGeometryPathValue);
  usize geometryDimensionality = featureGeometry.getDimensionality();
  if(geometryDimensionality != 3 && geometryDimensionality != 2)
  {
    return {MakeErrorResult<OutputActions>(-1000, fmt::format("Image Geometry at path '{}' must be either 3D or 2D", pFeatureGeometryPathValue.toString()))};
  }

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  std::vector<usize> tupleDims = std::vector<usize>{1};
  if(const auto& surfaceFeaturesParent = dataStructure.getDataAs<AttributeMatrix>(pCellFeaturesAttributeMatrixPathValue); surfaceFeaturesParent != nullptr)
  {
    tupleDims = surfaceFeaturesParent->getShape();
  }

  auto createSurfaceFeaturesAction =
      std::make_unique<CreateArrayAction>(DataType::boolean, tupleDims, std::vector<usize>{1}, pCellFeaturesAttributeMatrixPathValue.createChildPath(pSurfaceFeaturesArrayNameValue));
  resultOutputActions.value().appendAction(std::move(createSurfaceFeaturesAction));

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeSurfaceFeaturesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel) const
{
  const auto pMarkFeature0NeighborsValue = filterArgs.value<bool>(k_MarkFeature0Neighbors);
  const auto pFeatureGeometryPathValue = filterArgs.value<DataPath>(k_FeatureGeometryPath_Key);
  const auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  const auto pCellFeaturesAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);
  const auto pSurfaceFeaturesArrayPathValue = pCellFeaturesAttributeMatrixPathValue.createChildPath(filterArgs.value<std::string>(k_SurfaceFeaturesArrayName_Key));

  // Resize the surface features array to the proper size
  const Int32Array& featureIds = dataStructure.getDataRefAs<Int32Array>(pFeatureIdsArrayPathValue);
  auto& surfaceFeatures = dataStructure.getDataRefAs<BoolArray>(pSurfaceFeaturesArrayPathValue);
  auto& surfaceFeaturesStore = surfaceFeatures.getDataStoreRef();

  const usize featureIdsMaxIdx = std::distance(featureIds.begin(), std::max_element(featureIds.cbegin(), featureIds.cend()));
  const usize maxFeature = featureIds[featureIdsMaxIdx];
  const std::vector<usize> surfaceFeaturesTupleShape = {maxFeature + 1};

  Result<> resizeResults = ResizeDataArray<bool>(dataStructure, pSurfaceFeaturesArrayPathValue, surfaceFeaturesTupleShape);
  if(resizeResults.invalid())
  {
    return resizeResults;
  }
  surfaceFeaturesStore.fill(false);

  // Find surface features
  const auto& featureGeometry = dataStructure.getDataRefAs<ImageGeom>(pFeatureGeometryPathValue);
  const usize geometryDimensionality = featureGeometry.getDimensionality();
  if(geometryDimensionality == 3)
  {
    findSurfaceFeatures3D(dataStructure, pFeatureGeometryPathValue, pFeatureIdsArrayPathValue, pSurfaceFeaturesArrayPathValue, pMarkFeature0NeighborsValue, shouldCancel);
  }
  else if(geometryDimensionality == 2)
  {
    findSurfaceFeatures2D(dataStructure, pFeatureGeometryPathValue, pFeatureIdsArrayPathValue, pSurfaceFeaturesArrayPathValue, pMarkFeature0NeighborsValue, shouldCancel);
  }
  else
  {
    MakeErrorResult(-1000, fmt::format("Image Geometry at path '{}' must be either 3D or 2D", pFeatureGeometryPathValue.toString()));
  }

  return {};
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_SurfaceFeaturesArrayPathKey = "SurfaceFeaturesArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeSurfaceFeaturesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeSurfaceFeaturesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_FeatureGeometryPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_CellFeatureAttributeMatrixPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayNameFilterParameterConverter>(args, json, SIMPL::k_SurfaceFeaturesArrayPathKey, k_SurfaceFeaturesArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
