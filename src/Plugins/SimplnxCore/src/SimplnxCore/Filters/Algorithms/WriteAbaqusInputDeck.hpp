#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

#include <optional>

namespace nx::core
{
/**
 * @struct AbaqusCrystalPlasticityMaterialValues
 * @brief Contains the optional user-material values for a crystal-plasticity input deck.
 */
struct SIMPLNXCORE_EXPORT AbaqusCrystalPlasticityMaterialValues
{
  int32 NumDepvar = 1;
  int32 NumUserOutVar = 1;
  DynamicTableParameter::ValueType MaterialConstants;
  DataPath CellEulerAnglesArrayPath;
};

/**
 * @struct WriteAbaqusInputDeckInputValues
 * @brief Contains the values that control the shared Abaqus input-deck export.
 */
struct SIMPLNXCORE_EXPORT WriteAbaqusInputDeckInputValues
{
  FileSystemPathParameter::ValueType OutputPath;
  StringParameter::ValueType FilePrefix;
  StringParameter::ValueType JobName;
  bool UseReducedIntegration = true;
  int32 HourglassStiffness = 250;
  bool WriteDummyNode = false;
  DataPath ImageGeometryPath;
  DataPath FeatureIdsArrayPath;
  DataPath CellPhasesArrayPath;
  std::optional<AbaqusCrystalPlasticityMaterialValues> CrystalPlasticityMaterial;
};

/**
 * @brief Verifies that the common Abaqus cell arrays match the selected Image Geometry.
 * @param dataStructure Contains the Image Geometry and cell arrays.
 * @param imageGeometryPath Selects the Image Geometry that defines the required tuple count.
 * @param featureIdsArrayPath Selects the scalar Feature IDs array.
 * @param cellPhasesArrayPath Selects the scalar Cell Phases array.
 * @return An error if an array tuple count does not match the geometry cell count.
 */
SIMPLNXCORE_EXPORT Result<> ValidateAbaqusInputCellArrays(const DataStructure& dataStructure, const DataPath& imageGeometryPath, const DataPath& featureIdsArrayPath,
                                                          const DataPath& cellPhasesArrayPath);

/**
 * @class WriteAbaqusInputDeck
 * @brief Writes five Abaqus input files for a voxel-based finite-element model.
 */
class SIMPLNXCORE_EXPORT WriteAbaqusInputDeck
{
public:
  /**
   * @brief Constructs the shared Abaqus input-deck writer.
   * @param dataStructure Contains the input geometry and arrays.
   * @param messageHandler Receives progress messages.
   * @param shouldCancel Indicates that file generation must stop.
   * @param inputValues Supplies the export parameters for the call lifetime.
   */
  WriteAbaqusInputDeck(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, WriteAbaqusInputDeckInputValues* inputValues);

  /**
   * @brief Destroys the writer.
   */
  ~WriteAbaqusInputDeck() noexcept;

  WriteAbaqusInputDeck(const WriteAbaqusInputDeck&) = delete;
  WriteAbaqusInputDeck(WriteAbaqusInputDeck&&) noexcept = delete;
  WriteAbaqusInputDeck& operator=(const WriteAbaqusInputDeck&) = delete;
  WriteAbaqusInputDeck& operator=(WriteAbaqusInputDeck&&) noexcept = delete;

  /**
   * @brief Generates and commits all five Abaqus files.
   * @return Success, cancellation, an invalid-grain error, or a file-writing error.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const WriteAbaqusInputDeckInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
