#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"
#include "simplnx/simplnx_export.hpp"

#include <mutex>

namespace nx::core
{

class IGridGeometry;

class SIMPLNX_EXPORT AlignSections
{
public:
  AlignSections(DataStructure& dataStructure, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);
  virtual ~AlignSections() noexcept;

  AlignSections(const AlignSections&) = delete;            // Copy Constructor Not Implemented
  AlignSections(AlignSections&&) = delete;                 // Move Constructor Not Implemented
  AlignSections& operator=(const AlignSections&) = delete; // Copy Assignment Not Implemented
  AlignSections& operator=(AlignSections&&) = delete;      // Move Assignment Not Implemented

  /**
   * @brief execute
   * @param gridGeom
   * @return
   */
  Result<> execute(const SizeVec3& udims, const DataPath& imageGeometryPath);

  const std::atomic_bool& getCancel();

  /**
   * @brief Thread-safe progress update. Safe to call from the parallel data-transfer workers.
   * @param counter Slices completed since the previous call
   */
  void sendThreadSafeProgressMessage(usize counter);

protected:
  /**
   * @brief Returns the message handler so a subclass can build its own throttle for the serial
   * shift-finding loop. Subclasses get the handler rather than a shared throttle, because
   * findShifts() runs on one thread.
   * @return
   */
  const IFilter::MessageHandler& getMessageHandler() const;

  /**
   * @brief This method finds the slice to slice shifts and should be implemented by subclasses
   * @param xShifts
   * @param yShifts
   * @return Whether the x and y shifts were successfully found
   */
  virtual Result<> findShifts(std::vector<int64_t>& xShifts, std::vector<int64_t>& yShifts) = 0;

  /**
   * @brief Returns the list of every child in the ImageGeometry's Cell Attribute Matrix
   * @param imageGeometryPath
   * @return List of DataPaths for each member inside the Cell Attribute Matrix
   */
  virtual std::vector<DataPath> getSelectedDataPaths(const DataPath& imageGeometryPath) const;

private:
  DataStructure& m_DataStructure;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  mutable std::mutex m_ProgressMessage_Mutex;
  ThrottledMessageHandler m_Throttle;
};

} // namespace nx::core
