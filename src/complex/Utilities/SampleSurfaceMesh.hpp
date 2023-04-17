#pragma once

#include "complex/Common/Array.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/IDataArray.hpp"
#include "complex/Filter/Arguments.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/complex_export.hpp"

namespace complex
{

class IGridGeometry;

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

  /**
   * @brief This will read in a shifts file written by another DREAM3D alignment filter and populate the shifts parameters with the values as int64 numbers.
   * @param file The DREAM3D formatted alignment file to read
   * @param zDim The z dimension of the geometry being shifted
   * @param xShifts
   * @param yShifts
   * @return Whether or not the x and y shifts were successfully found
   */
  Result<> readDream3dShiftsFile(const std::filesystem::path& file, int64 zDim, std::vector<int64_t>& xShifts, std::vector<int64_t>& yShifts) const;

  /**
   * @brief This will read in a shifts file defined by the user and populate the shifts parameters with the values as int64 numbers.
   * @param file The user formatted alignment file to read
   * @param zDim The z dimension of the geometry being shifted
   * @param xShifts
   * @param yShifts
   * @return Whether or not the x and y shifts were successfully found
   */
  Result<> readUserShiftsFile(const std::filesystem::path& file, int64 zDim, std::vector<int64_t>& xShifts, std::vector<int64_t>& yShifts) const;

private:
  DataStructure& m_DataStructure;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
