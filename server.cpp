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

static const char * chipIDs[] = {
    // Starts at 0x7500, shifted right 5 bits
    // 0x74E0 (PIC18F15Q41) handled separately
    "PIC18F05Q41", // 0x7500 (PIC18F05Q41)
    "PIC18F14Q41", // 0x7520 (PIC18F14Q41)
    "PIC18F04Q41", // 0x7540 (PIC18F04Q41)
    "PIC18F16Q41", // 0x7560 (PIC18F16Q41)
    "PIC18F06Q41", // 0x7580 (PIC18F06Q41)
    "PIC16F16Q40", // 0x75A0 (PIC16F16Q40)
    "PIC18F06Q40", // 0x75C0 (PIC18F06Q40)
    "PIC18F15Q40", // 0x75E0 (PIC18F15Q40)
    "PIC18F05Q40", // 0x7600 (PIC18F05Q40)
    "PIC18F14Q40", // 0x7620 (PIC18F14Q40)
    "PIC18F04Q40"  // 0x7640 (PIC18F04Q40)
};

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
            const char * name;
            if ((id & 0xFC1F) != 0x7400) name = "Unknown";
            else if (id == 0x74E0) name = "PIC18F15Q41";
            else if (id < 0x7500 || id > 0x7640) name = "Unknown";
            else name = chipIDs[((id - 0x7500) >> 5) & 0x1F];
            printf("Chip ID: %s rev %c%d (%04x %04x)\nErasing\n", name, ((rev >> 6) & 0x1F) + 65, rev & 0x1F, id, rev);
            icsp_cmd_erase(ICSP_ERASE_REGION_FLASH | ICSP_ERASE_REGION_EEPROM | ICSP_ERASE_REGION_CONFIG);
            init = true;
            addr_hi = 0;
            addr &= 0xFFFF;
            gpio_put(PICO_DEFAULT_LED_PIN, true);
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
                    printf("Writing to %06X (%d bytes, EEPROM)... ", addr, bc);
                    printf("%d bytes written\n", icsp_program_page_8bit(addr, program, bc, false));
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
                    printf("Writing to %06X (%d words, flash)... ", addr, bc);
                    printf("%d words written\n", icsp_program_page(addr, program, bc, false));
                }
                break;
            }
            case 1: {
                printf("Programming complete; exiting LVP\n");
                icsp_exit_lvp();
                init = false;
                gpio_put(PICO_DEFAULT_LED_PIN, false);
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