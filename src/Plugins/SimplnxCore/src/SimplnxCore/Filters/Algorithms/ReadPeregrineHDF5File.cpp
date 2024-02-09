#include "ReadPeregrineHDF5File.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/Utilities/Parsing/HDF5/Readers/FileReader.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

using namespace nx::core;

ReadPeregrineHDF5File::ReadPeregrineHDF5File(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel,
                                             ReadPeregrineHDF5FileInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(msgHandler)
{
}

ReadPeregrineHDF5File::~ReadPeregrineHDF5File() noexcept = default;

Result<> ReadPeregrineHDF5File::operator()()
{

  return {};
}
