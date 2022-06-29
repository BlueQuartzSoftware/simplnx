#pragma once

#include "complex/complex_export.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/IDataArray.hpp"
#include "complex/Filter/Arguments.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{

class AbstractGeometryGrid;

class COMPLEX_EXPORT AlignSections
{
public:
  AlignSections(DataStructure& data, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);
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
  Result<> execute(AbstractGeometryGrid& gridGeom);

  const std::atomic_bool& getCancel();

  void updateProgress(const std::string& progMessage);

protected:
  /**
   * @brief This should be overridden in the subclass.
   * @param xShifts
   * @param yShifts
   */
  virtual void find_shifts(std::vector<int64_t>& xShifts, std::vector<int64_t>& yShifts) = 0;

  virtual std::vector<DataPath> getSelectedDataPaths() const = 0;

private:
  DataStructure& m_DataStructure;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
