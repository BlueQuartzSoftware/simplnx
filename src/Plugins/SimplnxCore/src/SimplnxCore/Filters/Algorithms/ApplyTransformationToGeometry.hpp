#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/ImageRotationUtilities.hpp"

#include <Eigen/Dense>

namespace nx::core
{
namespace detail
{
static inline constexpr StringLiteral k_NoTransform = "No Transform";
static inline constexpr StringLiteral k_PrecomputedTransformationMatrix = "Pre-Computed Transformation Matrix (4x4)";
static inline constexpr StringLiteral k_ManualTransformationMatrix = "Manual Transformation Matrix";
static inline constexpr StringLiteral k_Rotation = "Rotation";
static inline constexpr StringLiteral k_Translation = "Translation";
static inline constexpr StringLiteral k_Scale = "Scale";
static inline const ChoicesParameter::Choices k_TransformationChoices = {k_NoTransform, k_PrecomputedTransformationMatrix, k_ManualTransformationMatrix, k_Rotation, k_Translation, k_Scale};

static inline constexpr ChoicesParameter::ValueType k_NoTransformIdx = 0ULL;
static inline constexpr ChoicesParameter::ValueType k_PrecomputedTransformationMatrixIdx = 1ULL;
static inline constexpr ChoicesParameter::ValueType k_ManualTransformationMatrixIdx = 2ULL;
static inline constexpr ChoicesParameter::ValueType k_RotationIdx = 3ULL;
static inline constexpr ChoicesParameter::ValueType k_TranslationIdx = 4ULL;
static inline constexpr ChoicesParameter::ValueType k_ScaleIdx = 5ULL;

static inline constexpr StringLiteral k_NearestNeighborInterpolation = "Nearest Neighbor Resampling";
static inline constexpr StringLiteral k_LinearInterpolation = "Linear Interpolation";
static inline constexpr StringLiteral k_NoInterpolation = "No Interpolation";
static inline const ChoicesParameter::Choices k_InterpolationChoices = {k_NearestNeighborInterpolation, k_LinearInterpolation, k_NoInterpolation};

static inline constexpr ChoicesParameter::ValueType k_NearestNeighborInterpolationIdx = 0ULL;
static inline constexpr ChoicesParameter::ValueType k_LinearInterpolationIdx = 1ULL;
static inline constexpr ChoicesParameter::ValueType k_NoInterpolationIdx = 2ULL;
} // namespace detail

struct SIMPLNXCORE_EXPORT ApplyTransformationToGeometryInputValues
{
  DataPath SelectedGeometryPath;

  ChoicesParameter::ValueType TransformationSelection;
  ChoicesParameter::ValueType InterpolationSelection;

  DataPath ComputedTransformationMatrix;
  DynamicTableParameter::ValueType ManualMatrixTableData;
  VectorFloat32Parameter::ValueType Rotation;
  VectorFloat32Parameter::ValueType Translation;
  VectorFloat32Parameter::ValueType Scale;

  DataPath CellAttributeMatrixPath;
  bool RemoveOriginalGeometry;
  bool TranslateGeometryToGlobalOrigin;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT ApplyTransformationToGeometry
{
public:
  ApplyTransformationToGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ApplyTransformationToGeometryInputValues* inputValues);
  ~ApplyTransformationToGeometry() noexcept;

  ApplyTransformationToGeometry(const ApplyTransformationToGeometry&) = delete;
  ApplyTransformationToGeometry(ApplyTransformationToGeometry&&) noexcept = delete;
  ApplyTransformationToGeometry& operator=(const ApplyTransformationToGeometry&) = delete;
  ApplyTransformationToGeometry& operator=(ApplyTransformationToGeometry&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ApplyTransformationToGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  Result<> applyImageGeometryTransformation();

  Result<> applyNodeGeometryTransformation();

  ImageRotationUtilities::Matrix4fR m_TransformationMatrix;
};

} // namespace nx::core
