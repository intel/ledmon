test_scripts = '''
	empty_file
	valid_options
	invalid_options
'''.split()

env = environment()
env.set('top_srcdir', meson.source_root())
env.set('PATH', meson.build_root() + '/src:/usr/bin:/bin')

foreach t : test_scripts
	exec = find_program([ t, meson.current_source_dir() ], required : true)
	test(t, exec, env : env, timeout : 1)
endforeach
