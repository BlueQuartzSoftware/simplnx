@echo off

:: echo "complex_CONDA_EXECUTABLE: %complex_CONDA_EXECUTABLE%"
:: echo "complex_CONDA_ENV: %complex_CONDA_ENV%"
echo "PYTHON_TEST_FILE: %PYTHON_TEST_FILE%"
echo "PYTHONPATH: %PYTHONPATH%"
echo "PYTHON_EXECUTABLE: %PYTHON_EXECUTABLE%"

:: CALL "%complex_CONDA_EXECUTABLE%" activate "%complex_CONDA_ENV%"

echo "PATH: %PATH%"
:: echo "Where is Python: "
:: where python.exe
echo "Python Version: "
%PYTHON_EXECUTABLE% --version
if %errorlevel% neq 0 exit 1

%PYTHON_EXECUTABLE% "%PYTHON_TEST_FILE%"
