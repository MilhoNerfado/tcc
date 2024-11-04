zephyr_library()
zephyr_library_sources(modules.c)
zephyr_linker_sources(SECTIONS iterables.ld)