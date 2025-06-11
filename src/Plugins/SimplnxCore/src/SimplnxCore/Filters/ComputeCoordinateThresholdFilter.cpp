#include "ComputeCoordinateThresholdFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ComputeCoordinateThreshold.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/IntersectionUtilities.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeCoordinateThresholdFilter::name() const
{
  return FilterTraits<ComputeCoordinateThresholdFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeCoordinateThresholdFilter::className() const
{
  return FilterTraits<ComputeCoordinateThresholdFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeCoordinateThresholdFilter::uuid() const
{
  return FilterTraits<ComputeCoordinateThresholdFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeCoordinateThresholdFilter::humanName() const
{
  return "Compute Coordinate Threshold";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeCoordinateThresholdFilter::defaultTags() const
{
  return {className(), "Mask", "Statistics"};
}

//------------------------------------------------------------------------------
Parameters ComputeCoordinateThresholdFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insertLinkableParameter(
      std::make_unique<ChoicesParameter>(k_ContainerShapeType_Key, "Coordinate Container Shape", "This will determine how the bounding box for determining included points is defined",
                                         to_underlying(ComputeCoordinateThreshold::BoundsType::Rectangle), ChoicesParameter::Choices{"Rectangle", "Sphere"})); // sequence dependent DO NOT REORDER
  params.insert(std::make_unique<BoolParameter>(k_InvertContainer_Key, "Invert Bounding Container",
                                                "If selected, only points/edges/faces outside the container will be marked `true`, else only values in the container will be marked `true`", false));

  params.insertSeparator(Parameters::Separator{"Coordinate Bounds"});
  params.insert(std::make_unique<VectorFloat32Parameter>(k_MinCoord_Key, "Lower Bound (Physical Units)", "Specifies the lower corner of the rectangular prism (bounding box)",
                                                         std::vector<float32>{0.0f, 0.0f, 0.0f}, std::vector<std::string>{"Min X", "Min Y", "Min Z"}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_MaxCoord_Key, "Upper Bound (Physical Units)", "Specifies the upper corner of the rectangular prism (bounding box)",
                                                         std::vector<float32>{1.0f, 1.0f, 1.0f}, std::vector<std::string>{"Max X", "Max Y", "Max Z"}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_SphereInfo_Key, "Sphere centroid and Radius (Physical Units)",
                                                         "Specifies the centroid of the bounding sphere in the first 3 values (XYZ) and radius in the 4th",
                                                         std::vector<float32>{0.0f, 0.0f, 0.0f, 1.0f}, std::vector<std::string>{"X", "Y", "Z", "Radius"}));

  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(
      k_SelectedGeometryPath_Key, "Selected Geometry", "The DataPath to the Geometry that contains the points/edges/faces for the geometry", DataPath{},
      GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Vertex, IGeometry::Type::Edge, IGeometry::Type::Image, IGeometry::Type::Triangle, IGeometry::Type::Quad}));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CreatedMaskPath_Key, "Created Mask Path/Name", "The path/name of the created mask", DataPath({"Geometric Coordinate Mask"})));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_ContainerShapeType_Key, k_MinCoord_Key, static_cast<ChoicesParameter::ValueType>(to_underlying(ComputeCoordinateThreshold::BoundsType::Rectangle)));
  params.linkParameters(k_ContainerShapeType_Key, k_MaxCoord_Key, static_cast<ChoicesParameter::ValueType>(to_underlying(ComputeCoordinateThreshold::BoundsType::Rectangle)));
  params.linkParameters(k_ContainerShapeType_Key, k_SphereInfo_Key, static_cast<ChoicesParameter::ValueType>(to_underlying(ComputeCoordinateThreshold::BoundsType::Sphere)));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeCoordinateThresholdFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeCoordinateThresholdFilter::clone() const
{
  return std::make_unique<ComputeCoordinateThresholdFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeCoordinateThresholdFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                         const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pContainerShapeTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_ContainerShapeType_Key);
  auto pSelectedGeomPathValue = filterArgs.value<GeometrySelectionParameter::ValueType>(k_SelectedGeometryPath_Key);
  auto pCreatedMaskPathValue = filterArgs.value<ArrayCreationParameter::ValueType>(k_CreatedMaskPath_Key);

  const auto& geom = dataStructure.getDataRefAs<IGeometry>(pSelectedGeomPathValue);

  usize numCells = geom.getNumberOfCells();

  switch(static_cast<ComputeCoordinateThreshold::BoundsType>(pContainerShapeTypeValue))
  {
  case ComputeCoordinateThreshold::Rectangle: {
    auto pMinPoint = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MinCoord_Key);
    auto pMaxPoint = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MaxCoord_Key);

    if(pMaxPoint[0] < pMinPoint[0] || pMaxPoint[1] < pMinPoint[1] || pMaxPoint[2] < pMinPoint[2])
    {
      return MakePreflightErrorResult(-24710,
                                      fmt::format("The maximum coordinate has one or more values less than those in the minimum coordinate.\nMin Coord(XYZ): {}, {}, {}\nMax Coord(XYZ): {}, {}, {}",
                                                  pMinPoint[0], pMinPoint[1], pMinPoint[2], pMaxPoint[0], pMaxPoint[1], pMaxPoint[2]));
    }

    if(geom.getGeomType() == IGeometry::Type::Image)
    {
      const auto& imageGeom = dynamic_cast<const ImageGeom&>(geom);

      BoundingBox3Df bounds = imageGeom.getBoundingBoxf();
      const Point3Df& minBound = bounds.getMinPoint();
      const Point3Df& maxBound = bounds.getMaxPoint();

      if(pMaxPoint[0] < minBound[0] || pMaxPoint[1] < minBound[1] || pMaxPoint[2] < minBound[2])
      {
        return MakePreflightErrorResult(-24711, fmt::format("The supplied bounding box falls outside the geometries minimum point, filter will do nothing. Minimum Point in Geometry(XYZ): {}, {}, {}",
                                                            minBound[0], minBound[1], minBound[2]));
      }

      if(pMinPoint[0] > maxBound[0] || pMinPoint[1] > maxBound[1] || pMinPoint[2] > maxBound[2])
      {
        return MakePreflightErrorResult(-24712, fmt::format("The supplied bounding box falls outside the geometries maximum point, filter will do nothing. Maximum Point in Geometry(XYZ): {}, {}, {}",
                                                            maxBound[0], maxBound[1], maxBound[2]));
      }
    }

    break;
  }
  case ComputeCoordinateThreshold::Sphere: {
    auto pSphereInfo = filterArgs.value<VectorFloat32Parameter::ValueType>(k_SphereInfo_Key);

    if(pSphereInfo[3] <= 0)
    {
      return MakePreflightErrorResult(-24713, fmt::format("The spheres radius must be greater than 0. Supplied radius: {}", pSphereInfo[3]));
    }

    if(geom.getGeomType() == IGeometry::Type::Image)
    {
      const auto& imageGeom = dynamic_cast<const ImageGeom&>(geom);

      BoundingBox3Df bounds = imageGeom.getBoundingBoxf();
      const Point3Df& minBound = bounds.getMinPoint();
      const Point3Df& maxBound = bounds.getMaxPoint();

      bool doesIntersect = IntersectionUtilities::SphereIntersectsRectangularPrism({pSphereInfo[0], pSphereInfo[1], pSphereInfo[2]}, pSphereInfo[3], minBound.toArray(), maxBound.toArray());

      if(!doesIntersect)
      {
        return MakePreflightErrorResult(
            -24714,
            fmt::format("The supplied sphere falls outside the geometries bounds, filter will do nothing.\nMinimum Point in Geometry(XYZ): {}, {}, {}\nMaximum Point in Geometry(XYZ): {}, {}, {}",
                        minBound[0], minBound[1], minBound[2], maxBound[0], maxBound[1], maxBound[2]));
      }
    }
    break;
  }
  }

  nx::core::Result<OutputActions> resultOutputActions;
  {
    auto createAction = std::make_unique<CreateArrayAction>(DataType::uint8, std::vector<usize>{numCells}, std::vector<usize>{1}, pCreatedMaskPathValue);
    resultOutputActions.value().appendAction(std::move(createAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ComputeCoordinateThresholdFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                       const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ComputeCoordinateThresholdInputValues inputValues;

  inputValues.ShapeType = filterArgs.value<ChoicesParameter::ValueType>(k_ContainerShapeType_Key);
  inputValues.Invert = filterArgs.value<bool>(k_InvertContainer_Key);

  inputValues.MinCoord = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MinCoord_Key);
  inputValues.MaxCoord = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MaxCoord_Key);
  inputValues.SphereInfo = filterArgs.value<VectorFloat32Parameter::ValueType>(k_SphereInfo_Key);

  inputValues.GeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(k_SelectedGeometryPath_Key);
  inputValues.MaskArrayPath = filterArgs.value<ArrayCreationParameter::ValueType>(k_CreatedMaskPath_Key);

  return ComputeCoordinateThreshold(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
