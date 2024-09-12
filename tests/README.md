## Running tests with pytest

To run tests suite, make sure python and pytest with at least 3.11 version is installed.

## How to compile with tests enabled

Run `autogen.sh` to generate compilation configurations.
Run `./configure` with `--enable-test` to enable building unit tests and add a target for `make check`. This option requires `--enable-library` too.
Compile with `make` or `make check` when you want to run unit tests just after compilation.

## Parameters to configure

There is possibility to set:

- `--ledctl-binary` - path to ledctl,
- `--slot-filters` - comma separated list of slot filters to exclude,
- `--controller-filters` - comma separated list of controller types to exclude.


## How to run tests

There are few methods to run tests:

- run `pytest` with or without parameters to set,

```shell
$ pytest --ledctl-binary=src/ledctl/ledctl --slot-filters=sg3-,sg2- --controller-filters=VMD
```
- run `make check` which will compile the code and run tests. Logs will be placed in `./test-suite.log` file,
- run script `tests/runtests.sh`.