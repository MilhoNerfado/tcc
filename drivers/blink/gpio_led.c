//
// Created by milho on 8/1/24.
//

#include <zephyr/sys/printk.h>

#include <app/drivers/blink.h>

void blink(void) {
    printk("Hello from blink");
}
