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

    // Use a buffered block device to allow arbitrary length writes to the underlying BD
    BlockDevice *secondary_bd = get_secondary_bd();
    BufferedBlockDevice bufferedSecBD(secondary_bd);
    int ret = bufferedSecBD.init();
    if (ret == 0) {
        tr_info("Secondary BlockDevice inited");
    } else {
        tr_error("Cannot init secondary BlockDevice: %d", ret);
    }

    // Erase secondary slot
    // On the first boot, the secondary BlockDevice needs to be clean
    // If the first boot is not normal, please run the erase step, then reboot

    tr_info("> Press button to erase secondary slot");

    while(!DEMO_BUTTON_IS_PRESSED) {
        ThisThread::sleep_for(10ms);
    }

    tr_info("Erasing secondary BlockDevice...");
    ret = bufferedSecBD.erase(0, bufferedSecBD.size());
    if (ret == 0) {
        tr_info("Secondary BlockDevice erased");
    } else {
        tr_error("Cannot erase secondary BlockDevice: %d", ret);
    }

    tr_info("> Press button to copy update image to secondary BlockDevice");

    while(!DEMO_BUTTON_IS_PRESSED) {
        ThisThread::sleep_for(10ms);
    }

    // Copy the update image from internal flash to secondary BlockDevice
    bufferedSecBD.program(&_binary_SimpleApp_update_image_bin_start, 0, SimpleApp_update_image_bin_length);

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
