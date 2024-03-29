project('raspberry-display', 'cpp', default_options : ['cpp_std=c++17'])
cpp = meson.get_compiler('cpp')

cmake = import('cmake')
mqtt_cpp = cmake.subproject('mqtt_cpp')

deps = [
    dependency('threads'),
    dependency('ncurses'),
    dependency('boost', modules: ['system']),
    cpp.find_library('wiringPi', required : true),
    mqtt_cpp.dependency('mqtt_cpp_iface'),
]

common_src = ['src/timer.cpp', 'src/display.cpp', 'src/font.cpp']

mock_display_src = ['src/mock-display.cpp', common_src]
real_display_src = ['src/ht1632.cpp', common_src]

websocket_src = ['src/websocket.cpp']
curses_client_src = ['src/curses-client.cpp']
mqtt_client_src = ['src/mqtt-client.cpp']
    
executable('raspberry-display', websocket_src, real_display_src, dependencies: deps, install: true)
executable('mock-raspberry-display', websocket_src, mock_display_src, dependencies: deps)
executable('curses-client', curses_client_src, real_display_src, dependencies: deps)
executable('mock-curses-client', curses_client_src, mock_display_src, dependencies: deps)
executable('mock-mqtt-client', mqtt_client_src, mock_display_src, dependencies: deps)

