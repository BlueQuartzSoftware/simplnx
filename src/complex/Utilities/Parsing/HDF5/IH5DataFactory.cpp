#include "IH5DataFactory.hpp"

#include "complex/Utilities/Parsing/HDF5/H5Reader.hpp"

using namespace complex;

IH5DataFactory::IH5DataFactory() = default;

IH5DataFactory::~IH5DataFactory() = default;

std::string IH5DataFactory::getObjName(H5::IdType targetId) const
{
  return complex::H5::Reader::Generic::getName(targetId);
}
