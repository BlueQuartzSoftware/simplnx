# Complex Python Information

## Checklist when updating Python Bindings

- Update Version number.

  - If you add API then update the third number
  - If you break API (any where in complex), update the second number

- Document **ALL** new API in the appropriate documentation file(s)
- Create a ReleaseNotes_1XX.rst file with the appropriate highlights from the release
- Create example python code for any new API
- Update example python codes for any changed API
- Add unit test for any NEW API
- Update Unit test for changed API
- Tag the repository with the correct version in the correct semantic form

## Creating the Python Bindings

### MacOS: Use Mamba

- install mamba-forge

[https://mamba.readthedocs.io/en/latest/mamba-installation.html](https://mamba.readthedocs.io/en/latest/mamba-installation.html)
[https://github.com/conda-forge/miniforge#mambaforge](https://github.com/conda-forge/miniforge#mambaforge)

You want to install the "Mambaforge-MacOSX-x86_64" or "Mambaforge-MacOSX-arm64" package.
I elected to install into `/opt/local/mambaforge`

once you get that installed do:

```shell
    [user@host] $ /opt/local/mambaforge/bin/mamba init
```

which will edit your .zshrc file.

RELAUNCH A NEW TERMINAL!!!!

```shell
    [user@host] $ mamba create -n nx-build python=3.10
    [user@host] $ mamba activate nx-build
    [user@host] $ mamba install boa
```

Create the package from the `complex` sources

```shell
    [user@host] $ cd complex/conda
    (nx-build) [user@host] $ conda mambabuild --python 3.8 .
    (nx-build) [user@host] $ conda mambabuild --python 3.9 .
    (nx-build) [user@host] $ conda mambabuild --python 3.10 . 

```

### Windows/Linux

```shell
    [user@host] $ conda create on nx-build python=3.10 mamba boa
    [user@host] $ cd complex/conda
    [user@host] $ conda build . 
```

For faster environment solves mamba can also be used.

```shell
    [user@host] $ conda install boa
    [user@host] $ conda mambabuild --python 3.8 .
    [user@host] $ conda mambabuild --python 3.9 .
    [user@host] $ conda mambabuild --python 3.10 .
```

### Uploading to Anaconda.org

Open a "base" anaconda prompt.

```shell
    [user@host] $ anaconda login
    [user@host] $ anaconda upload --user bluequartzsoftware [path/to/tar.bz]
```

## Using the Python Bindings

```shell
conda config --add channels conda-forge
conda config --set channel_priority strict
conda create -n cxpython python=3.10
conda activate cxpython
conda install -c bluequartzsoftware complex
```

If you plan to use jupyter notebooks, then any other kernels and such will also need to be installed. VS Code does this for you.

## Test Python Bindings from Build Directory

Ensure you are building the python bindings.

```shell
COMPLEX_BUILD_PYTHON=ON
COMPLEX_EMBED_PYTHON=OFF
COMPLEX_ENABLE_SPHINX_DOCS=ON
Python3_EXECUTABLE=/path/to/python
```
