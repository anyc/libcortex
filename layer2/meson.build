layer2_inc = include_directories('.', '../src', '../include')
layer2_links = {'ping': []}
layer2_deps = {'ping': []}
if 'evdev' in get_option('modules')
  layer2_links += {'dynamic_evdev': [module_targets['evdev']]}
  layer2_deps += {'dynamic_evdev': [dependency('libevdev'), dependency('libudev')]}
endif
if 'curl' in get_option('modules')
  layer2_links += {'forked_curl': [module_targets['curl']]}
  layer2_deps += {'forked_curl': []}
endif
if 'libnl' in get_option('modules')
  layer2_links += {'netif': [module_targets['libnl']], 'netconf': [module_targets['libnl']]}
  layer2_deps += {'netif': [dependency('libnl-3.0')], 'netconf': [dependency('libnl-3.0')]}
endif

install_headers(files('dynamic_evdev.h', 'forked_curl.h', 'netconf.h',
  'netif.h', 'ping.h', 'wifi.h'), subdir: 'cortex/layer2')

layer2_targets = {}
foreach name : get_option('layer2_modules')
  layer2_targets += {name: shared_library('crtx_' + name, files(name + '.c'),
    include_directories: layer2_inc,
    link_with: [core_lib] + layer2_links[name],
    dependencies: layer2_deps[name],
    name_prefix: 'lib',
    version: '0.7.0',
    soversion: '0.7',
    install: true,
    install_dir: libdir
  )}
endforeach
