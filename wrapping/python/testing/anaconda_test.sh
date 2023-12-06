#!/bin/bash

# echo "complex_ANACONDA_DIR: $complex_ANACONDA_DIR"
# echo "complex_CONDA_ENV: $complex_CONDA_ENV"
echo "PYTHON_TEST_FILE: $PYTHON_TEST_FILE"
echo "PYTHONPATH: $PYTHONPATH"

# echo "Sourcing $complex_ANACONDA_DIR/etc/profile.d/conda.sh"
# source "$complex_ANACONDA_DIR"/etc/profile.d/conda.sh

# echo "Activating Conda environment $complex_CONDA_ENV"
# conda activate "$complex_CONDA_ENV"

echo "PATH: $PATH"
echo "Where is Python: " `which python`
# echo "Where is Conda: " `which conda`
# PYTHONEXE=$complex_ANACONDA_DIR/envs/$complex_CONDA_ENV/bin/python
PYTHONEXE=`which python`
echo "Python Version: " `${PYTHONEXE}  --version`

${PYTHONEXE} "$PYTHON_TEST_FILE"
