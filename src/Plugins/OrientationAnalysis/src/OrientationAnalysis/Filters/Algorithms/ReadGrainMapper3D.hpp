#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"
#include "OrientationAnalysis/utilities/GrainMapper3DUtilities.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <hdf5.h>

using namespace GrainMapper3DUtilities;

namespace nx::core
{

/**
 * @struct ReadGrainMapper3DInputValues
 * @brief Selects GrainMapper3D datasets, conversions, and destination paths.
 */
struct ORIENTATIONANALYSIS_EXPORT ReadGrainMapper3DInputValues
{
  std::filesystem::path InputFile;
  bool ReadDctData;
  DataPath DctImageGeometryPath;
  std::string DctCellAttributeMatrixName;
  std::string DctCellEnsembleAttributeMatrixName;
  bool ConvertPhaseData;
  bool ConvertOrientationData;
  bool ConvertIPFColors;

  bool ReadAbsorptionData;
  DataPath AbsorptionImageGeometryPath;
  std::string AbsorptionCellAttributeMatrixName;
};

namespace GM3DConstants
{
const std::string k_CrystalStructures("CrystalStructures");
const std::string k_LatticeConstants("LatticeConstants");
const std::string k_MaterialName("MaterialName");
const std::string k_UniversalHermannMauguin("UniversalHermannMauguin");
} // namespace GM3DConstants
/**
 * @class ReadGrainMapper3D
 * @brief Imports LabDCT and absorption volumes from a GrainMapper3D HDF5 file.
 *
 * Dataset conversions and ordinary volume reads use bounded HDF5 transfers.
 * Conversion buffers hold at most 65,536 values. Phase metadata remains a
 * small ensemble operation.
 *
 * Cancellation is checked between volume transfers. It returns success and
 * preserves arrays written before cancellation.
 */

class ORIENTATIONANALYSIS_EXPORT ReadGrainMapper3D
{
public:
  /**
   * @brief Initializes a GrainMapper3D importer.
   * @param dataStructure Provides destination arrays and geometries.
   * @param mesgHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation between transfers.
   * @param inputValues Selects source datasets, conversions, and destinations.
   * @pre All arguments outlive this importer.
   */
  ReadGrainMapper3D(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadGrainMapper3DInputValues* inputValues);
  ~ReadGrainMapper3D() noexcept = default;

  ReadGrainMapper3D(const ReadGrainMapper3D&) = delete;
  ReadGrainMapper3D(ReadGrainMapper3D&&) noexcept = delete;
  ReadGrainMapper3D& operator=(const ReadGrainMapper3D&) = delete;
  ReadGrainMapper3D& operator=(ReadGrainMapper3D&&) noexcept = delete;

  /**
   * @brief Imports the selected phase, LabDCT, and absorption data.
   * @return File, HDF5, shape, conversion, or destination transfer errors.
   * @pre Destination arrays match the selected source dataset shapes.
   * @pre Converted Rodrigues triples have nonzero magnitude.
   * @pre Converted IPF values are in the range [0, 1].
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

protected:
  /**
   * @brief Imports phase metadata into standard ensemble arrays.
   * @param reader Supplies GrainMapper3D phase metadata.
   * @param fileId Open source file identifier.
   * @return Phase-read errors.
   */
  Result<> copyPhaseInformation(GrainMapperReader& reader, hid_t fileId) const;

  /**
   * @brief Imports and converts selected LabDCT datasets.
   * @param reader Supplies dataset metadata.
   * @param fileId Open source file identifier.
   * @return HDF5, conversion, shape, or destination transfer errors.
   */
  Result<> copyDctData(GrainMapper3DUtilities::GrainMapperReader& reader, hid_t fileId) const;

  /**
   * @brief Imports the selected absorption volume.
   * @param reader Supplies source metadata.
   * @param fileId Open source file identifier.
   * @return HDF5 or destination transfer errors.
   */
  Result<> copyAbsorptionData(GrainMapperReader& reader, hid_t fileId) const;

private:
  DataStructure& m_DataStructure;
  const ReadGrainMapper3DInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
