@echo off


echo "PYTHON_TEST_FILE: %PYTHON_TEST_FILE%"
echo "PYTHONPATH: %PYTHONPATH%"
echo "Python3_EXECUTABLE: %Python3_EXECUTABLE%"

echo "PATH: %PATH%"
echo "Where is Python: "
where python.exe
echo "Python Version: "
%Python3_EXECUTABLE% --version

%Python3_EXECUTABLE% "%PYTHON_TEST_FILE%"

