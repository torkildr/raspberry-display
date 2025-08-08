project('raspberry-display', 'cpp', 
    default_options : [
        'cpp_std=c++17',
        'warning_level=3'
    ]
)
cpp = meson.get_compiler('cpp')

# Strong warning and static analysis flags for our code only
# These will be applied per-target to avoid affecting third-party libraries
our_warning_flags = []
if cpp.get_id() == 'gcc' or cpp.get_id() == 'clang'
    our_warning_flags = [
        '-Wpedantic',
        '-Wconversion',
        '-Wsign-conversion',
        '-Wunused',
        '-Wuninitialized',
        '-Wundef',
        '-Wcast-align',
        '-Wcast-qual',
        '-Wold-style-cast',
        '-Woverloaded-virtual',
        '-Wmissing-declarations',
        '-Wredundant-decls',
        '-Wdouble-promotion',
        '-Wformat=2',
        '-Wformat-security',
        '-Wduplicated-cond',
        '-Wduplicated-branches',
        '-Wlogical-op',
        '-Wrestrict'
    ]
endif

# Import MQTT library
cmake = import('cmake')
mqtt_cpp = cmake.subproject('mqtt_cpp')

# Common dependencies
common_deps = [
    dependency('threads'),  # Used by timer.cpp and mqtt-client.cpp
]

# MQTT client dependencies
mqtt_deps = common_deps + [
    dependency('boost', modules: ['system']),  # Required by MQTT library (boost::asio)
    mqtt_cpp.dependency('mqtt_cpp_iface'),     # MQTT client library
]

# Curses client dependencies (includes ncurses for mock display)
curses_deps = common_deps + [
    dependency('ncurses'),  # Used by curses-client.cpp and mock-display.cpp
]

# Real hardware dependencies
hardware_deps = [
    cpp.find_library('wiringPi', required : true),  # Used by ht1632.cpp
]

# Source file groups
common_src = ['src/timer.cpp', 'src/display.cpp', 'src/font.cpp']
mock_display_src = ['src/mock-display.cpp'] + common_src
real_display_src = ['src/ht1632.cpp'] + common_src

curses_client_src = ['src/curses-client.cpp']
mqtt_client_src = ['src/mqtt-client.cpp']

# Executables with strict warnings applied only to our source files
executable('raspberry-display-mqtt', 
    mqtt_client_src + real_display_src, 
    dependencies: mqtt_deps + hardware_deps,
    cpp_args: our_warning_flags,
    install: true
)

executable('curses-client', 
    curses_client_src + real_display_src, 
    dependencies: curses_deps + hardware_deps,
    cpp_args: our_warning_flags
)

executable('mock-display-mqtt', 
    mqtt_client_src + mock_display_src, 
    dependencies: mqtt_deps + curses_deps,  # Needs ncurses for mock display
    cpp_args: our_warning_flags
)

executable('mock-curses-client', 
    curses_client_src + mock_display_src, 
    dependencies: curses_deps,
    cpp_args: our_warning_flags
)
