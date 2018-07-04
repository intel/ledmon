udev_dep = dependency('libudev')
sgutils2_dep = cc.find_library('sgutils2')

shared_sources = files('''
	ahci.c
	ahci.h
	block.c
	block.h
	cntrl.c
	cntrl.h
	config.h
	config_file.c
	config_file.h
	dellssd.c
	dellssd.h
	enclosure.c
	enclosure.h
	list.c
	list.h
	pci_slot.c
	pci_slot.h
	raid.c
	raid.h
	scsi.c
	scsi.h
	slave.c
	slave.h
	smp.c
	smp.h
	status.c
	status.h
	sysfs.c
	sysfs.h
	udev.c
	udev.h
	utils.c
	utils.h
	vmdssd.c
	vmdssd.h
'''.split())

executable('ledctl', [shared_sources, 'ledctl.c'],
	dependencies : [ udev_dep, sgutils2_dep ],
	install : true,
	install_dir : get_option('sbindir')
)

executable('ledmon', [shared_sources, 'pidfile.c', 'pidfile.h', 'ledmon.c'],
	dependencies : [ udev_dep, sgutils2_dep ],
	install : true,
	install_dir : get_option('sbindir')
)

conf_test = executable('ledmon_conf_test', [shared_sources, 'ledmon_conf_test.c'],
	dependencies : [ udev_dep, sgutils2_dep ],
	install : false
)
