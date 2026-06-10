add_executable(trombone
	main.cpp
	dac.cpp
	HC-SR04.cpp
	encoder.cpp
)

pico_generate_pio_header(trombone
    ${CMAKE_CURRENT_LIST_DIR}/i2s.pio
)

target_link_libraries(trombone
	pico_stdlib
	pico_multicore
	hardware_gpio
	hardware_pio
)

target_include_directories(trombone PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)

pico_enable_stdio_usb(trombone 1)
pico_enable_stdio_uart(trombone 0)

pico_add_extra_outputs(trombone)
