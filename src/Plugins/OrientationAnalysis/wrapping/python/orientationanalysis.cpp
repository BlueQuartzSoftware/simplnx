#include <CxPybind/CxPybind.hpp>

#include <OrientationAnalysis/OrientationAnalysisPlugin.hpp>

#include <OrientationAnalysis/Parameters/OEMEbsdScanSelectionParameter.h>
#include <OrientationAnalysis/Parameters/ReadH5EbsdFileParameter.h>

#include "OrientationAnalysis/OrientationAnalysisFilterBinding.hpp"

using namespace nx::core;
using namespace nx::core::CxPybind;
namespace py = pybind11;

using namespace pybind11::literals;

PYBIND11_MODULE(orientationanalysis, mod)
{
  py::module_::import("simplnx");

  auto& internals = Internals::Instance();

  auto* plugin = internals.addPlugin<OrientationAnalysisPlugin>();

  auto h5EbsdReaderParameter = SIMPLNX_PY_BIND_PARAMETER(mod, ReadH5EbsdFileParameter);
  auto oemEbsdScanSelectionParameter = SIMPLNX_PY_BIND_PARAMETER(mod, OEMEbsdScanSelectionParameter);

  py::class_<ReadH5EbsdFileParameter::ValueType> h5EbsdReaderValueType(h5EbsdReaderParameter, "ValueType");
  h5EbsdReaderValueType.def(py::init<>());
  h5EbsdReaderValueType.def_readwrite("input_file_path", &ReadH5EbsdFileParameter::ValueType::inputFilePath);
  h5EbsdReaderValueType.def_readwrite("start_slice", &ReadH5EbsdFileParameter::ValueType::startSlice);
  h5EbsdReaderValueType.def_readwrite("end_slice", &ReadH5EbsdFileParameter::ValueType::endSlice);
  h5EbsdReaderValueType.def_readwrite("euler_representation", &ReadH5EbsdFileParameter::ValueType::eulerRepresentation);
  h5EbsdReaderValueType.def_readwrite("selected_array_names", &ReadH5EbsdFileParameter::ValueType::selectedArrayNames);
  h5EbsdReaderValueType.def_readwrite("use_recommended_transform", &ReadH5EbsdFileParameter::ValueType::useRecommendedTransform);

  py::class_<OEMEbsdScanSelectionParameter::ValueType> oemEbsdScanSelectionValueType(oemEbsdScanSelectionParameter, "ValueType");
  oemEbsdScanSelectionValueType.def(py::init<>());
  oemEbsdScanSelectionValueType.def_readwrite("input_file_path", &OEMEbsdScanSelectionParameter::ValueType::inputFilePath);
  oemEbsdScanSelectionValueType.def_readwrite("stacking_order", &OEMEbsdScanSelectionParameter::ValueType::stackingOrder);
  oemEbsdScanSelectionValueType.def_readwrite("scan_names", &OEMEbsdScanSelectionParameter::ValueType::scanNames);

  py::enum_<OEMEbsdScanSelectionParameter::EbsdReaderType> oemEbsdScanSelectionEbsdReaderType(oemEbsdScanSelectionParameter, "EbsdReaderType");
  oemEbsdScanSelectionEbsdReaderType.value("Oim", OEMEbsdScanSelectionParameter::EbsdReaderType::Oim);
  oemEbsdScanSelectionEbsdReaderType.value("Esprit", OEMEbsdScanSelectionParameter::EbsdReaderType::Esprit);

  BindParameterConstructor(h5EbsdReaderParameter);

  oemEbsdScanSelectionParameter.def(py::init<const std::string&, const std::string&, const std::string&, const OEMEbsdScanSelectionParameter::ValueType&, OEMEbsdScanSelectionParameter::EbsdReaderType,
                                             OEMEbsdScanSelectionParameter::ExtensionsType>(),
                                    "name"_a, "human_name"_a, "help_text"_a, "default_value"_a, "reader_type"_a, "extensions_type"_a);

  internals.addConversion<ReadH5EbsdFileParameter>();
  internals.addConversion<OEMEbsdScanSelectionParameter>();

  OrientationAnalysis::BindFilters(mod, internals);
}
