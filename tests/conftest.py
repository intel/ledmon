import logging

LOGGER = logging.getLogger(__name__)

def pytest_addoption(parser):
    parser.addoption("--ledctl-binary", action="store", default="src/ledctl/ledctl")
    parser.addoption("--slot-filters", action="store", default="none")

def pytest_collection_modifyitems(session, config, items):
    LOGGER.info(f'[CONFIG] ledctl binary: {config.getoption("--ledctl-binary")}')
    LOGGER.info(f'[CONFIG] slot filters: {config.getoption("--slot-filters")}')

def pytest_generate_tests(metafunc):
        ledctl_option_value = metafunc.config.option.ledctl_binary
        if 'ledctl_binary' in metafunc.fixturenames and ledctl_option_value is not None:
                metafunc.parametrize("ledctl_binary", [ledctl_option_value])

        slotfilters_option_value = metafunc.config.option.slot_filters
        if 'slot_filters' in metafunc.fixturenames and slotfilters_option_value is not None:
                metafunc.parametrize("slot_filters", [slotfilters_option_value.split(",")])
