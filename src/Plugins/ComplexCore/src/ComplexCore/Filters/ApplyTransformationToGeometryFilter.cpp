#include "ApplyTransformationToGeometryFilter.hpp"

#include "complex/Common/Numbers.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include "ComplexCore/Filters/Algorithms/ApplyTransformationToGeometry.hpp"

using namespace complex;

#include <string>

namespace OrientationTransformation
{

/**
 * The Orientation codes are written in such a way that the value of -1 indicates
 * an Active Rotation and +1 indicates a passive rotation.
 *
 * DO NOT UNDER ANY CIRCUMSTANCE CHANGE THESE VARIABLES. THERE WILL BE BAD
 * CONSEQUENCES IF THESE ARE CHANGED. EVERY PIECE OF CODE THAT RELIES ON THESE
 * FUNCTIONS WILL BREAK. IN ADDITION, THE QUATERNION ARITHMETIC WILL NO LONGER
 * BE CONSISTENT WITH ROTATION ARITHMETIC.
 *
 * YOU HAVE BEEN WARNED.
 *
 * Adam  Morawiec's book uses Passive rotations.
 **/
namespace Rotations::Constants
{

#define DREAM3D_PASSIVE_ROTATION

#ifdef DREAM3D_PASSIVE_ROTATION
constexpr float epsijk = 1.0f;
constexpr double epsijkd = 1.0;
#elif DREAM3D_ACTIVE_ROTATION
static const float epsijk = -1.0f;
static const double epsijkd = -1.0;
#endif
} // namespace Rotations::Constants
template <typename InputType, typename OutputType>
OutputType ax2om(const InputType& a)
{
  OutputType res(9);
  using value_type = typename OutputType::value_type;
  value_type q = 0.0L;
  value_type c = 0.0L;
  value_type s = 0.0L;
  value_type omc = 0.0L;

  c = cos(a[3]);
  s = sin(a[3]);

  omc = static_cast<value_type>(1.0 - c);

  res[0] = a[0] * a[0] * omc + c;
  res[4] = a[1] * a[1] * omc + c;
  res[8] = a[2] * a[2] * omc + c;
  int _01 = 1;
  int _10 = 3;
  int _12 = 5;
  int _21 = 7;
  int _02 = 2;
  int _20 = 6;
  // Check to see if we need to transpose
  if(Rotations::Constants::epsijk == 1.0F)
  {
    _01 = 3;
    _10 = 1;
    _12 = 7;
    _21 = 5;
    _02 = 6;
    _20 = 2;
  }

  q = omc * a[0] * a[1];
  res[_01] = q + s * a[2];
  res[_10] = q - s * a[2];
  q = omc * a[1] * a[2];
  res[_12] = q + s * a[0];
  res[_21] = q - s * a[0];
  q = omc * a[2] * a[0];
  res[_02] = q - s * a[1];
  res[_20] = q + s * a[1];

  return res;
}
} // namespace OrientationTransformation

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
  return {"#ComplexCore", "Rotation", "Transforming"};
}

//------------------------------------------------------------------------------
Parameters ApplyTransformationToGeometryFilter::parameters() const
{
  Parameters params;
  params.insert(
      std::make_unique<GeometrySelectionParameter>(k_GeometryToTransform_Key, "Geometry to Transform", "", DataPath{},
                                                   GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Vertex, IGeometry::Type::Edge, IGeometry::Type::Triangle,
                                                                                            IGeometry::Type::Quad, IGeometry::Type::Tetrahedral, IGeometry::Type::Hexahedral}));
  params.insertLinkableParameter(
      std::make_unique<ChoicesParameter>(k_TransformType_Key, "Transformation Type", "", 0,
                                         ChoicesParameter::Choices{"No Transformation", "Pre-Computed Transformation Matrix", "Manual Transformation Matrix", "Rotation", "Translation", "Scale"}));

  params.insert(std::make_unique<ArraySelectionParameter>(k_ComputedTransformationMatrix_Key, "Transformation Matrix", "", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::float32}));

  DynamicTableParameter::ValueType dynamicTable{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, {"R0", "R1", "R2", "R3"}, {"C0", "C1", "C2", "C3"}};
  dynamicTable.setMinCols(4);
  dynamicTable.setMinRows(4);
  dynamicTable.setDynamicCols(false);
  dynamicTable.setDynamicRows(false);
  params.insert(std::make_unique<DynamicTableParameter>(k_ManualTransformationMatrix_Key, "DynamicTableParameter", "DynamicTableParameter Example Help Text", dynamicTable));

  params.insert(std::make_unique<VectorFloat32Parameter>(k_RotationAxisAngle_Key, "Rotation Axis-Angle (Degrees)", "", std::vector<float>{0.0F, 0.0F, 1.0F, 90.0F},
                                                         std::vector<std::string>{"X", "Y", "Z", "Angle"}));

  params.insert(std::make_unique<VectorFloat32Parameter>(k_Translation_Key, "Translation", "", std::vector<float>{0.0F, 0.0F, 0.0F}, std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(
      std::make_unique<VectorFloat32Parameter>(k_Scale_Key, "Scale Factor", "2 = 2x Size. 0.5 is half the size", std::vector<float>{1.0F, 1.0F, 1.0F}, std::vector<std::string>{"X", "Y", "Z"}));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_TransformType_Key, k_ComputedTransformationMatrix_Key, std::make_any<ChoicesParameter::ValueType>(1));
  params.linkParameters(k_TransformType_Key, k_ManualTransformationMatrix_Key, std::make_any<ChoicesParameter::ValueType>(2));
  params.linkParameters(k_TransformType_Key, k_RotationAxisAngle_Key, std::make_any<ChoicesParameter::ValueType>(3));
  params.linkParameters(k_TransformType_Key, k_Translation_Key, std::make_any<ChoicesParameter::ValueType>(4));
  params.linkParameters(k_TransformType_Key, k_Scale_Key, std::make_any<ChoicesParameter::ValueType>(5));

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
  auto pGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(k_GeometryToTransform_Key);
  auto pTransformationType = static_cast<TransformType>(filterArgs.value<ChoicesParameter::ValueType>(k_TransformType_Key));
  auto pManualTransformationMatrixValue = filterArgs.value<DynamicTableParameter::ValueType>(k_ManualTransformationMatrix_Key);
  auto pRotationAxisAngleValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RotationAxisAngle_Key);
  auto pTranslationValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Translation_Key);
  auto pScaleValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Scale_Key);
  auto pGeometryToTransformValue = filterArgs.value<DataPath>(k_GeometryToTransform_Key);
  auto pComputedTransformMatrixDataPath = filterArgs.value<DataPath>(k_ComputedTransformationMatrix_Key);

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

  switch(pTransformationType)
  {
  case TransformType::No_Transform: {
    resultOutputActions.warnings().push_back(Warning{-701, "No transformation has been selected, so this filter will perform no operations"});
    break;
  }
  case TransformType::PreComputed_TransformMatrix: {
    // Verify that the selected precomputed array is of type float
    const auto* dataObject = dataStructure.getDataAs<Float32Array>(pComputedTransformMatrixDataPath);
    if(dataObject == nullptr)
    {
      return {MakeErrorResult<OutputActions>(-702, "Precomputed Transform Matrix was not of type Float 32")};
    }
    if(dataObject->getNumberOfTuples() != 16ULL)
    {
      return {MakeErrorResult<OutputActions>(
          -702, "Precomputed Transform Matrix did not contain exactly 16 values. The precomputed Transform Matrix is a 4x4 where the values are stored in ROW Major format")};
    }
    break;
  }
  case TransformType::ManualTransformMatrix: {
    if(pManualTransformationMatrixValue.getNumberOfColumns() != 4 || pManualTransformationMatrixValue.getNumberOfRows() != 4)
    {
      return {MakeErrorResult<OutputActions>(-702, "Manually specified Transform Matrix is not 4x4. The Transform Matrix must be a 4x4 where the values are stored in ROW Major format")};
    }
    break;
  }
  case TransformType::Rotation:
  case TransformType::Translation:
  case TransformType::Scale:
    break;
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ApplyTransformationToGeometryFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/
  complex::ApplyTransformationToGeometryInputValues inputValues;

  inputValues.pGeometryToTransform = filterArgs.value<GeometrySelectionParameter::ValueType>(k_GeometryToTransform_Key);
  inputValues.pTransformationType = static_cast<TransformType>(filterArgs.value<ChoicesParameter::ValueType>(k_TransformType_Key));
  auto pComputedTransformationMatrixPath = filterArgs.value<DataPath>(k_ComputedTransformationMatrix_Key);

  std::vector<float> m_TransformationMatrix(16, 0.0F);
  switch(inputValues.pTransformationType)
  {
  case TransformType::No_Transform: {
    complex::Result<> resultActions;
    resultActions.warnings().push_back(Warning{-709, "Transform Type was set to '0' which means no transform will be performed."});
    return resultActions;
    break;
  }
  case TransformType::PreComputed_TransformMatrix: {
    auto precomputedTransformMatrix = dataStructure.getDataRefAs<Float32Array>(pComputedTransformationMatrixPath);
    std::copy(precomputedTransformMatrix.begin(), precomputedTransformMatrix.end(), m_TransformationMatrix.begin());
    break;
  }
  case TransformType::ManualTransformMatrix: {
    auto flattenedData = filterArgs.value<DynamicTableParameter::ValueType>(k_ManualTransformationMatrix_Key).getFlattenedData();
    m_TransformationMatrix = std::vector<float>(flattenedData.begin(), flattenedData.end());
    break;
  }
  case TransformType::Rotation: {
    auto pRotationAxisAngleValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RotationAxisAngle_Key);
    // Convert Degrees to Radians for the last element
    pRotationAxisAngleValue[3] = pRotationAxisAngleValue[3] * static_cast<float>(complex::numbers::pi / 180.0f);
    using OrientationF = std::vector<float>;
    OrientationF om = OrientationTransformation::ax2om<OrientationF, OrientationF>(OrientationF(pRotationAxisAngleValue));

    for(size_t i = 0; i < 3; i++)
    {
      m_TransformationMatrix[4 * i + 0] = om[3 * i + 0];
      m_TransformationMatrix[4 * i + 1] = om[3 * i + 1];
      m_TransformationMatrix[4 * i + 2] = om[3 * i + 2];
      m_TransformationMatrix[4 * i + 3] = 0.0f;
    }
    m_TransformationMatrix[4 * 3 + 3] = 1.0f;
    break;
  }
  case TransformType::Translation: {
    auto pTranslationValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Translation_Key);
    m_TransformationMatrix[4 * 0 + 0] = 1.0f;
    m_TransformationMatrix[4 * 1 + 1] = 1.0f;
    m_TransformationMatrix[4 * 2 + 2] = 1.0f;
    m_TransformationMatrix[4 * 0 + 3] = pTranslationValue[0];
    m_TransformationMatrix[4 * 1 + 3] = pTranslationValue[1];
    m_TransformationMatrix[4 * 2 + 3] = pTranslationValue[2];
    m_TransformationMatrix[4 * 3 + 3] = 1.0f;
    break;
  }
  case TransformType::Scale: {
    auto pScaleValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Scale_Key);
    m_TransformationMatrix[4 * 0 + 0] = pScaleValue[0];
    m_TransformationMatrix[4 * 1 + 1] = pScaleValue[1];
    m_TransformationMatrix[4 * 2 + 2] = pScaleValue[2];
    m_TransformationMatrix[4 * 3 + 3] = 1.0f;
    break;
  }
  break;
  default:
    return {MakeErrorResult(-705, "Value of 'Transform Type' was not correct. The value should fall between 0 and 5.")};
  }

  inputValues.transformationMatrix = m_TransformationMatrix;

  // Let the Algorithm instance do the work
  return ApplyTransformationToGeometry(dataStructure, &inputValues, shouldCancel, messageHandler)();
}
} // namespace complex
