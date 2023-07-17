#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <pico/binary_info.h>
#include <pico/stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "icsp.hpp"

static int htob() {
    int n = 0;
    char c = getchar();
    if (c >= '0' && c <= '9') n += c - '0';
    else if (c >= 'A' && c <= 'F') n += c - 'A' + 10;
    else if (c >= 'a' && c <= 'f') n += c - 'a' + 10;
    else return -1;
    n <<= 4;
    c = getchar();
    if (c >= '0' && c <= '9') n += c - '0';
    else if (c >= 'A' && c <= 'F') n += c - 'A' + 10;
    else if (c >= 'a' && c <= 'f') n += c - 'a' + 10;
    else return -1;
    return n;
}

int main() {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, true);
    stdio_init_all();
    icsp_init(spi0, 20, 19, 18, 16);
    bool init = false;
    uint16_t addr_hi = 0;
    while (true) {
        if (!init) printf("Waiting for hex file...\n");
        if (getchar() != ':') continue;
        int bc = htob();
        if (bc == -1) {
            printf("Error: Invalid format (count)\n");
            continue;
        }
        int addr_h = htob();
        if (addr_h == -1) {
            printf("Error: Invalid format (address high)\n");
            continue;
        }
        int addr = htob();
        if (addr == -1) {
            printf("Error: Invalid format (address low)\n");
            continue;
        }
        addr |= (addr_h << 8) | (addr_hi << 16);
        int rec = htob();
        if (rec == -1) {
            printf("Error: Invalid format (command)\n");
            continue;
        }
        if (!init) {
            printf("Entering LVP mode\n");
            icsp_enter_lvp();
            uint16_t id = icsp_get_device_id();
            uint16_t rev = icsp_get_revision_id();
            if ((id == 0xFFFF && rev == 0xFFFF) || (id == 0 || rev == 0)) {
                printf("Error: Could not get chip ID\n");
                icsp_exit_lvp();
                continue;
            }
            printf("Chip ID: %04x rev %04x\nErasing\n", id, rev);
            icsp_cmd_erase(ICSP_ERASE_REGION_FLASH | ICSP_ERASE_REGION_EEPROM | ICSP_ERASE_REGION_CONFIG);
            init = true;
        }
        switch (rec) {
            case 0: {
                if (addr >= 0x200000) {
                    uint8_t program[32];
                    for (int i = 0; i < bc; i++) {
                        int l = htob();
                        if (l == -1) {
                            printf("Error: Invalid format (data low)\n");
                            continue;
                        }
                        program[i] = l;
                    }
                    printf("Writing to %06X (%d bytes, EEPROM)\n", addr, bc);
                    icsp_program_page_8bit(addr, program, bc, false);
                } else {
                    uint16_t program[32];
                    bc >>= 1;
                    for (int i = 0; i < bc; i++) {
                        int l = htob();
                        if (l == -1) {
                            printf("Error: Invalid format (data low)\n");
                            continue;
                        }
                        int h = htob();
                        if (h == -1) {
                            printf("Error: Invalid format (data high)\n");
                            continue;
                        }
                        program[i] = l | (h << 8);
                    }
                    printf("Writing to %06X (%d words, flash)\n", addr, bc);
                    icsp_program_page(addr, program, bc, false);
                }
                break;
            }
            case 1: {
                printf("Programming complete; exiting LVP\n");
                icsp_exit_lvp();
                init = false;
                break;
            }
            case 4: {
                int haddr_h = htob();
                if (haddr_h == -1) {
                    printf("Error: Invalid format (data high)\n");
                    continue;
                }
                int haddr = htob();
                if (haddr == -1) {
                    printf("Error: Invalid format (data low)\n");
                    continue;
                }
                addr_hi = (haddr_h << 8) | haddr;
                break;
            }
            default: {
                printf("Error: Unknown record type %d\n", rec);
                break;
            }
        }
    }
}