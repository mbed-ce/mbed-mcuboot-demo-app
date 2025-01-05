/*
 * Copyright (c) 2024 Jamie Smith
 * SPDX-License-Identifier: Apache-2.0
 *
 * The Simple App is an application which simply has the ability to set the
 * update complete flag, and otherwise just blinks the LED and prints messages.
 */

#include "mbed.h"
#include "mbed-trace/mbed_trace.h"

#include "bootutil/bootutil.h"

#define TRACE_GROUP "SimpleApp"

#if DEMO_BUTTON_ACTIVE_LOW
#define DEMO_BUTTON_IS_PRESSED !btn
#else
#define DEMO_BUTTON_IS_PRESSED btn
#endif

int main()
{
    // Enable traces from relevant trace groups
    mbed_trace_init();
    mbed_trace_include_filters_set("SimpleApp,MCUb,BL");

    DigitalIn btn(DEMO_BUTTON);

#ifdef LED1
    DigitalOut led(LED1);
#endif

    // Check if an update has been performed
    int swap_type = boot_swap_type();
    int ret;
    switch (swap_type)
    {
        case BOOT_SWAP_TYPE_NONE:
            tr_info("Regular boot");
            break;

        // Note: Older mcuboot versions appear to use SWAP_TYPE_REVERT.
        // Newer ones seem to use SWAP_TYPE_TEST.
        case BOOT_SWAP_TYPE_TEST:
        case BOOT_SWAP_TYPE_REVERT:
            // After MCUboot has swapped a (non-permanent) update image
            // into the primary slot, it defaults to reverting the image on the NEXT boot.
            // This is why we see "[INFO][MCUb]: Swap type: revert" which can be misleading.
            // Confirming the CURRENT boot dismisses the reverting.
            tr_info("Firmware update applied successfully");

            // Do whatever is needed to verify the firmware is okay (eg: self test)
            // then mark the update as successful. Here we let the user press a button.
            tr_info("Press the button to confirm, or reboot to revert the update");

            while(!DEMO_BUTTON_IS_PRESSED)
            {
                ThisThread::sleep_for(10ms);
            }

            ret = boot_set_confirmed();
            if (ret == 0)
            {
                tr_info("Current firmware set as confirmed");
            }
            else
            {
                tr_error("Failed to confirm the firmware: %d", ret);
            }
            break;

            // Note: Below are intermediate states of MCUboot and
            // should never reach the application...
        case BOOT_SWAP_TYPE_FAIL:   // Unable to boot due to invalid image
        case BOOT_SWAP_TYPE_PERM:   // Permanent update requested (when signing the image) and to be performed
        case BOOT_SWAP_TYPE_PANIC:  // Unrecoverable error
        default:
            tr_error("Unexpected swap type: %d", swap_type);
    }

    while(true)
    {
        ThisThread::sleep_for(1s);
        printf("Simple app is running...\n");

#ifdef LED1
        led = 1;
        ThisThread::sleep_for(250ms);
        led = 0;
        ThisThread::sleep_for(250ms);
        led = 1;
        ThisThread::sleep_for(250ms);
        led = 0;
#endif
    }
}