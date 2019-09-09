# note-nrf

This is an example of how to use the Notecard and the [note-c][note-c] library
with the native Nordic nRF52 SDK.

As a proof-of-concept, it implements the same functions as the
[note-arduino][note-arduino] library's JSON example.

## Installation

This repo should be cloned into a "parent" dev folder with the following
organization:

```
parent  
├ note-nrf  
├ note-c  
└ sdk-current  
```

Where
- [note-nrf][note-nrf] - The Notecard example for Nordic nRF5x devices.
- [note-c][note-c] - The Notecard SDK for C.
- [sdk-current][sdk-current] - The latest Nordic nRF5 SDK.

In order to do development, it may be the case that all you need to do is to install the nRF SDK as above,
and to install the Nordic/Crossworks "Segger Embedded Studio" (SES) available here with a free license:
https://www.nordicsemi.com/?sc_itemid=%7B48E11346-206B-45FD-860D-637E4588990B%7D

The board that I used for testing is the Adafruit Feather nRF52840 Express, which I selected because
of its built-in SWD/JTAG interface and connector:
https://www.adafruit.com/product/4062

The probe that works perfectly with both the Adafruit device and SES, and comes with the right cable, is:
https://www.segger.com/products/debug-probes/j-link/models/j-link-edu-mini/

However, if there are any dependency issues, the entire Nordic development environment can be
obtained by installing nRF Connect for Desktop:
https://www.nordicsemi.com/?sc_itemid=%7BB935528E-8BFA-42D9-8BB5-83E2A5E1FF5C%7D

This example has been tested with both UART and with I2C, with I2C being observed to be extremely reliable
and lower power draw than UART.

[note-nrf]: https://github.com/blues/note-nrf
[note-c]: https://github.com/blues/note-c
[note-arduino]: https://github.com/blues/note-arduino
[sdk-current]: https://www.nordicsemi.com/Software-and-Tools/Software/nRF5-SDK

