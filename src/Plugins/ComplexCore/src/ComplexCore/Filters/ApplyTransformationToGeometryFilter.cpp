#include "ApplyTransformationToGeometryFilter.hpp"

#include "ComplexCore/Filters/Algorithms/ApplyTransformationToGeometry.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ApplyTransformationToGeometryFilter::name() const
{
  return FilterTraits<ApplyTransformationToGeometryFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ApplyTransformationToGeometryFilter::className() const
{
  return FilterTraits<ApplyTransformationToGeometryFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ApplyTransformationToGeometryFilter::uuid() const
{
  return FilterTraits<ApplyTransformationToGeometryFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ApplyTransformationToGeometryFilter::humanName() const
{
  return "Apply Transformation to Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> ApplyTransformationToGeometryFilter::defaultTags() const
{
  return {"DREAM3D Review", "Rotation/Transforming"};
}

//------------------------------------------------------------------------------
Parameters ApplyTransformationToGeometryFilter::parameters() const
{
  Parameters params;

  /**
   * Please separate the parameters into groups generally of the following:
   *
   * params.insertSeparator(Parameters::Separator{"Input Parameters"});
   * params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
   * params.insertSeparator(Parameters::Separator{"Required Input Feature Data"});
   * params.insertSeparator(Parameters::Separator{"Created Cell Data"});
   * params.insertSeparator(Parameters::Separator{"Created Cell Feature Data"});
   *
   * .. or create appropriate separators as needed. The UI in COMPLEX no longer
   * does this for the developer by using catgories as in SIMPL
   */

  // Create the parameter descriptors that are needed for this filter

  params.insertSeparator(Parameters::Separator{"Input Parameters"});

  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_TransformationType_Key, "Transformation Type", "The type of transformation to perform.", k_RotationIdx, k_TransformationChoices));

  DynamicTableInfo tableInfo;
  tableInfo.setColsInfo(DynamicTableInfo::StaticVectorInfo({"1", "2", "3", "4"}));
  tableInfo.setRowsInfo(DynamicTableInfo::StaticVectorInfo({"1", "2", "3", "4"}));
  DynamicTableInfo::TableDataType defaultTable{{{1.0F, 0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F, 0.0F}, {0.0F, 0.0F, 0.0F, 1.0F}}};
  params.insert(std::make_unique<DynamicTableParameter>(k_ManualTransformationMatrix_Key, "Transformation Matrix", "", defaultTable, tableInfo));

  params.insert(std::make_unique<VectorFloat32Parameter>(k_Rotation_Key, "Rotation Axis-Angle", "<xyz> w (w in degrees)", std::vector<float32>{0.0F, 0.0F, 1.0F, 90.0F},
                                                         std::vector<std::string>{"x", "y", "z", "w (Deg)"}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Translation_Key, "Translation", "", std::vector<float32>{0.0F, 0.0F, 0.0F}, std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Scale_Key, "Scale", "0>= value < 1: Shrink, value = 1: No transform, value > 1.0 enlarge", std::vector<float32>{1.0F, 1.0F, 1.0F},
                                                         std::vector<std::string>{"X", "Y", "Z"}));

  params.insert(std::make_unique<ArraySelectionParameter>(k_ComputedTransformationMatrix_Key, "Transformation Matrix", "", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::float32}));

  params.insertSeparator({"Input Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometry_Key, "Selected Geometry", "The target geometry on which to perform the transformation", DataPath{},
                                                             IGeometry::GetAllGeomTypes()));

  params.insertSeparator(Parameters::Separator{"Image Geometry Cell Data Parameters"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseDataArraySelection_Key, "Select Data Arrays", "The data arrays to interpolate onto the transformed geometry", false));
  params.insert(std::make_unique<ChoicesParameter>(k_InterpolationType_Key, "Interpolation Type", "", k_NearestNeighborInterpolationIdx, k_InterpolationChoices));

  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellAttributeMatrixPath_Key, "Cell Attribute Matrix", "The path to the Cell level data that should be interpolated", DataPath{}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_DataArraySelection_Key, "Data Array Selection", "The list of arrays to calculate histogram(s) for",
                                                               MultiArraySelectionParameter::ValueType{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray},
                                                               complex::GetAllNumericTypes()));

  // Associate the Linkable Parameter(s) to the children parameters that they control

  params.linkParameters(k_UseDataArraySelection_Key, k_DataArraySelection_Key, true);
  params.linkParameters(k_UseDataArraySelection_Key, k_InterpolationType_Key, true);
  params.linkParameters(k_UseDataArraySelection_Key, k_CellAttributeMatrixPath_Key, true);
  params.linkParameters(k_UseDataArraySelection_Key, k_DataArraySelection_Key, true);

  params.linkParameters(k_TransformationType_Key, k_ComputedTransformationMatrix_Key, k_PrecomputedTransformationMatrixIdx);
  params.linkParameters(k_TransformationType_Key, k_ManualTransformationMatrix_Key, k_ManualTransformationMatrixIdx);
  params.linkParameters(k_TransformationType_Key, k_Rotation_Key, k_RotationIdx);
  params.linkParameters(k_TransformationType_Key, k_Translation_Key, k_TranslationIdx);
  params.linkParameters(k_TransformationType_Key, k_Scale_Key, k_ScaleIdx);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ApplyTransformationToGeometryFilter::clone() const
{
  return std::make_unique<ApplyTransformationToGeometryFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ApplyTransformationToGeometryFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                            const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pTransformationMatrixTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_TransformationType_Key);
  auto pInterpolationTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_InterpolationType_Key);
  auto pUseDataArraySelectionValue = filterArgs.value<bool>(k_UseDataArraySelection_Key);
  auto tableData = filterArgs.value<DynamicTableParameter::ValueType>(k_ManualTransformationMatrix_Key);
  auto pRotationAxisValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Rotation_Key);
  auto pTranslationValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Translation_Key);
  auto pScaleValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Scale_Key);
  auto pComputedTransformationMatrixValue = filterArgs.value<DataPath>(k_ComputedTransformationMatrix_Key);
  auto pCellAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellAttributeMatrixPath_Key);
  auto pDataArraySelectionValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_DataArraySelection_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.
  // None found in this filter based on the filter parameters

  // If this filter makes changes to the DataStructure in the form of
  // creating/deleting/moving/renaming DataGroups, Geometries, DataArrays then you
  // will need to use one of the `*Actions` classes located in complex/Filter/Actions
  // to relay that information to the preflight and execute methods. This is done by
  // creating an instance of the Action class and then storing it in the resultOutputActions variable.
  // This is done through a `push_back()` method combined with a `std::move()`. For the
  // newly initiated to `std::move` once that code is executed what was once inside the Action class
  // instance variable is *no longer there*. The memory has been moved. If you try to access that
  // variable after this line you will probably get a crash or have subtle bugs. To ensure that this
  // does not happen we suggest using braces `{}` to scope each of the action's declaration and store
  // so that the programmer is not tempted to use the action instance past where it should be used.
  // You have to create your own Actions class if there isn't something specific for your filter's needs

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ApplyTransformationToGeometryFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel) const
{

  ApplyTransformationToGeometryInputValues inputValues;

  //  inputValues.TransformationMatrixType = filterArgs.value<ChoicesParameter::ValueType>(k_TransformationMatrixType_Key);
  //  inputValues.InterpolationType = filterArgs.value<ChoicesParameter::ValueType>(k_InterpolationType_Key);
  //  inputValues.UseDataArraySelection = filterArgs.value<bool>(k_UseDataArraySelection_Key);
  //  inputValues.ManualTransformationMatrix = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_ManualTransformationMatrix_Key);
  //  inputValues.RotationAngle = filterArgs.value<float32>(k_RotationAngle_Key);
  //  inputValues.RotationAxis = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RotationAxis_Key);
  //  inputValues.Translation = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Translation_Key);
  //  inputValues.Scale = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Scale_Key);
  //  inputValues.ComputedTransformationMatrix = filterArgs.value<DataPath>(k_ComputedTransformationMatrix_Key);
  //  inputValues.CellAttributeMatrixPath = filterArgs.value<DataPath>(k_CellAttributeMatrixPath_Key);
  //  inputValues.DataArraySelection = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_DataArraySelection_Key);

  return ApplyTransformationToGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
