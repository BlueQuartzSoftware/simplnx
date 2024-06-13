# Simplnx Python Bindings

## Directory Layout

This directory has several areas that might be of interest depending if you are an end user or
are developing new python bindings for `simplnx`.

- cmake
    This directory holds various files used by CMake during the configuration of the python bindings
- CxPybind
    This directory holds C++ implementation details for the python bindings
- docs
    This directory holds the entirety of the python binding documentation
- examples
    This directory holds python files showing basic usage of the simplnx python bindings
- notebools
    This directory holds jupyter notebooks that use the python bindings
- pipelines
    This directory holds python versions of the example pipelines that are included in the simplnx repository
- plugins
    This directry holds an example simplnx plugin written in python
- testing
    This directory holds specific files used for unit testing the python codes
- utils
    This directory holds various utilities that are needed during the build process

## Checklist when updating Python Bindings

- Update Version number.

  - If you add API then update the third number
  - If you break API (any where in simplnx), update the second number

- Document **ALL** new API in the appropriate documentation file(s)
- Create a ReleaseNotes_XXX.rst file with the appropriate highlights from the release
- Create example python code for any new API
- Update example python codes for any changed API
- Add unit test for any NEW API
- Update Unit test for changed API
- Tag the repository with the correct version in the correct semantic form

## Creating the Python Bindings

Create an Anaconda virtual environment with the following command:

```shell
    (base) [user@host] $ conda create --name nx-build python=3.10 conda-build
    (base) [user@host] $ conda activate nx-build
    (nx-build) [user@host] $ cd simplnx/conda
    (nx-build) [user@host] $ conda build .
```

### Uploading to Anaconda.org

Open a "base" anaconda prompt.

```shell
    [user@host] $ anaconda login
    [user@host] $ anaconda upload --user bluequartzsoftware [path/to/tar.bz]
```

## Using the Python Bindings

```shell
    [user@host] $ conda config --add channels conda-forge
    [user@host] $ conda config --set channel_priority strict
    [user@host] $ conda create -n cxpython python=3.10
    [user@host] $ conda activate cxpython
    [user@host] $ conda install -c bluequartzsoftware simplnx
```

If you plan to use jupyter notebooks, then any other kernels and such will also need to be installed. VS Code does this for you.

## Test Python Bindings from Build Directory

Ensure you are building the python bindings.

```shell
SIMPLNX_BUILD_PYTHON=ON
SIMPLNX_EMBED_PYTHON=OFF
SIMPLNX_BUILD_PYTHON_DOCS=ON
Python3_EXECUTABLE=/path/to/python
```

## Creating Python Documentation with Sphinx

If you are modifying the python bindings and need to update the documentation or are updating the documentation
and want to see what the final rendered HTML site looks like you will need to ensure the following python
packages are installed into your python virtual environment.

``` shell
conda create -n d3ddocs python=3.10 sphinx myst-parser sphinx-markdown-tables sphinx_rtd_theme
conda activate d3ddocs
cd simplnx/docs/
make clean
make html
```

## Restructured Text Heading Levels

=====================================
Page Title
=====================================

###################################
Part Title
###################################

Heading 1
=========

Heading 2
---------

Heading 3
^^^^^^^^^

Heading 4
""""""""""

 