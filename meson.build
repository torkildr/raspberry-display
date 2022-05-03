project('raspberry-display', 'cpp', default_options : ['cpp_std=c++17'])
cpp = meson.get_compiler('cpp')

common_src = ['src/timer.cpp', 'src/display.cpp', 'src/font.cpp']
mock_display_src = ['src/display.cpp', 'src/mock-display.cpp', common_src]
real_display_src = ['src/display.cpp', 'src/ht1632.cpp', common_src]

deps = [
    dependency('threads'),
    dependency('ncurses'),
    dependency('boost', modules: ['system']),
    cpp.find_library('wiringPi', required : true),
]

websocket_src = ['src/websocket.cpp']
curses_client_src = ['src/curses-client.cpp']
    
executable('raspberry-display', websocket_src, real_display_src, dependencies: deps, install: true)
executable('mock-raspberry-display', websocket_src, mock_display_src, dependencies: deps)
executable('curses-client', curses_client_src, real_display_src, dependencies: deps)
executable('mock-curses-client', curses_client_src, mock_display_src, dependencies: deps)
