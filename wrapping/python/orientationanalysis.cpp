#include <CxPybind/CxPybind.hpp>

#include <OrientationAnalysis/OrientationAnalysisPlugin.hpp>

#include <OrientationAnalysis/Parameters/H5EbsdReaderParameter.h>
#include <OrientationAnalysis/Parameters/OEMEbsdScanSelectionParameter.h>

#include "OrientationAnalysis/OrientationAnalysisFilterBinding.hpp"

using namespace complex;
using namespace complex::CxPybind;
namespace py = pybind11;

using namespace pybind11::literals;

PYBIND11_MODULE(orientationanalysis, mod)
{
  py::module_::import("complex");

  auto& internals = Internals::Instance();

  auto* plugin = internals.addPlugin<OrientationAnalysisPlugin>();

  auto h5EbsdReaderParameter = COMPLEX_PY_BIND_PARAMETER(mod, H5EbsdReaderParameter);
  auto oemEbsdScanSelectionParameter = COMPLEX_PY_BIND_PARAMETER(mod, OEMEbsdScanSelectionParameter);

  py::class_<H5EbsdReaderParameter::ValueType> h5EbsdReaderValueType(h5EbsdReaderParameter, "ValueType");
  h5EbsdReaderValueType.def(py::init<>());
  h5EbsdReaderValueType.def_readwrite("input_file_path", &H5EbsdReaderParameter::ValueType::inputFilePath);
  h5EbsdReaderValueType.def_readwrite("start_slice", &H5EbsdReaderParameter::ValueType::startSlice);
  h5EbsdReaderValueType.def_readwrite("end_slice", &H5EbsdReaderParameter::ValueType::startSlice);
  h5EbsdReaderValueType.def_readwrite("euler_representation", &H5EbsdReaderParameter::ValueType::eulerRepresentation);
  h5EbsdReaderValueType.def_readwrite("hdf5_data_paths", &H5EbsdReaderParameter::ValueType::hdf5DataPaths);
  h5EbsdReaderValueType.def_readwrite("use_recommended_transform", &H5EbsdReaderParameter::ValueType::useRecommendedTransform);

  py::class_<OEMEbsdScanSelectionParameter::ValueType> oemEbsdScanSelectionValueType(oemEbsdScanSelectionParameter, "ValueType");
  oemEbsdScanSelectionValueType.def(py::init<>());
  oemEbsdScanSelectionValueType.def_readwrite("input_file_path", &OEMEbsdScanSelectionParameter::ValueType::inputFilePath);
  oemEbsdScanSelectionValueType.def_readwrite("stacking_order", &OEMEbsdScanSelectionParameter::ValueType::stackingOrder);
  oemEbsdScanSelectionValueType.def_readwrite("scan_names", &OEMEbsdScanSelectionParameter::ValueType::scanNames);

  py::enum_<OEMEbsdScanSelectionParameter::EbsdReaderType> oemEbsdScanSelectionEbsdReaderType(oemEbsdScanSelectionParameter, "EbsdReaderType");
  oemEbsdScanSelectionEbsdReaderType.value("Oim", OEMEbsdScanSelectionParameter::EbsdReaderType::Oim);
  oemEbsdScanSelectionEbsdReaderType.value("Esprit", OEMEbsdScanSelectionParameter::EbsdReaderType::Esprit);

  py::enum_<OEMEbsdScanSelectionParameter::ManufacturerType> oemEbsdScanSelectionManufacturerType(oemEbsdScanSelectionParameter, "ManufacturerType");
  oemEbsdScanSelectionManufacturerType.value("EDAX", OEMEbsdScanSelectionParameter::ManufacturerType::EDAX);
  oemEbsdScanSelectionManufacturerType.value("Oxford", OEMEbsdScanSelectionParameter::ManufacturerType::Oxford);
  oemEbsdScanSelectionManufacturerType.value("Bruker", OEMEbsdScanSelectionParameter::ManufacturerType::Bruker);
  oemEbsdScanSelectionManufacturerType.value("HEDM", OEMEbsdScanSelectionParameter::ManufacturerType::HEDM);
  oemEbsdScanSelectionManufacturerType.value("Zeiss", OEMEbsdScanSelectionParameter::ManufacturerType::Zeiss);
  oemEbsdScanSelectionManufacturerType.value("Phillips", OEMEbsdScanSelectionParameter::ManufacturerType::Phillips);
  oemEbsdScanSelectionManufacturerType.value("ThermoFisher", OEMEbsdScanSelectionParameter::ManufacturerType::ThermoFisher);
  oemEbsdScanSelectionManufacturerType.value("DREAM3D", OEMEbsdScanSelectionParameter::ManufacturerType::DREAM3D);
  oemEbsdScanSelectionManufacturerType.value("Unknown", OEMEbsdScanSelectionParameter::ManufacturerType::Unknown);

  BindParameterConstructor(h5EbsdReaderParameter);

  oemEbsdScanSelectionParameter.def(
      py::init<const std::string&, const std::string&, const std::string&, const OEMEbsdScanSelectionParameter::ValueType&, const OEMEbsdScanSelectionParameter::AllowedManufacturers&,
               OEMEbsdScanSelectionParameter::EbsdReaderType, OEMEbsdScanSelectionParameter::ExtensionsType>(),
      "name"_a, "human_name"_a, "help_text"_a, "default_value"_a, "allowed_manufacturers"_a, "reader_type"_a, "extensions_type"_a);

  internals.addConversion<H5EbsdReaderParameter>();
  internals.addConversion<OEMEbsdScanSelectionParameter>();

  OrientationAnalysis::BindFilters(mod, internals);

  internals.registerPluginPyFilters(*plugin);
}
