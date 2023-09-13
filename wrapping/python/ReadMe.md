# Complex Python Information


## Creating the Python Bindings

### MacOS: Use Mamba

- install mamba-forge

[https://mamba.readthedocs.io/en/latest/mamba-installation.html](https://mamba.readthedocs.io/en/latest/mamba-installation.html)
[https://github.com/conda-forge/miniforge#mambaforge](https://github.com/conda-forge/miniforge#mambaforge)

You want to install the "Mambaforge-MacOSX-x86_64" or "Mambaforge-MacOSX-arm64" package.
I elected to install into `/opt/local/mambaforge`

once you get that installed do:

```
/opt/local/mambaforge/bin/mamba init
```
which will edit your .zshrc file.

RELAUNCH A NEW TERMINAL!!!!

```
mamba create -n nx-build python=3.8
mamba activate nx-build
mamba install boa
```

Create the package from the `complex` sources

    [user@host] $ cd complex/conda
    [user@host] $ boa build .

### Windows/Linux:

    [user@host] $ cd complex/conda
    [user@host] $ conda build . 

For faster environment solves mamba can also be used. 

```
conda install boa
conda mambabuild --python 3.8|3.9|3.10 .
```

### Uploading to Anaconda.org

    [user@host] $ conda login
    [user@host] $ conda upload -c bluequartzsoftware [path/to/tar.bz]

## Using the Python Bindings


```
conda config --add channels conda-forge
conda config --set channel_priority strict
conda create -n cxpython python=3.8
conda activate cxpython
conda install -c bluequartzsoftware complex
```

If you plan to use jupyter notebooks, then any other kernels and such will also need to be installed. VS Code does this for you.

