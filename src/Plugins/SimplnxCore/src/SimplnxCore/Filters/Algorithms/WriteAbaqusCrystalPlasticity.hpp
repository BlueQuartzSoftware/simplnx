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
  FileSystemPathParameter::ValueType OutputPath;
  /** @brief Prefix for each output file name. */
  StringParameter::ValueType FilePrefix;
  /** @brief Job name in the master file heading. */
  StringParameter::ValueType JobName;
  /** @brief Number of solution-dependent state variables. */
  int32 NumDepvar = 1;
  /** @brief Number of user output variables. */
  int32 NumUserOutVar = 1;
  /** @brief User constants that follow the five generated constants. */
  DynamicTableParameter::ValueType MaterialConstants;
  /** @brief Path to the voxel geometry. */
  DataPath ImageGeometryPath;
  /** @brief Path to the scalar cell feature IDs. */
  DataPath FeatureIdsArrayPath;
  /** @brief Path to the three-component cell Euler angles in radians. */
  DataPath CellEulerAnglesArrayPath;
  /** @brief Path to the scalar cell phases. */
  DataPath CellPhasesArrayPath;
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

  ~WriteAbaqusCrystalPlasticity() noexcept;

  WriteAbaqusCrystalPlasticity(const WriteAbaqusCrystalPlasticity&) = delete;
  WriteAbaqusCrystalPlasticity(WriteAbaqusCrystalPlasticity&&) noexcept = delete;
  WriteAbaqusCrystalPlasticity& operator=(const WriteAbaqusCrystalPlasticity&) = delete;
  WriteAbaqusCrystalPlasticity& operator=(WriteAbaqusCrystalPlasticity&&) noexcept = delete;

  /**
   * @brief Generates and commits all five Abaqus files.
   * @return Success, cancellation, or a file-writing error.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const WriteAbaqusCrystalPlasticityInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
