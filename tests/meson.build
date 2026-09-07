test_sources = {
  'fork': ['fork.c'],
  'libnl': ['libnl.c'],
  'nl_route_raw': ['nl_route_raw.c'],
  'popen': ['popen.c'],
  'sd_journal': ['sd_journal.c'],
  'udev': ['udev.c'],
  'fanotify-req-perm': ['fanotify-req-perm.c'],
  'fanotify-fs-error': ['fanotify-fs-error.c'],
}
test_deps = {
  'libnl': [dependency('libnl-3.0')],
  'sd_journal': [dependency('libsystemd')],
  'udev': [dependency('libudev')],
}
test_links = {
  'libnl': [module_targets['libnl']],
  'sd_journal': [module_targets['sd_journal']],
  'udev': [module_targets['udev']],
}

foreach name : get_option('tests')
  sources = test_sources[name]
  executable(name + '.test', files(sources),
    include_directories: include_directories('../src'),
    link_with: [core_lib] + test_links.get(name, []),
    dependencies: test_deps.get(name, []),
    c_args: ['-DCRTX_TEST'],
    install: false
  )
endforeach
