@echo off

echo "complex_CONDA_EXECUTABLE: %complex_CONDA_EXECUTABLE%"
echo "complex_CONDA_ENV: %complex_CONDA_ENV%"
echo "PYTHON_TEST_FILE: %PYTHON_TEST_FILE%"
echo "PYTHONPATH: %PYTHONPATH%"

CALL "%complex_CONDA_EXECUTABLE%" activate "%complex_CONDA_ENV%"

echo "PATH: %PATH%"
echo "Where is Python: "
where python
echo "Python Version: "
python --version

python "%PYTHON_TEST_FILE%"
