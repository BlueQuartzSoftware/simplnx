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
   * @brief Checks the axis of the Misorientation Axis-Angle. preflightImpl() and operator()() both call it.
   * @param misorientationRotation Angle in degrees followed by the h, k and l axis components.
   * @return k_InvalidMisorientationAxisError when an axis component is NaN or infinite or the axis is [0, 0, 0].
   */
  static Result<> ValidateMisorientationAxis(const std::vector<float32>& misorientationRotation);

  /// Error code for a Misorientation Axis-Angle axis that is not finite or has length 0.
  static constexpr int32 k_InvalidMisorientationAxisError = -96714;

  /**
   * @brief Checks Phase of Interest against the ensemble counts. preflightImpl() and operator()() both call it.
   * @param phaseOfInterest One-based phase index.
   * @param numGbcdPhases Number of tuples of the GBCD array.
   * @param numCrystalStructures Number of tuples of the Crystal Structures array.
   * @param gbcdArrayPath GBCD array path, for the message.
   * @param crystalStructuresArrayPath Crystal Structures array path, for the message.
   * @return k_PhaseNotPositiveError, k_PhaseNotInGbcdError or k_PhaseNotInCrystalStructuresError, or a valid result.
   */
  static Result<> ValidatePhaseOfInterest(int32 phaseOfInterest, usize numGbcdPhases, usize numCrystalStructures, const DataPath& gbcdArrayPath, const DataPath& crystalStructuresArrayPath);

  /// Error code for a Phase of Interest that is 0 or negative.
  static constexpr int32 k_PhaseNotPositiveError = -96715;
  /// Error code for a Phase of Interest that the GBCD array does not contain.
  static constexpr int32 k_PhaseNotInGbcdError = -96712;
  /// Error code for a Phase of Interest that the Crystal Structures array does not contain.
  static constexpr int32 k_PhaseNotInCrystalStructuresError = -96716;
  /// Error code for a Crystal Structures value of the Phase of Interest that has no Laue operations (for example 999, Unknown).
  static constexpr int32 k_InvalidLaueIndexError = -96717;

  /**
   * @brief Computes and writes the selected stereographic projection.
   * @return An invalid misorientation axis error, Phase of Interest errors, an error when the crystal structure of the
   * Phase of Interest has no Laue operations, or directory-creation or file-open errors.
   * @pre The GBCD component shape has five positive dimensions whose products fit int32.
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
