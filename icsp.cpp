#include "icsp.hpp"
#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <pico/time.h>

static int dataOutPin = PICO_DEFAULT_SPI_TX_PIN, dataInPin = PICO_DEFAULT_SPI_RX_PIN, clockPin = PICO_DEFAULT_SPI_SCK_PIN, mclrPin = PICO_DEFAULT_SPI_TX_PIN + 1;
static spi_inst_t *spi;

void icsp_init(spi_inst_t *spi_, int pin_mclr, int pin_dat, int pin_clk, int pin_dat_in) {
    gpio_init(pin_mclr);
    gpio_init(pin_dat);
    gpio_init(pin_clk);
    if (pin_dat_in != -1) {
        gpio_init(pin_dat_in);
        gpio_set_function(pin_dat_in, GPIO_FUNC_SPI);
    }
    gpio_set_function(pin_dat, GPIO_FUNC_SPI);
    gpio_set_function(pin_clk, GPIO_FUNC_SPI);
    gpio_set_function(pin_mclr, GPIO_FUNC_NULL);
    gpio_put(pin_mclr, 1);
    spi_init(spi_, 5'000'000); // 100 ns high + 100 ns low pulse width = 5 MHz
    spi_set_slave(spi_, false);
    spi_set_format(spi_, 8, SPI_CPOL_1, SPI_CPHA_0, SPI_MSB_FIRST);
    dataOutPin = pin_dat;
    dataInPin = pin_dat_in;
    clockPin = pin_clk;
    mclrPin = pin_mclr;
    spi = spi_;
}

void icsp_enter_lvp() {
    gpio_put(mclrPin, 0);
    sleep_us(1);
    spi_write_blocking(spi, "MCHP", 4);
    sleep_us(1);
}

void icsp_exit_lvp() {
    gpio_put(mclrPin, 1);
    sleep_us(1);
}

void icsp_send_command(uint8_t cmd, int payload) {
    spi_write_blocking(spi, &cmd, 1);
    if (payload >= 0) {
        uint8_t pl[3] = {(payload >> 16) & 0xFF, (payload >> 8) & 0xFF, payload & 0xFF};
        spi_write_blocking(spi, pl, 3);
    }
}

void icsp_cmd_loadpc(int pc) {
    icsp_send_command(ICSP_COMMAND_LOAD_PC, pc);
}

void icsp_cmd_erase(int regions) {
    icsp_send_command(ICSP_COMMAND_BULK_ERASE, regions << 1);
    sleep_ms(11);
}

void icsp_cmd_erase_page() {
    icsp_send_command(ICSP_COMMAND_PAGE_ERASE, -1);
    sleep_ms(11);
}

uint16_t icsp_cmd_read_data(bool increment_pc) {
    if (dataInPin == -1) return 0xFFFF;
    uint8_t cmd = (increment_pc ? ICSP_COMMAND_READ_DATA_INCPC : ICSP_COMMAND_READ_DATA);
    spi_write_blocking(spi, &cmd, 1);
    gpio_set_function(dataOutPin, GPIO_FUNC_NULL);
    gpio_set_dir(dataOutPin, GPIO_IN);
    // 100 ns should have passed by now
    uint8_t data[3];
    spi_read_blocking(spi, 0, data, 3);
    return (data[1] << 8) | data[0];
}

void icsp_cmd_increment_pc() {
    icsp_send_command(ICSP_COMMAND_INCREMENT_ADDRESS, -1);
}

void icsp_cmd_write_data(uint16_t value, bool increment_pc) {
    icsp_send_command(increment_pc ? ICSP_COMMAND_READ_DATA_INCPC : ICSP_COMMAND_READ_DATA, value);
    sleep_us(75);
}

uint16_t icsp_get_device_id() {
    icsp_cmd_loadpc(0x3FFFFE);
    return icsp_cmd_read_data(false);
}

uint16_t icsp_get_revision_id() {
    icsp_cmd_loadpc(0x3FFFFC);
    return icsp_cmd_read_data(false);
}

void icsp_write_data(int addr, const uint16_t * data, size_t size) {
    icsp_cmd_loadpc(addr);
    for (int i = 0; i < size; i++) icsp_cmd_write_data(data[i], true);
}

void icsp_write_data_8bit(int addr, const uint8_t * data, size_t size) {
    icsp_cmd_loadpc(addr);
    for (int i = 0; i < size; i++) {
        icsp_cmd_write_data(data[i], true);
        sleep_ms(11);
    }
}

void icsp_read_data(int addr, uint16_t * data, size_t size) {
    icsp_cmd_loadpc(addr);
    for (int i = 0; i < size; i++) data[i] = icsp_cmd_read_data(true);
}

void icsp_read_data_8bit(int addr, uint8_t * data, size_t size) {
    icsp_cmd_loadpc(addr);
    for (int i = 0; i < size; i++) data[i] = icsp_cmd_read_data(true) & 0xFF;
}

void icsp_erase_page(int addr) {
    icsp_cmd_loadpc(addr);
    icsp_cmd_erase_page();
}

size_t icsp_program_page(int addr, uint16_t * data, size_t size, bool erase) {
    icsp_cmd_loadpc(addr);
    if (erase) icsp_cmd_erase_page();
    for (int i = 0; i < size; i++) {
        icsp_cmd_write_data(data[i], false);
        if (dataInPin != -1) {
            uint16_t word = icsp_cmd_read_data(true);
            if (word != data[i]) return i;
        }
    }
    return size;
}

size_t icsp_program_page_8bit(int addr, uint8_t * data, size_t size, bool erase) {
    icsp_cmd_loadpc(addr);
    if (erase) icsp_cmd_erase_page();
    for (int i = 0; i < size; i++) {
        icsp_cmd_write_data(data[i], false);
        sleep_ms(11);
        if (dataInPin != -1) {
            uint8_t byte = icsp_cmd_read_data(true) & 0xFF;
            if (byte != data[i]) return i;
        }
    }
    return size;
}
