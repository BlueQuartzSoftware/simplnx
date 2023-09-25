#include "RegularGridSampleSurfaceMeshFilter.hpp"

#include "ComplexCore/Filters/Algorithms/RegularGridSampleSurfaceMesh.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "complex/Filter/Actions/CreateImageGeometryAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <sstream>

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string RegularGridSampleSurfaceMeshFilter::name() const
{
  return FilterTraits<RegularGridSampleSurfaceMeshFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string RegularGridSampleSurfaceMeshFilter::className() const
{
  return FilterTraits<RegularGridSampleSurfaceMeshFilter>::className;
}

//------------------------------------------------------------------------------
Uuid RegularGridSampleSurfaceMeshFilter::uuid() const
{
  return FilterTraits<RegularGridSampleSurfaceMeshFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string RegularGridSampleSurfaceMeshFilter::humanName() const
{
  return "Sample Triangle Geometry on Regular Grid";
}

//------------------------------------------------------------------------------
std::vector<std::string> RegularGridSampleSurfaceMeshFilter::defaultTags() const
{
  return {className(), "Sampling", "Spacing"};
}

//------------------------------------------------------------------------------
Parameters RegularGridSampleSurfaceMeshFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Parameters"});
  params.insert(std::make_unique<VectorUInt64Parameter>(k_Dimensions_Key, "Dimensions (Voxels)", "The dimensions of the created Image geometry", std::vector<uint64>{128, 128, 128},
                                                        std::vector<std::string>{"x", "y", "z"}));
  params.insert(
      std::make_unique<VectorFloat32Parameter>(k_Origin_Key, "Origin", "The origin of the created Image geometry", std::vector<float32>{0.0F, 0.0F, 0.0F}, std::vector<std::string>{"x", "y", "z"}));
  params.insert(
      std::make_unique<VectorFloat32Parameter>(k_Spacing_Key, "Spacing", "The spacing of the created Image geometry", std::vector<float32>{1.0F, 1.0F, 1.0F}, std::vector<std::string>{"x", "y", "z"}));

  params.insert(std::make_unique<ChoicesParameter>(k_LengthUnit_Key, "Length Units (For Description Only)", "The units to be displayed below", to_underlying(IGeometry::LengthUnit::Micrometer),
                                                   IGeometry::GetAllLengthUnitStrings()));

  params.insertSeparator(Parameters::Separator{"Required Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriangleGeometryPath_Key, "Triangle Geometry", "The geometry to be sampled onto grid", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{GeometrySelectionParameter::AllowedType ::Triangle}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "Array specifying which Features are on either side of each Face", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{complex::DataType::int32}));

  params.insertSeparator(Parameters::Separator{"Created Objects"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_ImageGeomPath_Key, "Image Geometry", "The name and path for the image geometry to be created", DataPath{}));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellAMName_Key, "Cell Data Name", "The name for the cell data Attribute Matrix within the Image geometry", "Cell Data"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureIdsArrayName_Key, "Feature Ids Name", "The name for the feature ids array in cell data Attribute Matrix", "Feature Ids"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer RegularGridSampleSurfaceMeshFilter::clone() const
{
  return std::make_unique<RegularGridSampleSurfaceMeshFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult RegularGridSampleSurfaceMeshFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                           const std::atomic_bool& shouldCancel) const
{
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pDimensionsValue = filterArgs.value<VectorUInt64Parameter::ValueType>(k_Dimensions_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pLengthUnitValue = filterArgs.value<ChoicesParameter::ValueType>(k_LengthUnit_Key);
  auto pImageGeomPathValue = filterArgs.value<DataPath>(k_ImageGeomPath_Key);
  auto pCellAMNameValue = filterArgs.value<std::string>(k_CellAMName_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<std::string>(k_FeatureIdsArrayName_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;

  std::vector<usize> tupleDims = {static_cast<usize>(pDimensionsValue[0]), static_cast<usize>(pDimensionsValue[1]), static_cast<usize>(pDimensionsValue[2])};

  {
    auto createDataGroupAction = std::make_unique<CreateImageGeometryAction>(pImageGeomPathValue, tupleDims, std::vector<float32>(pOriginValue), std::vector<float32>(pSpacingValue), pCellAMNameValue);
    resultOutputActions.value().appendAction(std::move(createDataGroupAction));
  }
  DataPath featIdsPath = pImageGeomPathValue.createChildPath(pCellAMNameValue).createChildPath(pFeatureIdsArrayNameValue);
  {
    auto createDataGroupAction = std::make_unique<CreateArrayAction>(DataType::int32, tupleDims, std::vector<usize>{1}, featIdsPath);
    resultOutputActions.value().appendAction(std::move(createDataGroupAction));
  }

  std::vector<PreflightValue> preflightUpdatedValues;
  std::stringstream boxDimensions = std::stringstream();

  std::string lengthUnit = IGeometry::LengthUnitToString(static_cast<IGeometry::LengthUnit>(pLengthUnitValue));

  boxDimensions << "X Range: " << std::setprecision(8) << std::noshowpoint << pOriginValue[0] << " to " << std::setprecision(8) << std::noshowpoint
                << (pOriginValue[0] + (pDimensionsValue[0] * pSpacingValue[0])) << " (Delta: " << std::setprecision(8) << std::noshowpoint << (pDimensionsValue[0] * pSpacingValue[0]) << ") "
                << lengthUnit << "\n";
  boxDimensions << "Y Range: " << std::setprecision(8) << std::noshowpoint << pOriginValue[1] << " to " << std::setprecision(8) << std::noshowpoint
                << (pOriginValue[1] + (pDimensionsValue[1] * pSpacingValue[1])) << " (Delta: " << std::setprecision(8) << std::noshowpoint << (pDimensionsValue[1] * pSpacingValue[1]) << ") "
                << lengthUnit << "\n";
  boxDimensions << "Z Range: " << std::setprecision(8) << std::noshowpoint << pOriginValue[2] << " to " << std::setprecision(8) << std::noshowpoint
                << (pOriginValue[2] + (pDimensionsValue[2] * pSpacingValue[2])) << " (Delta: " << std::setprecision(8) << std::noshowpoint << (pDimensionsValue[2] * pSpacingValue[2]) << ") "
                << lengthUnit << "\n";

  float32 vol = (pDimensionsValue[0] * pSpacingValue[0]) * (pDimensionsValue[1] * pSpacingValue[1]) * (pDimensionsValue[2] * pSpacingValue[2]);

  boxDimensions << "Volume: " << std::setprecision(8) << std::noshowpoint << vol << " " << lengthUnit << "s ^3"
                << "\n";

  preflightUpdatedValues.push_back({"BoxDimensions", boxDimensions.str()});

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> RegularGridSampleSurfaceMeshFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel) const
{
  RegularGridSampleSurfaceMeshInputValues inputValues;

  inputValues.Dimensions = filterArgs.value<VectorUInt64Parameter::ValueType>(k_Dimensions_Key);
  inputValues.Spacing = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  inputValues.Origin = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  inputValues.TriangleGeometryPath = filterArgs.value<DataPath>(k_TriangleGeometryPath_Key);
  inputValues.SurfaceMeshFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  inputValues.FeatureIdsArrayPath =
      filterArgs.value<DataPath>(k_ImageGeomPath_Key).createChildPath(filterArgs.value<std::string>(k_CellAMName_Key)).createChildPath(filterArgs.value<std::string>(k_FeatureIdsArrayName_Key));

  return RegularGridSampleSurfaceMesh(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
