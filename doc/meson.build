pod2man = find_program('pod2man', required : true)
pod2man_args = [
	'--date=February 2018'
]

manuals = [
	['ledmon.conf', '5', 'Utilities Config'],
	['ledmon', '8', 'Monitor Service'],
	['ledctl', '8', 'Control Application']
]

foreach m : manuals
	man = custom_target(
		m[0] + '.' + m[1],
		output : m[0] + '.' + m[1],
		input :  m[0] + '.pod',
		command : [
			pod2man,
			pod2man_args,
			'--release="' + m[0].to_upper() +
				' Version @0@"'.format(meson.project_version()),
			'--section=' + m[1],
			'--name=' + m[0],
			'--center="Intel(R) Enclosure LED ' + m[2] + '"',
			'@INPUT@', '@OUTPUT@'
		],
		install : true,
		install_dir : join_paths(get_option('mandir'), 'man' + m[1])
	)
endforeach
