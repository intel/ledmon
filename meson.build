project('ledmon', 'c',
	default_options : [
		'c_std=c11',
		'warning_level=1',
	],
	license : 'GPL2',
	version : '0.91',
	meson_version : '>= 0.47.0',
)

cc = meson.get_compiler('c')

add_project_arguments('-include', 'build_config.h', language : 'c')
config_h = configuration_data()

# Use all features.  See features.h for details.
config_h.set('_GNU_SOURCE', 1)

# Version strings.
version_array = meson.project_version().split('.')
config_h.set('VERSION_MAJOR', version_array[0])
config_h.set('VERSION_MINOR', version_array[1])

if get_option('debug_ibpi')
	config_h.set('DEBUG_IBPI', 1)
endif

if get_option('use_dmalloc')
	config_h.set('_HAVE_DMALLOC_H', 1)
endif

configure_file(
	output : 'build_config.h',
	configuration : config_h
)

subdir ('doc')
subdir ('src')
subdir ('tests')
