#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{

/**
 * @struct WriteGBCDGMTFileInputValues
 * @brief Selects one GBCD phase, misorientation, and GMT destination.
 */
struct ORIENTATIONANALYSIS_EXPORT WriteGBCDGMTFileInputValues
{
  int32 PhaseOfInterest;                                    ///< One-based phase index in the GBCD array.
  VectorFloat32Parameter::ValueType MisorientationRotation; ///< Angle in degrees followed by axis x, y, and z.
  FileSystemPathParameter::ValueType OutputFile;
  DataPath GBCDArrayPath;
  DataPath CrystalStructuresArrayPath;
};

/**
 * @class WriteGBCDGMTFile
 * @brief Writes a GBCD stereographic projection in Generic Mapping Tools format.
 *
 * The algorithm caches only the selected five-dimensional GBCD phase slice and
 * the ensemble crystal structures. Symmetry loops then use local buffers and do
 * not access the source stores.
 *
 * The current implementation does not inspect the two bulk-read Result values
 * or output-write results. Cancellation returns success after it creates an
 * empty output file and before it writes projection values.
 */
class ORIENTATIONANALYSIS_EXPORT WriteGBCDGMTFile
{
public:
  /**
   * @brief Initializes a GBCD GMT writer.
   * @param dataStructure Provides GBCD and crystal-structure arrays.
   * @param mesgHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation between projection rows.
   * @param inputValues Selects phase, misorientation, and output path.
   * @pre All arguments outlive this writer.
   */
  WriteGBCDGMTFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteGBCDGMTFileInputValues* inputValues);
  ~WriteGBCDGMTFile() noexcept;

  WriteGBCDGMTFile(const WriteGBCDGMTFile&) = delete;
  WriteGBCDGMTFile(WriteGBCDGMTFile&&) noexcept = delete;
  WriteGBCDGMTFile& operator=(const WriteGBCDGMTFile&) = delete;
  WriteGBCDGMTFile& operator=(WriteGBCDGMTFile&&) noexcept = delete;

  /**
   * @brief Computes and writes the selected stereographic projection.
   * @return Directory-creation or file-open errors.
   * @pre PhaseOfInterest indexes both source arrays and its GBCD phase slice.
   * @pre MisorientationRotation contains an angle and a nonzero three-component axis.
   * @pre The GBCD component shape has five positive dimensions whose products fit int32.
   * @pre The selected crystal structure identifies a valid LaueOps entry.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const WriteGBCDGMTFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
