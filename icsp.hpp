#ifndef ICSP_HPP
#define ICSP_HPP
#include <stdint.h>
#include <stddef.h>

enum {
    ICSP_ERASE_REGION_EEPROM = 1,
    ICSP_ERASE_REGION_FLASH = 2,
    ICSP_ERASE_REGION_USER_ID = 4,
    ICSP_ERASE_REGION_CONFIG = 8
};

enum {
    ICSP_COMMAND_LOAD_PC = 0x80,
    ICSP_COMMAND_BULK_ERASE = 0x18,
    ICSP_COMMAND_PAGE_ERASE = 0xF0,
    ICSP_COMMAND_READ_DATA = 0xFC,
    ICSP_COMMAND_READ_DATA_INCPC = 0xFE,
    ICSP_COMMAND_INCREMENT_ADDRESS = 0xF8,
    ICSP_COMMAND_PROGRAM_DATA = 0xC0,
    ICSP_COMMAND_PROGRAM_DATA_INCPC = 0xE0
};

void icsp_init(spi_inst_t *spi, int pin_mclr, int pin_dat, int pin_clk, int pin_dat_in);
void icsp_enter_lvp();
void icsp_exit_lvp();

void icsp_send_command(uint8_t cmd, int payload);
void icsp_cmd_loadpc(int pc);
void icsp_cmd_erase(int regions);
void icsp_cmd_erase_page();
uint16_t icsp_cmd_read_data(bool increment_pc);
void icsp_cmd_increment_pc();
void icsp_cmd_write_data(uint16_t value, bool increment_pc);

uint16_t icsp_get_device_id();
uint16_t icsp_get_revision_id();
void icsp_write_data(int addr, const uint16_t * data, size_t size);
void icsp_write_data_8bit(int addr, const uint8_t * data, size_t size);
void icsp_read_data(int addr, uint16_t * data, size_t size);
void icsp_read_data_8bit(int addr, uint8_t * data, size_t size);
void icsp_erase_page(int addr);
size_t icsp_program_page(int addr, uint16_t * data, size_t size, bool erase = true);
size_t icsp_program_page_8bit(int addr, uint8_t * data, size_t size, bool erase = true);

#endif
