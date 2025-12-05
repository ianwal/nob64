#include "N64Console.hpp"
#include "N64Controller.hpp"
#include "n64_definitions.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <hardware/clocks.h>
#include <hardware/pio.h>
#include <memory>
#include <pico/stdlib.h>
#include <pico/time.h>

using namespace std::chrono_literals;

#if 0

N64Console* console;

int main(void)
{
    set_sys_clock_khz(130'000, true);

    uint joybus_pin = 4;

    console = new N64Console(joybus_pin, pio0);

    // Set up LED
    bool led = true;
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, led);

    auto start       = time_us_64();
    bool flip        = false;
    bool a_press     = false;
    static bool once = false;
    while (true) {
        n64_report_t report = default_n64_report;
        auto curr           = time_us_64();
        if (!once && (std::chrono::microseconds{ curr - start } > 10s)) {
            report.start = true;
            once         = true;
        }

        if ((std::chrono::microseconds{ time_us_64() } > 15s)) {
            report.stick_y = 120;
            if ((std::chrono::microseconds{ (curr - start) } > 3s)) {
                report.a = true;
                start    = time_us_64();
            }
        }

        console->WaitForPoll();
        console->SendReport(&report);
    }
}
#endif

#if 0

N64Controller* controller;

int main(void)
{
    set_sys_clock_khz(130'000, true);

    stdio_init_all();

    uint joybus_pin = 4;

    controller          = new N64Controller(joybus_pin, 120, pio0);
    n64_report_t report = default_n64_report;

    // Set up LED
    bool led = true;
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    while (true) {
        controller->Poll(&report, 0);

        printf("A: %d\n", report.a);
        printf("B: %d\n", report.b);
        printf("C-Left: %d\n", report.c_left);
        printf("C-Right: %d\n", report.c_right);
        printf("C-Down: %d\n", report.c_down);
        printf("C-Up: %d\n", report.c_up);
        printf("L: %d\n", report.l);
        printf("R: %d\n", report.r);
        printf("Z: %d\n", report.z);
        printf("Start: %d\n", report.start);
        printf("D-Pad Left: %d\n", report.dpad_left);
        printf("D-Pad Right: %d\n", report.dpad_right);
        printf("D-Pad Down: %d\n", report.dpad_down);
        printf("D-Pad Up: %d\n", report.dpad_up);
        printf("Stick X-Axis: %d\n", report.stick_x);
        printf("Stick Y-Axis: %d\n", report.stick_y);

        // Toggle LED
        // led = !led;
        // gpio_put(PICO_DEFAULT_LED_PIN, led);
    }
}

#endif

#if 1

// Analog Stick GPIO
// XA and XB are the signal pins for the x-axis.
// YA and YB are for the y-axis.
//
// How the analog stick is read:
// When XA signal changes edge, an interrupt is fired and XB is compared to XB.
// If XA == XB then the input is positive, otherwise it's negative.
// The stick value is relative, so the sum of all movements is the position of the stick.
// Repeat for the Y axis.
constexpr uint xa_pin = 123;
constexpr uint xb_pin = 123;
constexpr uint ya_pin = 123;
constexpr uint yb_pin = 123;

// TODO: alignas(64) these to prevent false-sharing?
std::atomic<std::int8_t> analog_stick_x{ 0U };
std::atomic<std::int8_t> analog_stick_y{ 0U };

/// @brief Reset the analog sticks back to the center.
void reset_analog_stick_calibration()
{
    analog_stick_x = 0U;
    analog_stick_y = 0U;
}

/// @brief Process one of the analog stick axis signals.
///
/// @note This is marked __not_in_flash_func to try to improve performance since this is an ISR.
///
/// @see https://n64brew.dev/wiki/Controller
void __not_in_flash_func(analog_stick_isr)(uint gpio, uint32_t event_mask)
{
    if (gpio == xa_pin) {
        bool const xb = gpio_get(xb_pin);
        if (event_mask & GPIO_IRQ_EDGE_RISE) {
            // If xa and xb are both HIGH then it's moving positive, else negative.
            analog_stick_x.fetch_add((xb ? 1 : -1));
        } else if (event_mask & GPIO_IRQ_EDGE_FALL) {
            // If xa and xb are both LOW then it's moving positive, else negative.
            analog_stick_x.fetch_add((!xb ? 1 : -1));
        } else {
            // Unexpected.
        }
    } else if (gpio == ya_pin) {
        bool const yb = gpio_get(yb_pin);
        if (event_mask & GPIO_IRQ_EDGE_RISE) {
            // If ya and yb are both HIGH then it's moving positive, else negative.
            analog_stick_y.fetch_add((yb ? 1 : -1));
        } else if (event_mask & GPIO_IRQ_EDGE_FALL) {
            // If ya and yb are both LOW then it's moving positive, else negative.
            analog_stick_y.fetch_add((!yb ? 1 : -1));
        } else {
            // Unexpected.
        }
    } else {
        // Unexpected.
    }
}

int main(void)
{
    set_sys_clock_khz(130'000, true);

    stdio_init_all();

    reset_analog_stick_calibration();

    uint const joybus_pin = 4;

    gpio_init(xa_pin);
    gpio_init(xb_pin);
    gpio_init(ya_pin);
    gpio_init(yb_pin);

    gpio_set_dir(xa_pin, false);
    gpio_set_dir(xb_pin, false);
    gpio_set_dir(ya_pin, false);
    gpio_set_dir(yb_pin, false);

    std::uint32_t const gpio_irq_edge_change = GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE;
    gpio_set_irq_callback(analog_stick_isr);
    gpio_set_irq_enabled(xa_pin, gpio_irq_edge_change, true);
    gpio_set_irq_enabled(ya_pin, gpio_irq_edge_change, true);
    irq_set_enabled(IO_IRQ_BANK0, true);

    N64Console console{ joybus_pin, pio0 };

    n64_report_t report = default_n64_report;
    while (true) {
        // TODO: Probably need to offload the analog stick ISR and button reading to another core to avoid disrupting joybus.
        console.WaitForPoll();
        report.stick_x = analog_stick_x.load();
        report.stick_y = analog_stick_y.load();
        console.SendReport(&report);
    }
}

#endif
