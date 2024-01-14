@echo off

:: echo "complex_CONDA_EXECUTABLE: %complex_CONDA_EXECUTABLE%"
:: echo "complex_CONDA_ENV: %complex_CONDA_ENV%"
echo "PYTHON_TEST_FILE: %PYTHON_TEST_FILE%"
echo "PYTHONPATH: %PYTHONPATH%"
echo "Python3_EXECUTABLE: %Python3_EXECUTABLE%"

:: CALL "%complex_CONDA_EXECUTABLE%" activate "%complex_CONDA_ENV%"

echo "PATH: %PATH%"
:: echo "Where is Python: "
:: where python.exe
echo "Python Version: "
%Python3_EXECUTABLE% --version

%Python3_EXECUTABLE% "%PYTHON_TEST_FILE%"
