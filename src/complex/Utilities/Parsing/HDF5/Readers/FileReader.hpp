#pragma once

#include "complex/Utilities/Parsing/HDF5/Readers/GroupReader.hpp"
#include "complex/complex_export.hpp"

#include <filesystem>
#include <string>

namespace complex::HDF5
{
class COMPLEX_EXPORT FileReader : public GroupReader
{
public:
  /**
   * @brief Constructs a FileReader wrapping the HDF5 file at the target
   * filepath. The constructed object will be invalid if the HDF5 file cannot
   * be found or openned.
   * @param filepath
   */
  FileReader(const std::filesystem::path& filepath);

  /**
   * @brief Constructs a FileReader wrapping the target HDF5 file ID. The
   * constructed object will be invalid if the provided HDF5 file ID is 0.
   * @param fileId
   */
  FileReader(IdType fileId);

  /**
   * @brief Releases the HDF5 file ID.
   */
  virtual ~FileReader() noexcept;

  /**
   * @brief Returns the HDF5 file name. Returns an empty string if the file
   * is invalid.
   * @return std::string
   */
  std::string getName() const override;

protected:
  /**
   * @brief Closes the HDF5 ID and resets it to 0.
   */
  void closeHdf5() override;
};
} // namespace complex::HDF5
