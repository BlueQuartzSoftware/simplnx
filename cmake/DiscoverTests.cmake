include_guard(GLOBAL)

# Register every visible test once. Only OOC-enabled builds split discovery to label
# cases tagged [OOC] (Catch2 tags ignore case) for `ctest -L "^ooc$"`.
function(simplnx_discover_tests UNIT_TEST_TARGET)
  if(DREAM3DNX_ENABLE_OOC)
    catch_discover_tests(${UNIT_TEST_TARGET} TEST_SPEC "~[OOC]~[.]")
    catch_discover_tests(${UNIT_TEST_TARGET} TEST_SPEC "[OOC]~[.]" TEST_LIST ${UNIT_TEST_TARGET}_OOC_TESTS PROPERTIES LABELS "ooc")
  else()
    catch_discover_tests(${UNIT_TEST_TARGET} TEST_SPEC "~[.]")
  endif()
endfunction()
