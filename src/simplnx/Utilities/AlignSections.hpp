#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/simplnx_export.hpp"

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
  Result<> execute(const SizeVec3& udims);

  const std::atomic_bool& getCancel();

  void updateProgress(const std::string& progMessage);

protected:
  /**
   * @brief This should be overridden in the subclass.
   * @param xShifts
   * @param yShifts
   * @return Whether or not the x and y shifts were successfully found
   */
  virtual Result<> findShifts(std::vector<int64_t>& xShifts, std::vector<int64_t>& yShifts) = 0;

  virtual std::vector<DataPath> getSelectedDataPaths() const = 0;

private:
  DataStructure& m_DataStructure;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
