cmake_minimum_required(VERSION 3.19)
cmake_policy(VERSION 3.19)

set(MBED_APP_JSON_PATH mbed_app.json5)

include(mbed-os/tools/cmake/mbed_toolchain_setup.cmake)

project(mbed-mcuboot-demo-app VERSION 1.0.0)

include(mbed_project_setup)

# Make sure that the build system generates both bin and hex files, regardless of
# target settings.
set(MBED_OUTPUT_EXT "" CACHE STRING "" FORCE)

add_subdirectory(mbed-os)

# Compile mcuboot sources
add_subdirectory(mcuboot/boot/bootutil)
add_subdirectory(mcuboot/boot/mbed)

add_executable(SimpleApp SimpleApp.cpp secondary_bd.cpp)
target_link_libraries(SimpleApp
	mbed-baremetal
	mbed-storage
	mbed-mcuboot)
mbed_set_post_build(SimpleApp)
mcuboot_generate_update_image(SimpleApp)

add_executable(UpdaterApp UpdaterApp.cpp secondary_bd.cpp)
target_link_libraries(UpdaterApp
	mbed-baremetal
	mbed-storage
	mbed-mcuboot)
mbed_set_post_build(UpdaterApp)
mcuboot_generate_initial_image(UpdaterApp)
mcuboot_generate_update_image(UpdaterApp)

# Time for a bit of CMake magic: we take the bin file for SimpleApp, then link it into
# UpdaterApp as block of constant data.  The name in code is based on the filename passed
# here to the linker.
# See here for more details on this: https://gareus.org/wiki/embedding_resources_in_executables
target_link_options(UpdaterApp PRIVATE -Wl,-b,binary,SimpleApp-update-image.bin -Wl,-b,elf32-littlearm)
add_dependencies(UpdaterApp SimpleApp) # Ensure SimpleApp gets built before UpdaterApp