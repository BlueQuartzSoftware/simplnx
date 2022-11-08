#include "FindSurfaceFeatures.hpp"

#include "complex/Common/Point2D.hpp"
#include "complex/Common/Point3D.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

using namespace complex;

namespace
{
bool isPointASurfaceFeature(const Point2D<usize>& point, usize xPoints, usize yPoints, bool markFeature0Neighbors, const Int32Array& featureIds)
{
  usize yStride = point.getY() * xPoints;

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

bool isPointASurfaceFeature(const Point3D<usize>& point, usize xPoints, usize yPoints, usize zPoints, bool markFeature0Neighbors, const Int32Array& featureIds)
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

void findSurfaceFeatures3D(DataStructure& ds, const DataPath& featureGeometryPathValue, const DataPath& featureIdsArrayPathValue, const DataPath& surfaceFeaturesArrayPathValue,
                           bool markFeature0Neighbors, const std::atomic_bool& shouldCancel)
{
  const ImageGeom& featureGeometry = ds.getDataRefAs<ImageGeom>(featureGeometryPathValue);
  const Int32Array& featureIds = ds.getDataRefAs<Int32Array>(featureIdsArrayPathValue);
  BoolArray& surfaceFeatures = ds.getDataRefAs<BoolArray>(surfaceFeaturesArrayPathValue);

  usize xPoints = featureGeometry.getNumXCells();
  usize yPoints = featureGeometry.getNumYCells();
  usize zPoints = featureGeometry.getNumZCells();

  for(usize z = 0; z < zPoints; z++)
  {
    usize zStride = z * xPoints * yPoints;
    for(usize y = 0; y < yPoints; y++)
    {
      usize yStride = y * xPoints;
      for(usize x = 0; x < xPoints; x++)
      {
        if(shouldCancel)
        {
          return;
        }

        int32 gnum = featureIds[zStride + yStride + x];
        if(gnum != 0 && !surfaceFeatures[gnum])
        {
          if(isPointASurfaceFeature(Point3D{x, y, z}, xPoints, yPoints, zPoints, markFeature0Neighbors, featureIds))
          {
            surfaceFeatures[gnum] = true;
          }
        }
      }
    }
  }
}

void findSurfaceFeatures2D(DataStructure& ds, const DataPath& featureGeometryPathValue, const DataPath& featureIdsArrayPathValue, const DataPath& surfaceFeaturesArrayPathValue,
                           bool markFeature0Neighbors, const std::atomic_bool& shouldCancel)
{
  const ImageGeom& featureGeometry = ds.getDataRefAs<ImageGeom>(featureGeometryPathValue);
  const Int32Array& featureIds = ds.getDataRefAs<Int32Array>(featureIdsArrayPathValue);
  BoolArray& surfaceFeatures = ds.getDataRefAs<BoolArray>(surfaceFeaturesArrayPathValue);

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
    usize yStride = y * xPoints;
    for(usize x = 0; x < xPoints; x++)
    {
      if(shouldCancel)
      {
        return;
      }

      int32 gnum = featureIds[yStride + x];
      if(gnum != 0 && surfaceFeatures[gnum] == 0)
      {
        if(isPointASurfaceFeature(Point2D{x, y}, xPoints, yPoints, markFeature0Neighbors, featureIds))
        {
          surfaceFeatures[gnum] = 1;
        }
      }
    }
  }
}
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string FindSurfaceFeatures::name() const
{
  return FilterTraits<FindSurfaceFeatures>::name.str();
}

//------------------------------------------------------------------------------
std::string FindSurfaceFeatures::className() const
{
  return FilterTraits<FindSurfaceFeatures>::className;
}

//------------------------------------------------------------------------------
Uuid FindSurfaceFeatures::uuid() const
{
  return FilterTraits<FindSurfaceFeatures>::uuid;
}

//------------------------------------------------------------------------------
std::string FindSurfaceFeatures::humanName() const
{
  return "Find Surface Features";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindSurfaceFeatures::defaultTags() const
{
  return {"#Generic", "#Spatial"};
}

//------------------------------------------------------------------------------
Parameters FindSurfaceFeatures::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<CommentParameter>(k_FilterComment_Key, "Comments", "User notes/comments", ""));
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<BoolParameter>(k_MarkFeature0Neighbors, "Mark Feature 0 Neighbors",
                                                "Marks features that are neighbors with feature 0.  If this option is off, only features that reside on the edge of the geometry will be marked.",
                                                true));
  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_FeatureGeometryPath_Key, "Feature Geometry", "The geometry in which to find surface features", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Feature Ids", "Specifies to which Feature each cell belongs", DataPath({"CellData", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Created  Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_SurfaceFeaturesArrayPath_Key, "Surface Features",
                                                         "The path to the created surface features array. Flag of 1 if Feature touches an outer surface or of 0 if it does not",
                                                         DataPath({"CellFeatureData", "SurfaceFeatures"})));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindSurfaceFeatures::clone() const
{
  return std::make_unique<FindSurfaceFeatures>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindSurfaceFeatures::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                            const std::atomic_bool& shouldCancel) const
{
  auto pFeatureGeometryPathValue = filterArgs.value<DataPath>(k_FeatureGeometryPath_Key);
  auto pSurfaceFeaturesArrayPathValue = filterArgs.value<DataPath>(k_SurfaceFeaturesArrayPath_Key);

  const ImageGeom& featureGeometry = dataStructure.getDataRefAs<ImageGeom>(pFeatureGeometryPathValue);
  usize geometryDimensionality = featureGeometry.getDimensionality();
  if(geometryDimensionality != 3 && geometryDimensionality != 2)
  {
    return {MakeErrorResult<OutputActions>(-1000, fmt::format("Image Geometry at path '{}' must be either 3D or 2D", pFeatureGeometryPathValue.toString()))};
  }

  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  {
    auto createSurfaceFeaturesAction = std::make_unique<CreateArrayAction>(DataType::boolean, std::vector<usize>{1}, std::vector<usize>{1}, pSurfaceFeaturesArrayPathValue);
    resultOutputActions.value().actions.push_back(std::move(createSurfaceFeaturesAction));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindSurfaceFeatures::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                          const std::atomic_bool& shouldCancel) const
{
  auto pFeatureGeometryPathValue = filterArgs.value<DataPath>(k_FeatureGeometryPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pSurfaceFeaturesArrayPathValue = filterArgs.value<DataPath>(k_SurfaceFeaturesArrayPath_Key);
  auto pMarkFeature0NeighborsValue = filterArgs.value<bool>(k_MarkFeature0Neighbors);

  // Resize the surface features array to the proper size
  const Int32Array& featureIds = dataStructure.getDataRefAs<Int32Array>(pFeatureIdsArrayPathValue);
  BoolArray& surfaceFeatures = dataStructure.getDataRefAs<BoolArray>(pSurfaceFeaturesArrayPathValue);
  DataStore<bool>& surfaceFeaturesStore = surfaceFeatures.getIDataStoreRefAs<DataStore<bool>>();

  usize featureIdsMaxIdx = std::distance(featureIds.begin(), std::max_element(featureIds.cbegin(), featureIds.cend()));
  usize maxFeature = featureIds[featureIdsMaxIdx];

  surfaceFeaturesStore.reshapeTuples(std::vector<usize>{maxFeature + 1});
  surfaceFeaturesStore.fill(0);

  // Find surface features
  ImageGeom& featureGeometry = dataStructure.getDataRefAs<ImageGeom>(pFeatureGeometryPathValue);
  usize geometryDimensionality = featureGeometry.getDimensionality();
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
} // namespace complex
