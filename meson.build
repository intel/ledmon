project('ledmon', 'c',
	default_options : [
		'c_std=c11',
		'warning_level=3',
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

prefixdir = get_option('prefix')
if not prefixdir.startswith('/')
	error('Prefix is not absolute: "@0@"'.format(prefixdir))
endif

config_paths = [
	['LEDCTL_DEF_LOG_FILE', 'ledctl_log_path'],
	['LEDMON_DEF_LOG_FILE', 'ledmon_log_path'],
	['LEDMON_DEF_CONF_FILE', 'ledmon_conf_path'],
	['LEDMON_PID_PATH', 'pid_dir_path']
]

foreach cp : config_paths
	file_path = get_option(cp[1])
	if file_path.startswith('/')
		config_h.set_quoted(cp[0], file_path)
	else
		config_h.set_quoted(cp[0], join_paths(prefixdir, file_path))
	endif
endforeach

config_h.set('LEDMON_DEF_SLEEP_INTERVAL', get_option('ledmon_def_sleep_interval'))
config_h.set('LEDMON_MIN_SLEEP_INTERVAL', get_option('ledmon_min_sleep_interval'))

configure_file(
	output : 'build_config.h',
	configuration : config_h
)

subdir ('doc')
subdir ('src')
subdir ('tests')
