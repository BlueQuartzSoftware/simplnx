# Complex Python Information

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
    (nx-build) [user@host] $ conda mambabuild --python 3.8 .; conda mambabuild --python 3.9 .; conda mambabuild --python 3.10 . 

```

### Windows/Linux

```shell
    [user@host] $ cd complex/conda
    [user@host] $ conda build . 
```

For faster environment solves mamba can also be used.

```shell
conda install boa
conda mambabuild --python 3.8|3.9|3.10 .
```

### Uploading to Anaconda.org

Open a "base" anaconda prompt.

```shell
    [user@host] $ anaconda login
    [user@host] $ anaconda upload -c bluequartzsoftware [path/to/tar.bz]
```

## Using the Python Bindings

```shell
conda config --add channels conda-forge
conda config --set channel_priority strict
conda create -n cxpython python=3.8
conda activate cxpython
conda install -c bluequartzsoftware complex
```

If you plan to use jupyter notebooks, then any other kernels and such will also need to be installed. VS Code does this for you.
