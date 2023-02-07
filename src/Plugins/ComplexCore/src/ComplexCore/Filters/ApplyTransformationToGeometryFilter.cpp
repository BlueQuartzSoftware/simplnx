#include "ApplyTransformationToGeometryFilter.hpp"

#include "ComplexCore/Filters/Algorithms/ApplyTransformationToGeometry.hpp"

#include "complex/Common/Numbers.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Utilities/Math/MatrixMath.hpp"

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
  return {"ComplexCore", "Rotation", "Transforming"};
}

//------------------------------------------------------------------------------
Parameters ApplyTransformationToGeometryFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_GeometryToTransform_Key, "Geometry to Transform", "The complete path to the geometry to transform", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Vertex, IGeometry::Type::Edge, IGeometry::Type::Triangle, IGeometry::Type::Quad,
                                                                                                      IGeometry::Type::Tetrahedral, IGeometry::Type::Hexahedral}));
  params.insertLinkableParameter(
      std::make_unique<ChoicesParameter>(k_TransformType_Key, "Transformation Type", "Type of transformation to be used", 0,
                                         ChoicesParameter::Choices{"No Transformation", "Pre-Computed Transformation Matrix", "Manual Transformation Matrix", "Rotation", "Translation", "Scale"}));

  params.insert(std::make_unique<ArraySelectionParameter>(k_ComputedTransformationMatrix_Key, "Pre-Computed Transformation Matrix",
                                                          "A 4x4 transformation matrix, supplied by an Attribute Array in row major order", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}));

  DynamicTableInfo tableInfo;
  tableInfo.setColsInfo(DynamicTableInfo::StaticVectorInfo{{"C0", "C1", "C2", "C3"}});
  tableInfo.setRowsInfo(DynamicTableInfo::StaticVectorInfo{{"R0", "R1", "R2", "R3"}});

  params.insert(std::make_unique<DynamicTableParameter>(k_ManualTransformationMatrix_Key, "Manual Transformation Matrix", "Manually entered 4x4 transformation matrix", tableInfo));

  params.insert(std::make_unique<VectorFloat32Parameter>(k_RotationAxisAngle_Key, "Rotation Axis-Angle (Degrees)", "Rotation about the supplied axis-angle",
                                                         std::vector<float>{0.0F, 0.0F, 1.0F, 90.0F}, std::vector<std::string>{"X", "Y", "Z", "Angle"}));

  params.insert(
      std::make_unique<VectorFloat32Parameter>(k_Translation_Key, "Translation", "x, y, z translation values", std::vector<float>{0.0F, 0.0F, 0.0F}, std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Scale_Key, "Scale Factor", "x, y, z scale values (2 = 2x Size. 0.5 is half the size)", std::vector<float>{1.0F, 1.0F, 1.0F},
                                                         std::vector<std::string>{"X", "Y", "Z"}));

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

  PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

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
  case TransformType::Rotation:
  case TransformType::Translation:
  case TransformType::Scale:
  case TransformType::ManualTransformMatrix:
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
    auto tableData = filterArgs.value<DynamicTableParameter::ValueType>(k_ManualTransformationMatrix_Key);
    auto flattenedData = DynamicTableInfo::FlattenData(tableData);
    for(usize i = 0; i < m_TransformationMatrix.size(); i++)
    {
      m_TransformationMatrix[i] = static_cast<float32>(flattenedData[i]);
    }
    break;
  }
  case TransformType::Rotation: {
    auto aa = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RotationAxisAngle_Key);

    // Ensure the axis part is normalized
    MatrixMath::Normalize3x1(aa.data());
    // Convert Degrees to Radians for the last element
    aa[3] = aa[3] * static_cast<float>(complex::numbers::pi / 180.0f);

    float cosTheta = cos(aa[3]);
    float oneMinusCosTheta = 1 - cosTheta;
    float sinTheta = sin(aa[3]);
    float l = aa[0];
    float m = aa[1];
    float n = aa[2];

    // First Row:
    m_TransformationMatrix[0] = l * l * (oneMinusCosTheta) + cosTheta;
    m_TransformationMatrix[1] = m * l * (oneMinusCosTheta)-n * sinTheta;
    m_TransformationMatrix[2] = n * l * (oneMinusCosTheta) + m * sinTheta;
    m_TransformationMatrix[3] = 0.0F;

    // Second Row:
    m_TransformationMatrix[4] = l * m * (oneMinusCosTheta) + n * sinTheta;
    m_TransformationMatrix[5] = m * m * (oneMinusCosTheta) + cosTheta;
    m_TransformationMatrix[6] = n * m * (oneMinusCosTheta)-l * sinTheta;
    m_TransformationMatrix[7] = 0.0F;

    // Third Row:
    m_TransformationMatrix[8] = l * n * (oneMinusCosTheta)-m * sinTheta;
    m_TransformationMatrix[9] = m * n * (oneMinusCosTheta) + l * sinTheta;
    m_TransformationMatrix[10] = n * n * (oneMinusCosTheta) + cosTheta;
    m_TransformationMatrix[11] = 0.0F;

    // Fourth Row:
    m_TransformationMatrix[12] = 0.0F;
    m_TransformationMatrix[13] = 0.0F;
    m_TransformationMatrix[14] = 0.0F;
    m_TransformationMatrix[15] = 1.0F;

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
