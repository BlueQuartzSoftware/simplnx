#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{

class IGridGeometry;

/**
 * @class AlignSections
 * @brief Provides shared slice-shift discovery and cell-array transfer.
 *
 * A subclass calculates one X and Y shift for each image Z slice. execute()
 * applies those shifts to every cell array. In-memory arrays can transfer in
 * parallel when each task owns a different array. If any selected array is OOC,
 * all transfers use sequential slice buffers and bulk store I/O. A forced OOC
 * test path uses the same bounded transfer.
 *
 * The object holds non-owning references and is not thread-safe. The data
 * structure, cancellation flag, and message handler must outlive it.
 */
class SIMPLNX_EXPORT AlignSections
{
public:
  /**
   * @brief Creates the shared alignment operation.
   * @param dataStructure Supplies image and cell data to modify.
   * @param shouldCancel Supplies the cancellation flag.
   * @param mesgHandler Receives progress messages.
   */
  AlignSections(DataStructure& dataStructure, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);
  virtual ~AlignSections() noexcept;

  AlignSections(const AlignSections&) = delete;
  AlignSections(AlignSections&&) = delete;
  AlignSections& operator=(const AlignSections&) = delete;
  AlignSections& operator=(AlignSections&&) = delete;

  /**
   * @brief Calculates slice shifts and applies them to selected cell arrays.
   * @param udims Specifies image dimensions in X, Y, Z order.
   * @param imageGeometryPath Identifies the image geometry.
   * @return Shift-discovery or OOC bulk-I/O result.
   * @pre udims matches the image geometry. All dimension and component products fit usize.
   *
   * Cancellation returns a valid result. Arrays can contain a partially applied
   * alignment when cancellation occurs during transfer.
   */
  Result<> execute(const SizeVec3& udims, const DataPath& imageGeometryPath);

  const std::atomic_bool& getCancel();

  MessageHelper& getMessageHelper();

protected:
  /**
   * @brief Calculates one X and Y shift for each Z slice.
   * @param xShifts Receives X shifts and is pre-sized to the Z dimension.
   * @param yShifts Receives Y shifts and is pre-sized to the Z dimension.
   * @return Valid result when all shifts are available.
   */
  virtual Result<> findShifts(std::vector<int64_t>& xShifts, std::vector<int64_t>& yShifts) = 0;

  /**
   * @brief Gets every direct child of the image cell-data group.
   * @param imageGeometryPath Identifies the image geometry.
   * @return Direct child paths. execute() skips children that are not IDataArray objects.
   */
  virtual std::vector<DataPath> getSelectedDataPaths(const DataPath& imageGeometryPath) const;

private:
  DataStructure& m_DataStructure;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  MessageHelper m_MessageHelper;
};

} // namespace nx::core
