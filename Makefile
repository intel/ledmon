all:
	ninja -C build

clean:
	ninja -C build clean

install:
	DESTDIR=$(DESTDIR) ninja -C build install

dist:
	ninja -C build dist

check:
	ninja -C build test
