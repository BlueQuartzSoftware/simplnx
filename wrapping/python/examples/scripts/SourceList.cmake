
set(PYTHON_TEST_INPUT_DIR "${simplnx_SOURCE_DIR}/wrapping/python/examples/scripts")

set(SIMPLNX_PYTHON_TESTS
  "angle_conversion" 
  "basic_arrays" 
  "basic_ebsd_ipf"
  "basic_numpy" 
  "create_ensemble_info" 
  "generated_file_list" 
  "geometry_examples" 
  "import_d3d"      # Dependent on 'basic_ebsd_ipf' running first
  "import_hdf5"     # Dependent on 'basic_ebsd_ipf' running first
  "output_file" 
  "pipeline" 
  "read_csv_file"
#  "read_esprit_data"
)

CreatePythonTests(PREFIX "PY_SIMPLNX"
  INPUT_DIR ${PYTHON_TEST_INPUT_DIR}
  TEST_NAMES ${SIMPLNX_PYTHON_TESTS}
)


