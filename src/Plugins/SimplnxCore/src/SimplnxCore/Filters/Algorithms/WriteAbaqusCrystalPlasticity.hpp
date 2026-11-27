#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

namespace nx::core
{
/**
 * @struct WriteAbaqusCrystalPlasticityInputValues
 * @brief Contains the values that control the Abaqus crystal-plasticity export.
 */
struct SIMPLNXCORE_EXPORT WriteAbaqusCrystalPlasticityInputValues
{
  /** @brief Directory that receives all five files. */
  FileSystemPathParameter::ValueType output_path;
  /** @brief Prefix for each output file name. */
  StringParameter::ValueType file_prefix;
  /** @brief Job name in the master file heading. */
  StringParameter::ValueType job_name;
  /** @brief True to write reduced-integration C3D8R elements. */
  bool use_reduced_integration = true;
  /** @brief Hourglass stiffness value for reduced-integration elements. */
  int32 hourglass_stiffness = 250;
  /** @brief True to convert the input Euler angles from radians to degrees. */
  bool write_euler_angles_in_degrees = false;
  /** @brief Number of solution-dependent state variables. */
  int32 num_depvar = 1;
  /** @brief Number of user output variables. */
  int32 num_user_out_var = 1;
  /** @brief True to request the unsymmetric equation solver for each material. */
  bool use_unsymmetric_solver = true;
  /** @brief True to write the grain and phase IDs as constants 1 and 2. */
  bool include_grain_and_phase_ids = true;
  /** @brief One-based position of the first of three Euler angles in the constant list. */
  int32 euler_angles_start_index = 3;
  /** @brief User constants in table order, with Euler angles inserted at EulerAnglesStartIndex. */
  DynamicTableParameter::ValueType material_constants;
  /** @brief Path to the Image Geometry that defines the voxel mesh. */
  DataPath image_geometry_path;
  /** @brief Path to the scalar cell feature IDs. */
  DataPath feature_ids_array_path;
  /** @brief Path to the three-component cell Euler angles in radians. */
  DataPath cell_euler_angles_array_path;
  /** @brief Path to the scalar cell phases. */
  DataPath cell_phases_array_path;
};

/**
 * @class WriteAbaqusCrystalPlasticity
 * @brief Writes five Abaqus input files for a voxel-based crystal-plasticity model.
 */
class SIMPLNXCORE_EXPORT WriteAbaqusCrystalPlasticity
{
public:
  /**
   * @brief Constructs the algorithm.
   * @param dataStructure Contains the input geometry and arrays.
   * @param messageHandler Receives progress messages.
   * @param shouldCancel Indicates that file generation must stop.
   * @param inputValues Supplies the export parameters for the call lifetime.
   */
  WriteAbaqusCrystalPlasticity(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, WriteAbaqusCrystalPlasticityInputValues* inputValues);

  ~WriteAbaqusCrystalPlasticity() noexcept = default;

  WriteAbaqusCrystalPlasticity(const WriteAbaqusCrystalPlasticity&) = delete;
  WriteAbaqusCrystalPlasticity(WriteAbaqusCrystalPlasticity&&) noexcept = delete;
  WriteAbaqusCrystalPlasticity& operator=(const WriteAbaqusCrystalPlasticity&) = delete;
  WriteAbaqusCrystalPlasticity& operator=(WriteAbaqusCrystalPlasticity&&) noexcept = delete;

  /**
   * @brief Generates and commits all five Abaqus files.
   * @return Success, cancellation, an invalid-grain error, or a file-writing error.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const WriteAbaqusCrystalPlasticityInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
