#include "ApplyTransformationToGeometryFilter.hpp"

#include "complex/Common/Numbers.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateImageGeometryAction.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <Eigen/Dense>

#include "ComplexCore/Filters/Algorithms/ApplyTransformationToImageGeometry.hpp"
#include "ComplexCore/Filters/Algorithms/ApplyTransformationToNodeGeometry.hpp"

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
using RotationRepresentation = ApplyTransformationToGeometryFilter::RotationRepresentation;
using Matrix3fR = Eigen::Matrix<float32, 3, 3, Eigen::RowMajor>;
using Matrix4fR = Eigen::Matrix<float, 4, 4, Eigen::RowMajor>;

constexpr float32 k_Threshold = 0.01f;

const Eigen::Vector3f k_XAxis = Eigen::Vector3f::UnitX();
const Eigen::Vector3f k_YAxis = Eigen::Vector3f::UnitY();
const Eigen::Vector3f k_ZAxis = Eigen::Vector3f::UnitZ();

// struct RotateArgs
//{
//  int64 xp = 0;
//  int64 yp = 0;
//  int64 zp = 0;
//  float32 xRes = 0.0f;
//  float32 yRes = 0.0f;
//  float32 zRes = 0.0f;
//  int64 xpNew = 0;
//  int64 ypNew = 0;
//  int64 zpNew = 0;
//  float32 xResNew = 0.0f;
//  float32 yResNew = 0.0f;
//  float32 zResNew = 0.0f;
//  float32 xMinNew = 0.0f;
//  float32 yMinNew = 0.0f;
//  float32 zMinNew = 0.0f;
//};

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
  return {"#DREAM3D Review", "Rotation", "Transforming"};
}

//------------------------------------------------------------------------------
void updateGeometry(const complex::ImageGeom& imageGeom, const RotateArgs& params, const Matrix3fR& scalingMatrix, const Matrix3fR& rotationMatrix, const double translationMatrix[3],
                    std::vector<size_t>& dims, std::vector<float32>& spacing, std::vector<float32>& origin)
{
  float m_ScalingMatrix[3][3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
  Eigen::Map<Matrix3fR>(&m_ScalingMatrix[0][0], scalingMatrix.rows(), scalingMatrix.cols()) = scalingMatrix;

  Eigen::Vector3f original_origin(origin[0], origin[1], origin[2]);
  Eigen::Vector3f original_origin_rot = rotationMatrix * original_origin;
  float32 spacingArray[3] = {params.xResNew * m_ScalingMatrix[0][0], params.yResNew * m_ScalingMatrix[1][1], params.zResNew * m_ScalingMatrix[2][2]};
  int64_t dimArray[3] = {params.xpNew, params.ypNew, params.zpNew};
  
  for(int i = 0; i < 3; i++)
  {
    spacing[i] = spacingArray[i];
    dims[i] = dimArray[i];
  }

  // Applies Scaling to Image

  // Applies Translation to Image
  origin[0] = params.xMinNew * m_ScalingMatrix[0][0] + translationMatrix[0] + original_origin_rot[0] * params.xResNew * m_ScalingMatrix[0][0] / params.xRes;
  origin[1] = params.yMinNew * m_ScalingMatrix[1][1] + translationMatrix[1] + original_origin_rot[1] * params.yResNew * m_ScalingMatrix[1][1] / params.yRes;
  origin[2] = params.zMinNew * m_ScalingMatrix[2][2] + translationMatrix[2] + original_origin_rot[2] * params.zResNew * m_ScalingMatrix[2][2] / params.zRes;
}

//------------------------------------------------------------------------------
void DetermineMinMax(const Matrix3fR& rotationMatrix, const FloatVec3& spacing, usize col, usize row, usize plane, float32& xMin, float32& xMax, float32& yMin, float32& yMax, float32& zMin,
                     float32& zMax)
{
  Eigen::Vector3f coords(static_cast<float32>(col) * spacing[0], static_cast<float32>(row) * spacing[1], static_cast<float32>(plane) * spacing[2]);

  Eigen::Vector3f newCoords = rotationMatrix * coords;

  xMin = std::min(newCoords[0], xMin);
  xMax = std::max(newCoords[0], xMax);

  yMin = std::min(newCoords[1], yMin);
  yMax = std::max(newCoords[1], yMax);

  zMin = std::min(newCoords[2], zMin);
  zMax = std::max(newCoords[2], zMax);
}

//------------------------------------------------------------------------------
float32 CosBetweenVectors(const Eigen::Vector3f& a, const Eigen::Vector3f& b)
{
  float32 normA = a.norm();
  float32 normB = b.norm();

  if(normA == 0.0f || normB == 0.0f)
  {
    return 1.0f;
  }

  return a.dot(b) / (normA * normB);
}

//------------------------------------------------------------------------------
float32 DetermineSpacing(const FloatVec3& spacing, const Eigen::Vector3f& axisNew)
{
  float32 xAngle = std::abs(CosBetweenVectors(k_XAxis, axisNew));
  float32 yAngle = std::abs(CosBetweenVectors(k_YAxis, axisNew));
  float32 zAngle = std::abs(CosBetweenVectors(k_ZAxis, axisNew));

  std::array<float32, 3> axes = {xAngle, yAngle, zAngle};

  auto iter = std::max_element(axes.cbegin(), axes.cend());

  usize index = std::distance(axes.cbegin(), iter);

  return spacing[index];
}

//------------------------------------------------------------------------------
RotateArgs CreateRotateArgs(const ImageGeom& imageGeom, const Matrix3fR& rotationMatrix)
{
  const SizeVec3 origDims = imageGeom.getDimensions();
  const FloatVec3 spacing = imageGeom.getSpacing();

  float32 xMin = std::numeric_limits<float32>::max();
  float32 xMax = std::numeric_limits<float32>::min();
  float32 yMin = std::numeric_limits<float32>::max();
  float32 yMax = std::numeric_limits<float32>::min();
  float32 zMin = std::numeric_limits<float32>::max();
  float32 zMax = std::numeric_limits<float32>::min();

  const std::vector<std::array<usize, 3>> coords{{0, 0, 0},
                                                 {origDims[0] - 1, 0, 0},
                                                 {0, origDims[1] - 1, 0},
                                                 {origDims[0] - 1, origDims[1] - 1, 0},
                                                 {0, 0, origDims[2] - 1},
                                                 {origDims[0] - 1, 0, origDims[2] - 1},
                                                 {0, origDims[1] - 1, origDims[2] - 1},
                                                 {origDims[0] - 1, origDims[1] - 1, origDims[2] - 1}};

  for(const auto& item : coords)
  {
    DetermineMinMax(rotationMatrix, spacing, item[0], item[1], item[2], xMin, xMax, yMin, yMax, zMin, zMax);
  }

  Eigen::Vector3f xAxisNew = rotationMatrix * k_XAxis;
  Eigen::Vector3f yAxisNew = rotationMatrix * k_YAxis;
  Eigen::Vector3f zAxisNew = rotationMatrix * k_ZAxis;

  float32 xResNew = DetermineSpacing(spacing, xAxisNew);
  float32 yResNew = DetermineSpacing(spacing, yAxisNew);
  float32 zResNew = DetermineSpacing(spacing, zAxisNew);

  ImageGeom::MeshIndexType xpNew = static_cast<int64>(std::nearbyint((xMax - xMin) / xResNew) + 1);
  ImageGeom::MeshIndexType ypNew = static_cast<int64>(std::nearbyint((yMax - yMin) / yResNew) + 1);
  ImageGeom::MeshIndexType zpNew = static_cast<int64>(std::nearbyint((zMax - zMin) / zResNew) + 1);

  complex::RotateArgs params;

  params.xp = origDims[0];
  params.xRes = spacing[0];
  params.yp = origDims[1];
  params.yRes = spacing[1];
  params.zp = origDims[2];
  params.zRes = spacing[2];

  params.xpNew = xpNew;
  params.xResNew = xResNew;
  params.xMinNew = xMin;
  params.ypNew = ypNew;
  params.yResNew = yResNew;
  params.yMinNew = yMin;
  params.zpNew = zpNew;
  params.zResNew = zResNew;
  params.zMinNew = zMin;

  return params;
}

//------------------------------------------------------------------------------
Parameters ApplyTransformationToGeometryFilter::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<GeometrySelectionParameter>(k_GeometryToTransform_Key, "Geometry to Transform", "", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{AbstractGeometry::Type::Vertex, AbstractGeometry::Type::Edge, AbstractGeometry::Type::Triangle,
                                                                                                      AbstractGeometry::Type::Quad, AbstractGeometry::Type::Tetrahedral,
                                                                                                      AbstractGeometry::Type::Hexahedral, AbstractGeometry::Type::Image}));
  params.insertLinkableParameter(
      std::make_unique<ChoicesParameter>(k_TransformType_Key, "Transformation Type", "", 0,
                                         ChoicesParameter::Choices{"No Transformation", "Pre-Computed Transformation Matrix", "Manual Transformation Matrix", "Rotation", "Translation", "Scale"}));

  params.insert(std::make_unique<ArraySelectionParameter>(k_ComputedTransformationMatrix_Key, "Transformation Matrix", "", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::float32}, true));

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

  params.insertSeparator({"Image Geometry Options"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseArraySelector_Key, "Use Array Selector", "", true));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_SelectedCellArrays_Key, "Cell Arrays", "", MultiArraySelectionParameter::ValueType{}, GetAllDataTypes()));
  params.linkParameters(k_UseArraySelector_Key, k_SelectedCellArrays_Key, true);
  params.insert(std::make_unique<ChoicesParameter>(k_InterpolationType_Key, "Interpolation Type", "", 0, ChoicesParameter::Choices{"Nearest Neighbor", "Linear Interpolation"}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedImageGeometry_Key, "Created Geometry", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ApplyTransformationToGeometryFilter::clone() const
{
  return std::make_unique<ApplyTransformationToGeometryFilter>();
}

//------------------------------------------------------------------------------
Result<OutputActions> transformationMatrixCheck(DataStructure dataStructure, float* transformationMatrix, TransformType transformType, DataPath computedMatrixDataPath,
                                                DynamicTableParameter::ValueType manualTranformationMatrix, VectorFloat32Parameter::ValueType rotationAxisAngle,
                                                VectorFloat32Parameter::ValueType translation, VectorFloat32Parameter::ValueType scale)
{
  switch(transformType)
  {
  case TransformType::No_Transform: {
    complex::Result<OutputActions> resultOutputActions;
    resultOutputActions.warnings().push_back(Warning{-701, "No transformation has been selected, so this filter will perform no operations"});
    return resultOutputActions;
  }
  case TransformType::PreComputed_TransformMatrix: {
    // Verify that the selected precomputed array is of type float
    const auto* dataObject = dataStructure.getDataAs<Float32Array>(computedMatrixDataPath);
    if(dataObject == nullptr)
    {
      return {MakeErrorResult<OutputActions>(-702, "Precomputed Transform Matrix was not of type Float 32")};
    }
    if(dataObject->getNumberOfTuples() != 16ULL)
    {
      return {MakeErrorResult<OutputActions>(
          -702, "Precomputed Transform Matrix did not contain exactly 16 values. The precomputed Transform Matrix is a 4x4 where the values are stored in ROW Major format")};
    }
    auto precomputedTransformMatrix = dataStructure.getDataRefAs<Float32Array>(computedMatrixDataPath);
    std::copy(precomputedTransformMatrix.begin(), precomputedTransformMatrix.end(), transformationMatrix);
    break;
  }
  case TransformType::ManualTransformMatrix: {
    if(manualTranformationMatrix.getNumberOfColumns() != 4 || manualTranformationMatrix.getNumberOfRows() != 4)
    {
      return {MakeErrorResult<OutputActions>(-702, "Manually specified Transform Matrix is not 4x4. The Transform Matrix must be a 4x4 where the values are stored in ROW Major format")};
    }
    auto flattenedData = manualTranformationMatrix.getFlattenedData();
    std::copy(flattenedData.begin(), flattenedData.end(), transformationMatrix);
    break;
  }
  case TransformType::Rotation: {
    auto pRotationAxisAngleValue = rotationAxisAngle;
    // Convert Degrees to Radians for the last element
    pRotationAxisAngleValue[3] = pRotationAxisAngleValue[3] * static_cast<float>(complex::numbers::pi / 180.0f);
    using OrientationF = std::vector<float>;
    OrientationF om = OrientationTransformation::ax2om<OrientationF, OrientationF>(OrientationF(pRotationAxisAngleValue));

    for(size_t i = 0; i < 3; i++)
    {
      transformationMatrix[4 * i + 0] = om[3 * i + 0];
      transformationMatrix[4 * i + 1] = om[3 * i + 1];
      transformationMatrix[4 * i + 2] = om[3 * i + 2];
      transformationMatrix[4 * i + 3] = 0.0f;
    }
    transformationMatrix[4 * 3 + 3] = 1.0f;
  }
  case TransformType::Translation: {
    auto pTranslationValue = translation;
    transformationMatrix[4 * 0 + 0] = 1.0f;
    transformationMatrix[4 * 1 + 1] = 1.0f;
    transformationMatrix[4 * 2 + 2] = 1.0f;
    transformationMatrix[4 * 0 + 3] = pTranslationValue[0];
    transformationMatrix[4 * 1 + 3] = pTranslationValue[1];
    transformationMatrix[4 * 2 + 3] = pTranslationValue[2];
    transformationMatrix[4 * 3 + 3] = 1.0f;
  }
  case TransformType::Scale:
    auto pScaleValue = scale;
    transformationMatrix[4 * 0 + 0] = pScaleValue[0];
    transformationMatrix[4 * 1 + 1] = pScaleValue[1];
    transformationMatrix[4 * 2 + 2] = pScaleValue[2];
    transformationMatrix[4 * 3 + 3] = 1.0f;
    break;
  }
  return {};
}

//------------------------------------------------------------------------------
// As of 4/27/2022 Deferred deletion is not possible so arrays will internally double
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
  auto pCreatedGeomtry = filterArgs.value<DataPath>(k_CreatedImageGeometry_Key);
  auto pInterpolationType = filterArgs.value<ChoicesParameter::ValueType>(k_InterpolationType_Key);
  auto pUseArraySelector = filterArgs.value<BoolParameter::ValueType>(k_UseArraySelector_Key);
  auto pSelectedArrays = filterArgs.value<std::vector<DataPath>>(k_SelectedCellArrays_Key);

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

  float m_TransformationMatrix[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  transformationMatrixCheck(dataStructure, m_TransformationMatrix, pTransformationType, pComputedTransformMatrixDataPath, pManualTransformationMatrixValue, pRotationAxisAngleValue, pTranslationValue,
                            pScaleValue);

  const DataObject* dataObject = dataStructure.getData(pGeometryToTransformValue);

  //if(!pUseArraySelector)
  //{
  //  pSelectedArrays = dataStructure.getAllDataPaths();
  //}

  if(dataObject->getDataObjectType() == DataObject::Type::ImageGeom)
  {
    if(pInterpolationType == 1)
    {
      for(const auto& cellArrayPath : pSelectedArrays)
      {
        const auto& cellArray = dataStructure.getDataRefAs<IDataArray>(cellArrayPath);
        if(cellArray.getDataType() == complex::DataType::boolean)
        {
          return {MakeErrorResult<OutputActions>(-704, "Input Type Error, cannot run linear interpolation on a boolean array")};
        }
      }
    }

    auto selectedImageGeomPath = filterArgs.value<DataPath>(k_GeometryToTransform_Key);
    const auto& selectedImageGeom = dataStructure.getDataRefAs<ImageGeom>(selectedImageGeomPath);
    Eigen::Map<Matrix4fR> transformation(m_TransformationMatrix);
    //Matrix4fR transformation = Matrix4fR::Zero();
 //   for(int i = 0; i < 4; i++)
 //   {
	//  for (int j = 0; j < 4; j++)
	//  {
 //       transformation(i, j) = m_TransformationMatrix[4 * i + j];
	//  }
	//}
	//Eigen::Map<Matrix4fR>(m_TransformationMatrix);
    //Eigen::Map<Matrix4fR> transformation2 = transformation;
    //float* tData = transformation2.data();
    Eigen::Transform<float, 3, Eigen::Affine> transform = Eigen::Transform<float, 3, Eigen::Affine>(transformation);

    Matrix3fR rotationMatrix = Matrix3fR::Zero();
    Matrix3fR scaleMatrix = Matrix3fR::Zero();
    double translationMatrix[3] = {0, 0, 0};

    transform.computeRotationScaling(&rotationMatrix, &scaleMatrix);
    translationMatrix[0] = transform.data()[12];
    translationMatrix[1] = transform.data()[13];
    translationMatrix[2] = transform.data()[14];

    // TODO: READ THROUGH AND COMPARE
    RotateArgs rotateArgs = CreateRotateArgs(selectedImageGeom, rotationMatrix);
    std::vector<size_t> dims = {static_cast<usize>(rotateArgs.xpNew), static_cast<usize>(rotateArgs.ypNew), static_cast<usize>(rotateArgs.zpNew)};
    std::vector<float32> spacing = {rotateArgs.xResNew, rotateArgs.yResNew, rotateArgs.zResNew};
    std::vector<float32> origin = selectedImageGeom.getOrigin().toContainer<std::vector<float32>>();

    updateGeometry(selectedImageGeom, rotateArgs, scaleMatrix, rotationMatrix, translationMatrix, dims, spacing, origin);

    DataPath createdImageGeomPath = filterArgs.value<DataPath>(k_CreatedImageGeometry_Key);

    resultOutputActions.value().actions.push_back(std::make_unique<CreateImageGeometryAction>(createdImageGeomPath, dims, origin, spacing));

    std::vector<usize> cellArrayDims(dims.crbegin(), dims.crend());

    SizeVec3 originalImageDims = selectedImageGeom.getDimensions();
    usize originalImageSize = std::accumulate(originalImageDims.begin(), originalImageDims.end(), static_cast<usize>(1), std::multiplies<>{});

    for(const auto& cellArrayPath : pSelectedArrays)
    {
      const auto& cellArray = dataStructure.getDataRefAs<IDataArray>(cellArrayPath);
      usize arraySize = cellArray.getNumberOfTuples();
      if(arraySize != originalImageSize)
      {
        return {MakeErrorResult<OutputActions>(
            -1, fmt::format("Selected Array '{}' was size {}, but Image Geometry '{}' expects size {}", cellArray.getName(), arraySize, selectedImageGeom.getName(), originalImageSize))};
      }
      DataPath createdArrayPath = createdImageGeomPath.createChildPath(cellArray.getName());
      resultOutputActions.value().actions.push_back(std::make_unique<CreateArrayAction>(cellArray.getDataType(), cellArrayDims, cellArray.getIDataStoreRef().getComponentShape(), createdArrayPath));
    }
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
  auto pGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(k_GeometryToTransform_Key);
  auto pTransformationType = static_cast<TransformType>(filterArgs.value<ChoicesParameter::ValueType>(k_TransformType_Key));
  auto pManualTransformationMatrixValue = filterArgs.value<DynamicTableParameter::ValueType>(k_ManualTransformationMatrix_Key);
  auto pRotationAxisAngleValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RotationAxisAngle_Key);
  auto pTranslationValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Translation_Key);
  auto pScaleValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Scale_Key);
  auto pGeometryToTransformValue = filterArgs.value<DataPath>(k_GeometryToTransform_Key);
  auto pComputedTransformMatrixDataPath = filterArgs.value<DataPath>(k_ComputedTransformationMatrix_Key);
  auto pCreatedGeomtry = filterArgs.value<DataPath>(k_CreatedImageGeometry_Key);
  auto pInterpolationType = filterArgs.value<ChoicesParameter::ValueType>(k_InterpolationType_Key);
  auto pUseArraySelector = filterArgs.value<BoolParameter::ValueType>(k_UseArraySelector_Key);
  auto pSelectedArrays = filterArgs.value<std::vector<DataPath>>(k_SelectedCellArrays_Key);

  float m_TransformationMatrix[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  transformationMatrixCheck(dataStructure, m_TransformationMatrix, pTransformationType, pComputedTransformMatrixDataPath, pManualTransformationMatrixValue, pRotationAxisAngleValue, pTranslationValue,
                            pScaleValue);
  RotateArgs rotateArgs;

  complex::ApplyTransformationToNodeGeometryInputValues inputNodeValues;
  complex::ApplyTransformationToImageGeometryInputValues inputImageValues;

  inputNodeValues.pGeometryToTransform = pGeometryPath;
  inputNodeValues.pTransformationType = pTransformationType;

  inputImageValues.pGeometryToTransform = pGeometryPath;
  inputImageValues.pTransformationType = pTransformationType;
  inputImageValues.pInterpolationType = pInterpolationType;

  float transformMatrix[16];
  for(int i=0; i < 16; i++)
  {
    transformMatrix[i] = m_TransformationMatrix[i];
  }
  
  inputNodeValues.transformationMatrix.insert(inputNodeValues.transformationMatrix.begin(), std::begin(transformMatrix), std::end(transformMatrix));
  inputImageValues.pTransformationMatrix.insert(inputImageValues.pTransformationMatrix.begin(), std::begin(transformMatrix), std::end(transformMatrix));

  DataObject* dataObject = dataStructure.getData(inputImageValues.pGeometryToTransform);

  // Let the Algorithm instance do the work
  if(dataObject->getDataObjectType() == DataObject::Type::ImageGeom)
  {
    const auto& selectedImageGeom = dataStructure.getDataRefAs<ImageGeom>(pCreatedGeomtry);
    Matrix3fR rotationMatrix = Matrix3fR::Zero();
    Matrix3fR scaleMatrix = Matrix3fR::Zero();
    Eigen::Map<Matrix4fR> transformation(m_TransformationMatrix);
    Eigen::Transform<float, 3, Eigen::Affine> transform = Eigen::Transform<float, 3, Eigen::Affine>(transformation);
    transform.computeRotationScaling(&rotationMatrix, &scaleMatrix);
    rotateArgs = CreateRotateArgs(selectedImageGeom, rotationMatrix);
    inputImageValues.pRotateArgs = rotateArgs;
    inputImageValues.pUseArraySelector = pUseArraySelector;
    inputImageValues.pSelectedArrays = pSelectedArrays;
    inputImageValues.pCreatedImageGeometry = pCreatedGeomtry;
    return ApplyTransformationToImageGeometry(dataStructure, &inputImageValues, shouldCancel, messageHandler)();
  }
  return ApplyTransformationToNodeGeometry(dataStructure, &inputNodeValues, shouldCancel, messageHandler)();
}
} // namespace complex
