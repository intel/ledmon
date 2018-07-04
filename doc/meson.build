pod2man = find_program('pod2man', required : true)
pod2man_args = [
	'--date=February 2018'
]

man = custom_target(
	'ledmon.conf.5',
	output : 'ledmon.conf.5',
	input : 'ledmon.conf.pod',
	command : [
		pod2man,
		pod2man_args,
		'--release="LEDMON.CONF Version @0@"'.format(meson.project_version()),
		'--section=5',
		'--name=ledmon.conf',
		'--center="Intel(R) Enclosure LED Utilities Config"',
		'@INPUT@', '@OUTPUT@'
	],
	install : true,
	install_dir : join_paths(get_option('mandir'), 'man5')
)

man = custom_target(
	'ledmon.8',
	output : 'ledmon.8',
	input : 'ledmon.pod',
	command : [
		pod2man,
		pod2man_args,
		'--release="LEDMON Version @0@"'.format(meson.project_version()),
		'--section=8',
		'--name=ledmon',
		'--center="Intel(R) Enclosure LED Monitor Service"',
		'@INPUT@', '@OUTPUT@'
	],
	install : true,
	install_dir : join_paths(get_option('mandir'), 'man8')
)

man = custom_target(
	'ledctl.8',
	output : 'ledctl.8',
	input : 'ledctl.pod',
	command : [
		pod2man,
		pod2man_args,
		'--release="LEDCTL Version @0@"'.format(meson.project_version()),
		'--section=8',
		'--name=ledctl',
		'--center="Intel(R) Enclosure LED Control Application"',
		'@INPUT@', '@OUTPUT@'
	],
	install : true,
	install_dir : join_paths(get_option('mandir'), 'man8')
)
