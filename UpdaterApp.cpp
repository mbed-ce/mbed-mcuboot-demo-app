/*
 * Copyright (c) 2020 Embedded Planet
 * Copyright (c) 2020 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"

#include "bootutil/bootutil.h"
#include "bootutil/image.h"
#include "flash_map_backend/secondary_bd.h"

#include "FlashIAP/FlashIAPBlockDevice.h"
#include "blockdevice/BufferedBlockDevice.h"
#include "drivers/InterruptIn.h"

#define TRACE_GROUP "UpdaterApp"
#include "mbed-trace/mbed_trace.h"

#if DEMO_BUTTON_ACTIVE_LOW
#define DEMO_BUTTON_IS_PRESSED !btn
#else
#define DEMO_BUTTON_IS_PRESSED btn
#endif

// SimpleApp.bin gets loaded into ram between these addresses.
// See CMakeLists.txt for details on how this is done.
extern "C" uint8_t _binary_SimpleApp_update_image_bin_start;
extern "C" uint8_t _binary_SimpleApp_update_image_bin_end;
const size_t SimpleApp_update_image_bin_length = &_binary_SimpleApp_update_image_bin_end - &_binary_SimpleApp_update_image_bin_start;

int main()
{
    // Enable traces from relevant trace groups
    mbed_trace_init();
    mbed_trace_include_filters_set("UpdaterApp,MCUb,BL");

    DigitalIn btn(DEMO_BUTTON);

    BlockDevice *secondary_bd = get_secondary_bd();
    int ret = secondary_bd->init();
    if (ret == 0) {
        tr_info("Secondary BlockDevice inited");
    } else {
        tr_error("Cannot init secondary BlockDevice: %d", ret);
    }

    tr_info(" ");
    tr_info("mcuboot configuration: -----------------------------------");
    tr_info("- Primary slot: start address 0x%x, size %ukiB", MCUBOOT_PRIMARY_SLOT_START_ADDR, MCUBOOT_SLOT_SIZE/1024);
    tr_info("  - Header size: 0x%x", MCUBOOT_HEADER_SIZE);
    tr_info("  - Max flash sectors per slot: %u", MCUBOOT_MAX_IMG_SECTORS);
    tr_info("- Scratch area: start address 0x%x, size %ukiB", MCUBOOT_SCRATCH_START_ADDR, MCUBOOT_SCRATCH_SIZE/1024);
    tr_info("- Secondary BD: size %" PRIu64 "kiB", secondary_bd->size()/1024);
    tr_info("  - Program (page) size %" PRIu64 " bytes", secondary_bd->get_program_size());
    tr_info("  - Erase (sector) size %" PRIu64 " bytes", secondary_bd->get_erase_size());
    tr_info(" ");

    // Erase secondary slot
    // On the first boot, the secondary BlockDevice needs to be clean
    // If the first boot is not normal, please run the erase step, then reboot

    tr_info("> Press button to erase secondary slot");

    while(!DEMO_BUTTON_IS_PRESSED) {
        ThisThread::sleep_for(10ms);
    }

    tr_info("Erasing secondary BlockDevice...");
    ret = secondary_bd->erase(0, secondary_bd->size());
    if (ret == 0) {
        tr_info("Secondary BlockDevice erased");
    } else {
        tr_error("Cannot erase secondary BlockDevice: %d", ret);
    }

    // Workaround: for block devices such as MicroSD cards where there is no fixed erase value,
    // mcuboot will think that the "magic" region on the secondary BD, which it uses to store
    // state info, is corrupt instead of simply not written yet.
    // To fix this, we need to actually fill the last 40 bytes with 0xFFs.
    if(secondary_bd->get_erase_value() == -1)
    {
        const std::vector<uint8_t> writeBuffer(40, 0xFF);
        secondary_bd->program(writeBuffer.data(), secondary_bd->size() - 40, 40);
    }

    tr_info("> Press button to copy update image to secondary BlockDevice");

    while(!DEMO_BUTTON_IS_PRESSED) {
        ThisThread::sleep_for(10ms);
    }

    // Copy the update image from internal flash to secondary BlockDevice
    ret = secondary_bd->program(&_binary_SimpleApp_update_image_bin_start, 0, SimpleApp_update_image_bin_length);
    if (ret == 0) {
        tr_info("Image copied.");
    } else {
        tr_error("Cannot copy image: %d", ret);
    }

    ret = secondary_bd->deinit();
    if (ret == 0) {
        tr_info("Secondary BlockDevice deinited");
    } else {
        tr_error("Cannot deinit secondary BlockDevice: %d", ret);
    }

    // Give user time to react, as programming can be pretty fast
    ThisThread::sleep_for(250ms);

    // Activate the image in the secondary BlockDevice
    tr_info("> Image copied to secondary BlockDevice, press button to activate");

    while(!DEMO_BUTTON_IS_PRESSED) {
        ThisThread::sleep_for(10ms);
    }

    ret = boot_set_pending(false);
    if (ret == 0) {
        tr_info("> Secondary image pending, reboot to update");
    } else {
        tr_error("Failed to set secondary image pending: %d", ret);
    }
}
