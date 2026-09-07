example_deps = {'named_pipe': [], 'libvirt-usb-hotplug': [dependency('libvirt'), dependency('libxml-2.0'), dependency('libudev')], 'dbus-tool': [dependency('libsystemd')], 'usdev': [dependency('libnl-3.0')]}
example_links = {'named_pipe': [], 'libvirt-usb-hotplug': [], 'dbus-tool': [], 'usdev': []}
if 'udev' in get_option('modules')
  links = example_links['libvirt-usb-hotplug']
  links += [module_targets['udev']]
  example_links += {'libvirt-usb-hotplug': links}
endif
if 'sdbus' in get_option('modules')
  links = example_links['dbus-tool']
  links += [module_targets['sdbus']]
  example_links += {'dbus-tool': links}
endif
if 'netconf' in get_option('layer2_modules')
  links = example_links['usdev']
  links += [layer2_targets['netconf']]
  example_links += {'usdev': links}
endif

example_sources = {
  'libvirt-usb-hotplug': 'libvirt-usb-hotplug.c',
  'named_pipe': 'named_pipe.c',
  'dbus-tool': 'dbus-tool.c',
  'usdev': 'usdev.c',
}
example_cargs = {
  'usdev': [
    '-DLIBEXECDIR="' + join_paths(get_option('prefix'), get_option('libexecdir')) + '"',
    '-DSYSCONFDIR="' + join_paths(get_option('prefix'), get_option('sysconfdir')) + '"',
  ],
}

foreach name : get_option('examples')
  if name == 'ipquery'
    executable('ipquery', files('ipquery/main.c', 'ipquery/ipquery.c'),
      include_directories: include_directories('../src', '../include', 'ipquery'),
      link_with: core_lib,
      dependencies: [dependency('threads')],
      install: true,
      install_dir: get_option('bindir'))
  else
    executable(name, files(example_sources[name]),
      include_directories: include_directories('../src', '../include', '../layer2'),
      link_with: [core_lib] + example_links[name],
      dependencies: example_deps[name],
      c_args: example_cargs.get(name, []),
      install: true,
      install_dir: get_option('bindir'))
  endif
endforeach
