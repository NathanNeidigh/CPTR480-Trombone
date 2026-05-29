add_executable(trombone
	main.cpp
)

target_link_libraries(trombone
	pico_stdlib
)

pico_enable_stdio_usb(trombone 1)
pico_enable_stdio_uart(trombone 0)

pico_add_extra_outputs(trombone)
