#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT CombineStlFilesInputValues
{
  FileSystemPathParameter::ValueType stl_files_path;
  DataPath triangle_data_container_name;
  DataPath face_attribute_matrix_name;
  DataPath face_normals_array_name;

  DataPath part_number_index_array_path;
  bool create_part_numbers{};

  DataPath face_label_index_array_path;
  bool create_face_labels{};

  DataPath vertex_file_index_array_path;
  bool label_vertices{};

  DataObjectNameParameter::ValueType cell_feature_attribute_matrix_name;
  DataObjectNameParameter::ValueType active_array_name;
  DataObjectNameParameter::ValueType file_list_array_name;
};

/**
 * @class CombineStlFiles
 * @brief This filter combines all of the STL files from a given directory into a single triangle geometry
 */

class SIMPLNXCORE_EXPORT CombineStlFiles
{
public:
  CombineStlFiles(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CombineStlFilesInputValues* inputValues);
  ~CombineStlFiles() noexcept = default;

  CombineStlFiles(const CombineStlFiles&) = delete;
  CombineStlFiles(CombineStlFiles&&) noexcept = delete;
  CombineStlFiles& operator=(const CombineStlFiles&) = delete;
  CombineStlFiles& operator=(CombineStlFiles&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CombineStlFilesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
