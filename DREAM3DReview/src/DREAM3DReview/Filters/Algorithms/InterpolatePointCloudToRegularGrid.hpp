#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  InterpolatePointCloudToRegularGridInputValues inputValues;

  inputValues.UseMask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.StoreKernelDistances = filterArgs.value<bool>(k_StoreKernelDistances_Key);
  inputValues.InterpolationTechnique = filterArgs.value<ChoicesParameter::ValueType>(k_InterpolationTechnique_Key);
  inputValues.KernelSize = filterArgs.value<VectorFloat32Parameter::ValueType>(k_KernelSize_Key);
  inputValues.Sigmas = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Sigmas_Key);
  inputValues.DataContainerName = filterArgs.value<DataPath>(k_DataContainerName_Key);
  inputValues.InterpolatedDataContainerName = filterArgs.value<DataPath>(k_InterpolatedDataContainerName_Key);
  inputValues.VoxelIndicesArrayPath = filterArgs.value<DataPath>(k_VoxelIndicesArrayPath_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.ArraysToInterpolate = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_ArraysToInterpolate_Key);
  inputValues.ArraysToCopy = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_ArraysToCopy_Key);
  inputValues.InterpolatedAttributeMatrixName = filterArgs.value<DataPath>(k_InterpolatedAttributeMatrixName_Key);
  inputValues.KernelDistancesArrayName = filterArgs.value<DataPath>(k_KernelDistancesArrayName_Key);
  inputValues.InterpolatedSuffix = filterArgs.value<StringParameter::ValueType>(k_InterpolatedSuffix_Key);
  inputValues.CopySuffix = filterArgs.value<StringParameter::ValueType>(k_CopySuffix_Key);

  return InterpolatePointCloudToRegularGrid(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT InterpolatePointCloudToRegularGridInputValues
{
  bool UseMask;
  bool StoreKernelDistances;
  ChoicesParameter::ValueType InterpolationTechnique;
  VectorFloat32Parameter::ValueType KernelSize;
  VectorFloat32Parameter::ValueType Sigmas;
  DataPath DataContainerName;
  DataPath InterpolatedDataContainerName;
  DataPath VoxelIndicesArrayPath;
  DataPath MaskArrayPath;
  MultiArraySelectionParameter::ValueType ArraysToInterpolate;
  MultiArraySelectionParameter::ValueType ArraysToCopy;
  DataPath InterpolatedAttributeMatrixName;
  DataPath KernelDistancesArrayName;
  StringParameter::ValueType InterpolatedSuffix;
  StringParameter::ValueType CopySuffix;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT InterpolatePointCloudToRegularGrid
{
public:
  InterpolatePointCloudToRegularGrid(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                     InterpolatePointCloudToRegularGridInputValues* inputValues);
  ~InterpolatePointCloudToRegularGrid() noexcept;

  InterpolatePointCloudToRegularGrid(const InterpolatePointCloudToRegularGrid&) = delete;
  InterpolatePointCloudToRegularGrid(InterpolatePointCloudToRegularGrid&&) noexcept = delete;
  InterpolatePointCloudToRegularGrid& operator=(const InterpolatePointCloudToRegularGrid&) = delete;
  InterpolatePointCloudToRegularGrid& operator=(InterpolatePointCloudToRegularGrid&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const InterpolatePointCloudToRegularGridInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
