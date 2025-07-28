#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  InterpolatePointCloudToRegularGridInputValues inputValues;
  inputValues.CopyArrays = filterArgs.value<MultiArraySelectionParameter::ValueType>(copy_arrays);
  inputValues.GaussianSigmas = filterArgs.value<VectorFloat32Parameter::ValueType>(gaussian_sigmas);
  inputValues.InputImageGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(input_image_geometry_path);
  inputValues.InputMaskPath = filterArgs.value<ArraySelectionParameter::ValueType>(input_mask_path);
  inputValues.InputVertexGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(input_vertex_geometry_path);
  inputValues.InterpolateArrays = filterArgs.value<MultiArraySelectionParameter::ValueType>(interpolate_arrays);
  inputValues.InterpolatedGroupName = filterArgs.value<DataObjectNameParameter::ValueType>(interpolated_group_name);
  inputValues.InterpolationIndex = filterArgs.value<ChoicesParameter::ValueType>(interpolation_index);
  inputValues.KernelDistancesArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(kernel_distances_array_name);
  inputValues.KernelSize = filterArgs.value<VectorFloat32Parameter::ValueType>(kernel_size);
  inputValues.StoreKernelDistances = filterArgs.value<BoolParameter::ValueType>(store_kernel_distances);
  inputValues.UseMask = filterArgs.value<BoolParameter::ValueType>(use_mask);
  inputValues.VoxelIndicesPath = filterArgs.value<ArraySelectionParameter::ValueType>(voxel_indices_path);
  return InterpolatePointCloudToRegularGrid(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT InterpolatePointCloudToRegularGridInputValues
{
  MultiArraySelectionParameter::ValueType CopyArrays;
  VectorFloat32Parameter::ValueType GaussianSigmas;
  GeometrySelectionParameter::ValueType InputImageGeometryPath;
  ArraySelectionParameter::ValueType InputMaskPath;
  GeometrySelectionParameter::ValueType InputVertexGeometryPath;
  MultiArraySelectionParameter::ValueType InterpolateArrays;
  DataObjectNameParameter::ValueType InterpolatedGroupName;
  ChoicesParameter::ValueType InterpolationIndex;
  DataObjectNameParameter::ValueType KernelDistancesArrayName;
  VectorFloat32Parameter::ValueType KernelSize;
  BoolParameter::ValueType StoreKernelDistances;
  BoolParameter::ValueType UseMask;
  ArraySelectionParameter::ValueType VoxelIndicesPath;
};

/**
 * @class InterpolatePointCloudToRegularGrid
 * @brief This algorithm implements support code for the InterpolatePointCloudToRegularGridFilter
 */

class SIMPLNXCORE_EXPORT InterpolatePointCloudToRegularGrid
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

private:
  DataStructure& m_DataStructure;
  const InterpolatePointCloudToRegularGridInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
