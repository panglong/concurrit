# Download #

We do not have any release yet. Just download the source using the following command and follow the installation instructions below:

`git clone https://tayfunelmas@code.google.com/p/concurrit/`

# Install #

Assume that CONCURRIT is installed at the following directory: `$HOME/concurrit`

Follow the steps below to install CONCURRIT:
  1. Edit file `$HOME/concurrit/scripts/export_vars.sh` to set three variables: `CONCURRIT_HOME`, `BOOST_ROOT`, and (if exists) `RADBENCH_HOME`
  1. Add the following line to your login bash script (e.g., ~/.profile):
> > `source $HOME/concurrit/scripts/export_vars.sh`
  1. `cd $CONCURRIT_HOME` and run `./scripts/install_concurrit.sh`
> > This will download (requires an Internet connection) and install necessary libraries, and finally compile CONCURRIT.

# Check installation #

To check if the installation is successful, do the following in `$CONCURRIT_HOME`:
  1. `./scripts/compile_bench.sh bbuf`
  1. `./scripts/run_bench.sh bbuf`
You should see a deadlock.

# Directory structure #

  * `backup`: Some old files. Omit this directory.
  * `bench`: Root for benchmark directories.
  * `bin, lib, obj`: Created during the compilation to store the executables, shared libraries, and object files.
  * `dummy`: Sources for a dummy shared library defining empty bodies for the instrumentation functions. Used for compiling the program under test without referring to CONCURRIT libraries.
  * `include`: Header files for CONCURRIT.
  * `src`: Source files for CONCURRIT.
  * `pintool`: The Pin tool for runtime binary instrumentation.
  * `python`: An older Python implementation of CONCURRIT.
  * `remote`: Source files for system testing of large programs.
  * `third_party`: Root directory for third-party libraries/tools.
  * `work`: A temporary working directory used when running benchmarks.