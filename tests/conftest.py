import logging

LOGGER = logging.getLogger(__name__)

store_params = [
    ["--ledctl-binary", "src/ledctl/ledctl", "Path to the ledctl binary."],
    [
        "--slot-filters", "none",
        "List comma separated. Filter out slots starting with filter."
    ],
    [
        "--controller-filters", "none",
        "List comma separated. Filter out controllers matching by name e.g. VMD, SCSI, NPEM."
    ]
]


def pytest_addoption(parser):
    for param, def_val, help_txt in store_params:
        parser.addoption(param, action="store", default=def_val, help=help_txt)


def pytest_collection_modifyitems(session, config, items):
    for param in store_params:
        LOGGER.info(f'[CONFIG] {param[0]}: {config.getoption(param[0])}')


def pytest_generate_tests(metafunc):
    params = {
        "ledctl_binary": metafunc.config.option.ledctl_binary,
        "slot_filters": metafunc.config.option.slot_filters,
        "controller_filters": metafunc.config.option.controller_filters
    }
    for param, val in params.items():
        if param in metafunc.fixturenames and val is not None:
            metafunc.parametrize(param, [val.split(",")])
