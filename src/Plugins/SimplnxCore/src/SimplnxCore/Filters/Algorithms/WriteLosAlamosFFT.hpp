#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

namespace nx::core
{

/**
 * @struct WriteLosAlamosFFTInputValues
 * @brief Stores output and source-array paths.
 */
struct SIMPLNXCORE_EXPORT WriteLosAlamosFFTInputValues
{
  FileSystemPathParameter::ValueType OutputFile;
  DataPath FeatureIdsArrayPath;
  DataPath CellEulerAnglesArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath ImageGeomPath;
};

/**
 * @class WriteLosAlamosFFT
 * @brief Writes an eight-column Los Alamos FFT text export from image-cell data.
 *
 * Contiguous arrays are formatted directly to the output stream, while
 * disk-backed arrays use bounded tuple batches. This avoids per-voxel DataStore I/O.
 */
class SIMPLNXCORE_EXPORT WriteLosAlamosFFT
{
public:
  /**
   * @brief Creates a Los Alamos FFT writer.
   * @param dataStructure Provides image geometry and source arrays.
   * @param mesgHandler Is retained but not used.
   * @param shouldCancel Stops before later tuple chunks when true.
   * @param inputValues Specifies validated paths. The caller must keep this object
   * alive for the writer lifetime.
   */
  WriteLosAlamosFFT(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteLosAlamosFFTInputValues* inputValues);
  /**
   * @brief Destroys the non-owning writer.
   */
  ~WriteLosAlamosFFT() noexcept;

  WriteLosAlamosFFT(const WriteLosAlamosFFT&) = delete;
  WriteLosAlamosFFT(WriteLosAlamosFFT&&) noexcept = delete;
  WriteLosAlamosFFT& operator=(const WriteLosAlamosFFT&) = delete;
  WriteLosAlamosFFT& operator=(WriteLosAlamosFFT&&) noexcept = delete;

  /**
   * @brief Writes all image tuples as eight text columns.
   * @return Directory, file, DataStore, stream, or cancellation error.
   *
   * The writer opens the destination directly. An error or cancellation can leave
   * a partial file and can replace an existing destination.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const WriteLosAlamosFFTInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
