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
 * @brief Input values for the WriteGBCDGMTFile algorithm.
 */
struct ORIENTATIONANALYSIS_EXPORT WriteGBCDGMTFileInputValues
{
  int32 PhaseOfInterest;                                    ///< Phase index to extract from the GBCD array.
  VectorFloat32Parameter::ValueType MisorientationRotation; ///< Misorientation as [angle, axis_x, axis_y, axis_z].
  FileSystemPathParameter::ValueType OutputFile;            ///< Path to the output GMT file.
  DataPath GBCDArrayPath;                                   ///< Path to the GBCD (5D) DataArray.
  DataPath CrystalStructuresArrayPath;                      ///< Path to the CrystalStructures ensemble array.
};

/**
 * @class WriteGBCDGMTFile
 * @brief Writes Grain Boundary Character Distribution (GBCD) data in GMT format for stereographic
 * projection plotting.
 *
 * @section ooc_summary OOC Optimization Summary
 * The GBCD array can be hundreds of MB (5-dimensional). Rather than accessing individual bins
 * via operator[] during the O(nSym^2 * thetaPoints * phiPoints) inner loops, the algorithm
 * caches only the phase-of-interest slice via a single copyIntoBuffer() call. Crystal
 * structures (ensemble-level, tiny) are also cached locally. All subsequent lookups use
 * the local buffers with zero OOC store access.
 */
class ORIENTATIONANALYSIS_EXPORT WriteGBCDGMTFile
{
public:
  WriteGBCDGMTFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteGBCDGMTFileInputValues* inputValues);
  ~WriteGBCDGMTFile() noexcept;

  WriteGBCDGMTFile(const WriteGBCDGMTFile&) = delete;
  WriteGBCDGMTFile(WriteGBCDGMTFile&&) noexcept = delete;
  WriteGBCDGMTFile& operator=(const WriteGBCDGMTFile&) = delete;
  WriteGBCDGMTFile& operator=(WriteGBCDGMTFile&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const WriteGBCDGMTFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
