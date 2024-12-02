#pragma once

#include <stdint.h>
#include <unordered_map>

namespace display
{
    struct tImage {
        uint8_t *data;
        uint16_t width;
        uint16_t height;
        uint8_t dataSize;
    };

    struct tChar {
        int code;
        tImage *image;
    };

    struct tFont {
        int length;
        tChar *chars;
    };

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x20[10] = {
        // ██████
        // ██████
        // ██████
        // ██████
        // ██████
        // ██████
        // ██████
        // ██████
        // ██████
        // ██████
        0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc};
    static tImage Font_0x20 = {image_data_Font_0x20, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x2d[10] = {
        // ██████
        // ██████
        // ██████
        // ██████
        // █∙∙∙∙∙
        // ██████
        // ██████
        // ██████
        // ██████
        // ██████
        0xfc, 0xfc, 0xfc, 0xfc, 0x80, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc};
    static tImage Font_0x2d = {image_data_Font_0x2d, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x2e[10] = {
        // ██████
        // ██████
        // ██████
        // ██████
        // ██████
        // ██████
        // ███∙∙█
        // ███∙∙█
        // ██████
        // ██████
        0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xe4, 0xe4, 0xfc, 0xfc};
    static tImage Font_0x2e = {image_data_Font_0x2e, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x41[10] = {
        // ██∙∙∙█
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // █∙∙∙∙∙
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // ██████
        // ██████
        0xc4, 0xb8, 0xb8, 0xb8, 0x80, 0xb8, 0xb8, 0xb8, 0xfc, 0xfc};
    static tImage Font_0x41 = {image_data_Font_0x41, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x42[10] = {
        // ██∙∙∙∙
        // █∙███∙
        // █∙███∙
        // ██∙∙∙∙
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // ██∙∙∙∙
        // ██████
        // ██████
        0xc0, 0xb8, 0xb8, 0xc0, 0xb8, 0xb8, 0xb8, 0xc0, 0xfc, 0xfc};
    static tImage Font_0x42 = {image_data_Font_0x42, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x43[10] = {
        // ██∙∙∙█
        // █∙███∙
        // █████∙
        // █████∙
        // █████∙
        // █████∙
        // █∙███∙
        // ██∙∙∙█
        // ██████
        // ██████
        0xc4, 0xb8, 0xf8, 0xf8, 0xf8, 0xf8, 0xb8, 0xc4, 0xfc, 0xfc};
    static tImage Font_0x43 = {image_data_Font_0x43, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x44[10] = {
        // ██∙∙∙∙
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // ██∙∙∙∙
        // ██████
        // ██████
        0xc0, 0xb8, 0xb8, 0xb8, 0xb8, 0xb8, 0xb8, 0xc0, 0xfc, 0xfc};
    static tImage Font_0x44 = {image_data_Font_0x44, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x45[10] = {
        // █∙∙∙∙∙
        // █████∙
        // █████∙
        // ██∙∙∙∙
        // █████∙
        // █████∙
        // █████∙
        // █∙∙∙∙∙
        // ██████
        // ██████
        0x80, 0xf8, 0xf8, 0xc0, 0xf8, 0xf8, 0xf8, 0x80, 0xfc, 0xfc};
    static tImage Font_0x45 = {image_data_Font_0x45, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x46[10] = {
        // █∙∙∙∙∙
        // █████∙
        // █████∙
        // ██∙∙∙∙
        // █████∙
        // █████∙
        // █████∙
        // █████∙
        // ██████
        // ██████
        0x80, 0xf8, 0xf8, 0xc0, 0xf8, 0xf8, 0xf8, 0xf8, 0xfc, 0xfc};
    static tImage Font_0x46 = {image_data_Font_0x46, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x47[10] = {
        // ██∙∙∙█
        // █∙███∙
        // █████∙
        // █████∙
        // █∙∙██∙
        // █∙███∙
        // █∙███∙
        // █∙∙∙∙█
        // ██████
        // ██████
        0xc4, 0xb8, 0xf8, 0xf8, 0x98, 0xb8, 0xb8, 0x84, 0xfc, 0xfc};
    static tImage Font_0x47 = {image_data_Font_0x47, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x48[10] = {
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // █∙∙∙∙∙
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // ██████
        // ██████
        0xb8, 0xb8, 0xb8, 0x80, 0xb8, 0xb8, 0xb8, 0xb8, 0xfc, 0xfc};
    static tImage Font_0x48 = {image_data_Font_0x48, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x49[10] = {
        // ██∙∙∙█
        // ███∙██
        // ███∙██
        // ███∙██
        // ███∙██
        // ███∙██
        // ███∙██
        // ██∙∙∙█
        // ██████
        // ██████
        0xc4, 0xec, 0xec, 0xec, 0xec, 0xec, 0xec, 0xc4, 0xfc, 0xfc};
    static tImage Font_0x49 = {image_data_Font_0x49, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x4a[10] = {
        // █∙∙∙∙█
        // ██∙███
        // ██∙███
        // ██∙███
        // ██∙███
        // ██∙███
        // ██∙███
        // ███∙∙∙
        // ██████
        // ██████
        0x84, 0xdc, 0xdc, 0xdc, 0xdc, 0xdc, 0xdc, 0xe0, 0xfc, 0xfc};
    static tImage Font_0x4a = {image_data_Font_0x4a, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x4b[10] = {
        // █∙███∙
        // ██∙██∙
        // ███∙█∙
        // ████∙∙
        // ███∙█∙
        // ██∙██∙
        // █∙███∙
        // █∙███∙
        // ██████
        // ██████
        0xb8, 0xd8, 0xe8, 0xf0, 0xe8, 0xd8, 0xb8, 0xb8, 0xfc, 0xfc};
    static tImage Font_0x4b = {image_data_Font_0x4b, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x4c[10] = {
        // █████∙
        // █████∙
        // █████∙
        // █████∙
        // █████∙
        // █████∙
        // █████∙
        // █∙∙∙∙∙
        // ██████
        // ██████
        0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0x80, 0xfc, 0xfc};
    static tImage Font_0x4c = {image_data_Font_0x4c, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x4d[10] = {
        // █∙███∙
        // █∙███∙
        // █∙∙█∙∙
        // █∙∙█∙∙
        // █∙█∙█∙
        // █∙█∙█∙
        // █∙███∙
        // █∙███∙
        // ██████
        // ██████
        0xb8, 0xb8, 0x90, 0x90, 0xa8, 0xa8, 0xb8, 0xb8, 0xfc, 0xfc};
    static tImage Font_0x4d = {image_data_Font_0x4d, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x4e[10] = {
        // █∙███∙
        // █∙██∙∙
        // █∙██∙∙
        // █∙█∙█∙
        // █∙█∙█∙
        // █∙∙██∙
        // █∙∙██∙
        // █∙███∙
        // ██████
        // ██████
        0xb8, 0xb0, 0xb0, 0xa8, 0xa8, 0x98, 0x98, 0xb8, 0xfc, 0xfc};
    static tImage Font_0x4e = {image_data_Font_0x4e, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x4f[10] = {
        // ██∙∙∙█
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // ██∙∙∙█
        // ██████
        // ██████
        0xc4, 0xb8, 0xb8, 0xb8, 0xb8, 0xb8, 0xb8, 0xc4, 0xfc, 0xfc};
    static tImage Font_0x4f = {image_data_Font_0x4f, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x50[10] = {
        // ██∙∙∙∙
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // ██∙∙∙∙
        // █████∙
        // █████∙
        // █████∙
        // ██████
        // ██████
        0xc0, 0xb8, 0xb8, 0xb8, 0xc0, 0xf8, 0xf8, 0xf8, 0xfc, 0xfc};
    static tImage Font_0x50 = {image_data_Font_0x50, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x51[10] = {
        // ██∙∙∙█
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // █∙█∙█∙
        // ██∙██∙
        // █∙█∙∙█
        // █∙████
        // ██████
        0xc4, 0xb8, 0xb8, 0xb8, 0xb8, 0xa8, 0xd8, 0xa4, 0xbc, 0xfc};
    static tImage Font_0x51 = {image_data_Font_0x51, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x52[10] = {
        // ██∙∙∙∙
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // ██∙∙∙∙
        // ███∙█∙
        // ██∙██∙
        // █∙███∙
        // ██████
        // ██████
        0xc0, 0xb8, 0xb8, 0xb8, 0xc0, 0xe8, 0xd8, 0xb8, 0xfc, 0xfc};
    static tImage Font_0x52 = {image_data_Font_0x52, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x53[10] = {
        // █∙∙∙∙█
        // █████∙
        // █████∙
        // ██∙∙∙█
        // █∙████
        // █∙████
        // █∙████
        // ██∙∙∙∙
        // ██████
        // ██████
        0x84, 0xf8, 0xf8, 0xc4, 0xbc, 0xbc, 0xbc, 0xc0, 0xfc, 0xfc};
    static tImage Font_0x53 = {image_data_Font_0x53, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x54[10] = {
        // █∙∙∙∙∙
        // ███∙██
        // ███∙██
        // ███∙██
        // ███∙██
        // ███∙██
        // ███∙██
        // ███∙██
        // ██████
        // ██████
        0x80, 0xec, 0xec, 0xec, 0xec, 0xec, 0xec, 0xec, 0xfc, 0xfc};
    static tImage Font_0x54 = {image_data_Font_0x54, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x55[10] = {
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // ██∙∙∙█
        // ██████
        // ██████
        0xb8, 0xb8, 0xb8, 0xb8, 0xb8, 0xb8, 0xb8, 0xc4, 0xfc, 0xfc};
    static tImage Font_0x55 = {image_data_Font_0x55, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x56[10] = {
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // ██∙█∙█
        // ██∙█∙█
        // ██∙█∙█
        // ███∙██
        // ███∙██
        // ██████
        // ██████
        0xb8, 0xb8, 0xb8, 0xd4, 0xd4, 0xd4, 0xec, 0xec, 0xfc, 0xfc};
    static tImage Font_0x56 = {image_data_Font_0x56, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x57[10] = {
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // █∙█∙█∙
        // █∙█∙█∙
        // █∙∙█∙∙
        // █∙∙█∙∙
        // █∙███∙
        // ██████
        // ██████
        0xb8, 0xb8, 0xb8, 0xa8, 0xa8, 0x90, 0x90, 0xb8, 0xfc, 0xfc};
    static tImage Font_0x57 = {image_data_Font_0x57, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x58[10] = {
        // █∙███∙
        // ██∙█∙█
        // ██∙█∙█
        // ███∙██
        // ███∙██
        // ██∙█∙█
        // ██∙█∙█
        // █∙███∙
        // ██████
        // ██████
        0xb8, 0xd4, 0xd4, 0xec, 0xec, 0xd4, 0xd4, 0xb8, 0xfc, 0xfc};
    static tImage Font_0x58 = {image_data_Font_0x58, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x59[10] = {
        // █∙███∙
        // █∙███∙
        // ██∙█∙█
        // ██∙█∙█
        // ███∙██
        // ███∙██
        // ███∙██
        // ███∙██
        // ██████
        // ██████
        0xb8, 0xb8, 0xd4, 0xd4, 0xec, 0xec, 0xec, 0xec, 0xfc, 0xfc};
    static tImage Font_0x59 = {image_data_Font_0x59, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x5a[10] = {
        // █∙∙∙∙∙
        // ██∙███
        // ██∙███
        // ███∙██
        // ███∙██
        // ████∙█
        // ████∙█
        // █∙∙∙∙∙
        // ██████
        // ██████
        0x80, 0xdc, 0xdc, 0xec, 0xec, 0xf4, 0xf4, 0x80, 0xfc, 0xfc};
    static tImage Font_0x5a = {image_data_Font_0x5a, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x61[10] = {
        // ██████
        // ██████
        // ██∙∙∙█
        // █∙████
        // █∙∙∙∙█
        // █∙███∙
        // █∙███∙
        // █∙∙∙∙█
        // ██████
        // ██████
        0xfc, 0xfc, 0xc4, 0xbc, 0x84, 0xb8, 0xb8, 0x84, 0xfc, 0xfc};
    static tImage Font_0x61 = {image_data_Font_0x61, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x62[10] = {
        // █████∙
        // █████∙
        // ██∙∙█∙
        // █∙██∙∙
        // █∙███∙
        // █∙███∙
        // █∙██∙∙
        // ██∙∙█∙
        // ██████
        // ██████
        0xf8, 0xf8, 0xc8, 0xb0, 0xb8, 0xb8, 0xb0, 0xc8, 0xfc, 0xfc};
    static tImage Font_0x62 = {image_data_Font_0x62, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x63[10] = {
        // ██████
        // ██████
        // ██∙∙∙█
        // █∙███∙
        // █████∙
        // █████∙
        // █∙███∙
        // ██∙∙∙█
        // ██████
        // ██████
        0xfc, 0xfc, 0xc4, 0xb8, 0xf8, 0xf8, 0xb8, 0xc4, 0xfc, 0xfc};
    static tImage Font_0x63 = {image_data_Font_0x63, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x64[10] = {
        // █∙████
        // █∙████
        // █∙█∙∙█
        // █∙∙██∙
        // █∙███∙
        // █∙███∙
        // █∙∙██∙
        // █∙█∙∙█
        // ██████
        // ██████
        0xbc, 0xbc, 0xa4, 0x98, 0xb8, 0xb8, 0x98, 0xa4, 0xfc, 0xfc};
    static tImage Font_0x64 = {image_data_Font_0x64, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x65[10] = {
        // ██████
        // ██████
        // ██∙∙∙█
        // █∙███∙
        // █∙███∙
        // █∙∙∙∙∙
        // █████∙
        // █∙∙∙∙█
        // ██████
        // ██████
        0xfc, 0xfc, 0xc4, 0xb8, 0xb8, 0x80, 0xf8, 0x84, 0xfc, 0xfc};
    static tImage Font_0x65 = {image_data_Font_0x65, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x66[10] = {
        // ██∙███
        // ███∙██
        // ███∙██
        // ██∙∙∙█
        // ███∙██
        // ███∙██
        // ███∙██
        // ███∙██
        // ██████
        // ██████
        0xdc, 0xec, 0xec, 0xc4, 0xec, 0xec, 0xec, 0xec, 0xfc, 0xfc};
    static tImage Font_0x66 = {image_data_Font_0x66, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x67[10] = {
        // ██████
        // ██████
        // █∙█∙∙█
        // █∙∙██∙
        // █∙███∙
        // █∙███∙
        // █∙∙██∙
        // █∙█∙∙█
        // █∙████
        // ██∙∙∙∙
        0xfc, 0xfc, 0xa4, 0x98, 0xb8, 0xb8, 0x98, 0xa4, 0xbc, 0xc0};
    static tImage Font_0x67 = {image_data_Font_0x67, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x68[10] = {
        // █████∙
        // █████∙
        // ██∙∙█∙
        // █∙██∙∙
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // ██████
        // ██████
        0xf8, 0xf8, 0xc8, 0xb0, 0xb8, 0xb8, 0xb8, 0xb8, 0xfc, 0xfc};
    static tImage Font_0x68 = {image_data_Font_0x68, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x69[10] = {
        // ███∙██
        // ██████
        // ███∙∙█
        // ███∙██
        // ███∙██
        // ███∙██
        // ███∙██
        // ██∙∙∙█
        // ██████
        // ██████
        0xec, 0xfc, 0xe4, 0xec, 0xec, 0xec, 0xec, 0xc4, 0xfc, 0xfc};
    static tImage Font_0x69 = {image_data_Font_0x69, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x6a[10] = {
        // ██∙███
        // ██████
        // ██∙∙∙█
        // ██∙███
        // ██∙███
        // ██∙███
        // ██∙███
        // ██∙███
        // ██∙███
        // ███∙∙█
        0xdc, 0xfc, 0xc4, 0xdc, 0xdc, 0xdc, 0xdc, 0xdc, 0xdc, 0xe4};
    static tImage Font_0x6a = {image_data_Font_0x6a, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x6b[10] = {
        // █████∙
        // █████∙
        // █∙███∙
        // ██∙██∙
        // ███∙█∙
        // ███∙∙∙
        // ██∙██∙
        // █∙███∙
        // ██████
        // ██████
        0xf8, 0xf8, 0xb8, 0xd8, 0xe8, 0xe0, 0xd8, 0xb8, 0xfc, 0xfc};
    static tImage Font_0x6b = {image_data_Font_0x6b, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x6c[10] = {
        // ███∙██
        // ███∙██
        // ███∙██
        // ███∙██
        // ███∙██
        // ███∙██
        // ███∙██
        // ██∙███
        // ██████
        // ██████
        0xec, 0xec, 0xec, 0xec, 0xec, 0xec, 0xec, 0xdc, 0xfc, 0xfc};
    static tImage Font_0x6c = {image_data_Font_0x6c, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x6d[10] = {
        // ██████
        // ██████
        // ██∙∙∙∙
        // █∙█∙█∙
        // █∙█∙█∙
        // █∙█∙█∙
        // █∙█∙█∙
        // █∙█∙█∙
        // ██████
        // ██████
        0xfc, 0xfc, 0xc0, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xfc, 0xfc};
    static tImage Font_0x6d = {image_data_Font_0x6d, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x6e[10] = {
        // ██████
        // ██████
        // ██∙∙█∙
        // █∙██∙∙
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // ██████
        // ██████
        0xfc, 0xfc, 0xc8, 0xb0, 0xb8, 0xb8, 0xb8, 0xb8, 0xfc, 0xfc};
    static tImage Font_0x6e = {image_data_Font_0x6e, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x6f[10] = {
        // ██████
        // ██████
        // ██∙∙∙█
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // ██∙∙∙█
        // ██████
        // ██████
        0xfc, 0xfc, 0xc4, 0xb8, 0xb8, 0xb8, 0xb8, 0xc4, 0xfc, 0xfc};
    static tImage Font_0x6f = {image_data_Font_0x6f, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x70[10] = {
        // ██████
        // ██████
        // ██∙∙█∙
        // █∙██∙∙
        // █∙███∙
        // █∙███∙
        // █∙██∙∙
        // ██∙∙█∙
        // █████∙
        // █████∙
        0xfc, 0xfc, 0xc8, 0xb0, 0xb8, 0xb8, 0xb0, 0xc8, 0xf8, 0xf8};
    static tImage Font_0x70 = {image_data_Font_0x70, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x71[10] = {
        // ██████
        // ██████
        // █∙█∙∙█
        // █∙∙██∙
        // █∙███∙
        // █∙███∙
        // █∙∙██∙
        // █∙█∙∙█
        // █∙████
        // █∙████
        0xfc, 0xfc, 0xa4, 0x98, 0xb8, 0xb8, 0x98, 0xa4, 0xbc, 0xbc};
    static tImage Font_0x71 = {image_data_Font_0x71, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x72[10] = {
        // ██████
        // ██████
        // ██∙∙█∙
        // █∙██∙∙
        // █████∙
        // █████∙
        // █████∙
        // █████∙
        // ██████
        // ██████
        0xfc, 0xfc, 0xc8, 0xb0, 0xf8, 0xf8, 0xf8, 0xf8, 0xfc, 0xfc};
    static tImage Font_0x72 = {image_data_Font_0x72, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x73[10] = {
        // ██████
        // ██████
        // █∙∙∙∙█
        // █████∙
        // ██∙∙∙█
        // █∙████
        // █∙████
        // ██∙∙∙∙
        // ██████
        // ██████
        0xfc, 0xfc, 0x84, 0xf8, 0xc4, 0xbc, 0xbc, 0xc0, 0xfc, 0xfc};
    static tImage Font_0x73 = {image_data_Font_0x73, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x74[10] = {
        // ███∙██
        // ███∙██
        // ██∙∙∙█
        // ███∙██
        // ███∙██
        // ███∙██
        // ███∙██
        // ██∙███
        // ██████
        // ██████
        0xec, 0xec, 0xc4, 0xec, 0xec, 0xec, 0xec, 0xdc, 0xfc, 0xfc};
    static tImage Font_0x74 = {image_data_Font_0x74, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x75[10] = {
        // ██████
        // ██████
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // █∙∙██∙
        // █∙█∙∙█
        // ██████
        // ██████
        0xfc, 0xfc, 0xb8, 0xb8, 0xb8, 0xb8, 0x98, 0xa4, 0xfc, 0xfc};
    static tImage Font_0x75 = {image_data_Font_0x75, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x76[10] = {
        // ██████
        // ██████
        // █∙███∙
        // █∙███∙
        // ██∙█∙█
        // ██∙█∙█
        // ███∙██
        // ███∙██
        // ██████
        // ██████
        0xfc, 0xfc, 0xb8, 0xb8, 0xd4, 0xd4, 0xec, 0xec, 0xfc, 0xfc};
    static tImage Font_0x76 = {image_data_Font_0x76, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x77[10] = {
        // ██████
        // ██████
        // █∙███∙
        // █∙███∙
        // █∙█∙█∙
        // █∙█∙█∙
        // █∙∙█∙∙
        // █∙███∙
        // ██████
        // ██████
        0xfc, 0xfc, 0xb8, 0xb8, 0xa8, 0xa8, 0x90, 0xb8, 0xfc, 0xfc};
    static tImage Font_0x77 = {image_data_Font_0x77, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x78[10] = {
        // ██████
        // ██████
        // █∙███∙
        // ██∙█∙█
        // ███∙██
        // ███∙██
        // ██∙█∙█
        // █∙███∙
        // ██████
        // ██████
        0xfc, 0xfc, 0xb8, 0xd4, 0xec, 0xec, 0xd4, 0xb8, 0xfc, 0xfc};
    static tImage Font_0x78 = {image_data_Font_0x78, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x79[10] = {
        // ██████
        // ██████
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // █∙∙██∙
        // █∙█∙∙█
        // █∙████
        // ██∙∙∙∙
        0xfc, 0xfc, 0xb8, 0xb8, 0xb8, 0xb8, 0x98, 0xa4, 0xbc, 0xc0};
    static tImage Font_0x79 = {image_data_Font_0x79, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x7a[10] = {
        // ██████
        // ██████
        // █∙∙∙∙∙
        // ██∙███
        // ███∙██
        // ███∙██
        // ████∙█
        // █∙∙∙∙∙
        // ██████
        // ██████
        0xfc, 0xfc, 0x80, 0xdc, 0xec, 0xec, 0xf4, 0x80, 0xfc, 0xfc};
    static tImage Font_0x7a = {image_data_Font_0x7a, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x21[10] = {
        // ███∙██
        // ███∙██
        // ███∙██
        // ███∙██
        // ███∙██
        // ███∙██
        // ██████
        // ███∙██
        // ██████
        // ██████
        0xec, 0xec, 0xec, 0xec, 0xec, 0xec, 0xfc, 0xec, 0xfc, 0xfc};
    static tImage Font_0x21 = {image_data_Font_0x21, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x23[10] = {
        // ██∙█∙█
        // ██∙█∙█
        // █∙∙∙∙∙
        // ██∙█∙█
        // ██∙█∙█
        // █∙∙∙∙∙
        // ██∙█∙█
        // ██∙█∙█
        // ██████
        // ██████
        0xd4, 0xd4, 0x80, 0xd4, 0xd4, 0x80, 0xd4, 0xd4, 0xfc, 0xfc};
    static tImage Font_0x23 = {image_data_Font_0x23, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x25[10] = {
        // █∙████
        // ██∙█∙∙
        // ██∙█∙∙
        // ███∙██
        // ███∙██
        // █∙∙█∙█
        // █∙∙█∙█
        // █████∙
        // ██████
        // ██████
        0xbc, 0xd0, 0xd0, 0xec, 0xec, 0x94, 0x94, 0xf8, 0xfc, 0xfc};
    static tImage Font_0x25 = {image_data_Font_0x25, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x26[10] = {
        // ██████
        // ██████
        // ███∙∙█
        // ██∙██∙
        // ███∙∙█
        // █∙█∙█∙
        // ██∙██∙
        // █∙█∙∙█
        // ██████
        // ██████
        0xfc, 0xfc, 0xe4, 0xd8, 0xe4, 0xa8, 0xd8, 0xa4, 0xfc, 0xfc};
    static tImage Font_0x26 = {image_data_Font_0x26, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x27[10] = {
        // ███∙██
        // ███∙██
        // ███∙██
        // ██████
        // ██████
        // ██████
        // ██████
        // ██████
        // ██████
        // ██████
        0xec, 0xec, 0xec, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc};
    static tImage Font_0x27 = {image_data_Font_0x27, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x28[10] = {
        // ██∙███
        // ███∙██
        // ████∙█
        // ████∙█
        // ████∙█
        // ████∙█
        // ███∙██
        // ██∙███
        // ██████
        // ██████
        0xdc, 0xec, 0xf4, 0xf4, 0xf4, 0xf4, 0xec, 0xdc, 0xfc, 0xfc};
    static tImage Font_0x28 = {image_data_Font_0x28, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x29[10] = {
        // ████∙█
        // ███∙██
        // ██∙███
        // ██∙███
        // ██∙███
        // ██∙███
        // ███∙██
        // ████∙█
        // ██████
        // ██████
        0xf4, 0xec, 0xdc, 0xdc, 0xdc, 0xdc, 0xec, 0xf4, 0xfc, 0xfc};
    static tImage Font_0x29 = {image_data_Font_0x29, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x2a[10] = {
        // ██████
        // ██████
        // ███∙██
        // ███∙██
        // █∙∙∙∙∙
        // ███∙██
        // ██∙█∙█
        // ██████
        // ██████
        // ██████
        0xfc, 0xfc, 0xec, 0xec, 0x80, 0xec, 0xd4, 0xfc, 0xfc, 0xfc};
    static tImage Font_0x2a = {image_data_Font_0x2a, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x2b[10] = {
        // ██████
        // ██████
        // ███∙██
        // ███∙██
        // █∙∙∙∙∙
        // ███∙██
        // ███∙██
        // ██████
        // ██████
        // ██████
        0xfc, 0xfc, 0xec, 0xec, 0x80, 0xec, 0xec, 0xfc, 0xfc, 0xfc};
    static tImage Font_0x2b = {image_data_Font_0x2b, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x2c[10] = {
        // ██████
        // ██████
        // ██████
        // ██████
        // ██████
        // ██████
        // ███∙∙█
        // ███∙██
        // ███∙██
        // ████∙█
        0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xe4, 0xec, 0xec, 0xf4};
    static tImage Font_0x2c = {image_data_Font_0x2c, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x30[10] = {
        // ██∙∙∙█
        // █∙███∙
        // █∙███∙
        // █∙█∙█∙
        // █∙█∙█∙
        // █∙███∙
        // █∙███∙
        // ██∙∙∙█
        // ██████
        // ██████
        0xc4, 0xb8, 0xb8, 0xa8, 0xa8, 0xb8, 0xb8, 0xc4, 0xfc, 0xfc};
    static tImage Font_0x30 = {image_data_Font_0x30, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x31[10] = {
        // ███∙██
        // ███∙∙█
        // ███∙█∙
        // ███∙██
        // ███∙██
        // ███∙██
        // ███∙██
        // █∙∙∙∙∙
        // ██████
        // ██████
        0xec, 0xe4, 0xe8, 0xec, 0xec, 0xec, 0xec, 0x80, 0xfc, 0xfc};
    static tImage Font_0x31 = {image_data_Font_0x31, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x32[10] = {
        // ██∙∙∙█
        // █∙███∙
        // █∙████
        // ██∙███
        // ███∙██
        // ████∙█
        // █████∙
        // █∙∙∙∙∙
        // ██████
        // ██████
        0xc4, 0xb8, 0xbc, 0xdc, 0xec, 0xf4, 0xf8, 0x80, 0xfc, 0xfc};
    static tImage Font_0x32 = {image_data_Font_0x32, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x33[10] = {
        // ██∙∙∙█
        // █∙███∙
        // █∙████
        // ██∙∙∙█
        // █∙████
        // █∙████
        // █∙███∙
        // ██∙∙∙█
        // ██████
        // ██████
        0xc4, 0xb8, 0xbc, 0xc4, 0xbc, 0xbc, 0xb8, 0xc4, 0xfc, 0xfc};
    static tImage Font_0x33 = {image_data_Font_0x33, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x34[10] = {
        // █∙████
        // █∙∙███
        // █∙█∙██
        // █∙██∙█
        // █∙███∙
        // █∙∙∙∙∙
        // █∙████
        // █∙████
        // ██████
        // ██████
        0xbc, 0x9c, 0xac, 0xb4, 0xb8, 0x80, 0xbc, 0xbc, 0xfc, 0xfc};
    static tImage Font_0x34 = {image_data_Font_0x34, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x35[10] = {
        // █∙∙∙∙∙
        // █████∙
        // █████∙
        // ██∙∙∙∙
        // █∙████
        // █∙████
        // █∙████
        // ██∙∙∙∙
        // ██████
        // ██████
        0x80, 0xf8, 0xf8, 0xc0, 0xbc, 0xbc, 0xbc, 0xc0, 0xfc, 0xfc};
    static tImage Font_0x35 = {image_data_Font_0x35, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x36[10] = {
        // ██∙∙∙█
        // █∙███∙
        // █████∙
        // ██∙∙∙∙
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // ██∙∙∙█
        // ██████
        // ██████
        0xc4, 0xb8, 0xf8, 0xc0, 0xb8, 0xb8, 0xb8, 0xc4, 0xfc, 0xfc};
    static tImage Font_0x36 = {image_data_Font_0x36, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x37[10] = {
        // █∙∙∙∙∙
        // █∙███∙
        // ██∙███
        // ███∙██
        // ███∙██
        // ████∙█
        // ████∙█
        // ████∙█
        // ██████
        // ██████
        0x80, 0xb8, 0xdc, 0xec, 0xec, 0xf4, 0xf4, 0xf4, 0xfc, 0xfc};
    static tImage Font_0x37 = {image_data_Font_0x37, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x38[10] = {
        // ██∙∙∙█
        // █∙███∙
        // █∙███∙
        // ██∙∙∙█
        // █∙███∙
        // █∙███∙
        // █∙███∙
        // ██∙∙∙█
        // ██████
        // ██████
        0xc4, 0xb8, 0xb8, 0xc4, 0xb8, 0xb8, 0xb8, 0xc4, 0xfc, 0xfc};
    static tImage Font_0x38 = {image_data_Font_0x38, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x39[10] = {
        // ██∙∙∙█
        // █∙███∙
        // █∙███∙
        // █∙∙∙∙█
        // █∙████
        // █∙████
        // █∙███∙
        // ██∙∙∙█
        // ██████
        // ██████
        0xc4, 0xb8, 0xb8, 0x84, 0xbc, 0xbc, 0xb8, 0xc4, 0xfc, 0xfc};
    static tImage Font_0x39 = {image_data_Font_0x39, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x3a[10] = {
        // ██████
        // ██████
        // ███∙∙█
        // ███∙∙█
        // ██████
        // ███∙∙█
        // ███∙∙█
        // ██████
        // ██████
        // ██████
        0xfc, 0xfc, 0xe4, 0xe4, 0xfc, 0xe4, 0xe4, 0xfc, 0xfc, 0xfc};
    static tImage Font_0x3a = {image_data_Font_0x3a, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x3b[10] = {
        // ██████
        // ██████
        // ███∙∙█
        // ███∙∙█
        // ██████
        // ███∙∙█
        // ███∙∙█
        // ███∙██
        // ███∙██
        // ████∙█
        0xfc, 0xfc, 0xe4, 0xe4, 0xfc, 0xe4, 0xe4, 0xec, 0xec, 0xf4};
    static tImage Font_0x3b = {image_data_Font_0x3b, 6, 10, 8};
    #endif

    #if (0x0 == 0x0)
    static uint8_t image_data_Font_0x3d[10] = {
        // ██████
        // ██████
        // ██████
        // █∙∙∙∙∙
        // ██████
        // █∙∙∙∙∙
        // ██████
        // ██████
        // ██████
        // ██████
        0xfc, 0xfc, 0xfc, 0x80, 0xfc, 0x80, 0xfc, 0xfc, 0xfc, 0xfc};
    static tImage Font_0x3d = {image_data_Font_0x3d, 6, 10, 8};
    #endif

    class Font
    {
        private:
            std::unordered_map<int, display::tImage> mFontMap = {
                {0x20, Font_0x20},
                {0x2d, Font_0x2d},
                {0x2e, Font_0x2e},
                {0x41, Font_0x41},
                {0x42, Font_0x42},
                {0x43, Font_0x43},
                {0x44, Font_0x44},
                {0x45, Font_0x45},
                {0x46, Font_0x46},
                {0x47, Font_0x47},
                {0x48, Font_0x48},
                {0x49, Font_0x49},
                {0x4a, Font_0x4a},
                {0x4b, Font_0x4b},
                {0x4c, Font_0x4c},
                {0x4d, Font_0x4d},
                {0x4e, Font_0x4e},
                {0x4f, Font_0x4f},
                {0x50, Font_0x50},
                {0x51, Font_0x51},
                {0x52, Font_0x52},
                {0x53, Font_0x53},
                {0x54, Font_0x54},
                {0x55, Font_0x55},
                {0x56, Font_0x56},
                {0x57, Font_0x57},
                {0x58, Font_0x58},
                {0x59, Font_0x59},
                {0x5a, Font_0x5a},
                {0x61, Font_0x61},
                {0x62, Font_0x62},
                {0x63, Font_0x63},
                {0x64, Font_0x64},
                {0x65, Font_0x65},
                {0x66, Font_0x66},
                {0x67, Font_0x67},
                {0x68, Font_0x68},
                {0x69, Font_0x69},
                {0x6a, Font_0x6a},
                {0x6b, Font_0x6b},
                {0x6c, Font_0x6c},
                {0x6d, Font_0x6d},
                {0x6e, Font_0x6e},
                {0x6f, Font_0x6f},
                {0x70, Font_0x70},
                {0x71, Font_0x71},
                {0x72, Font_0x72},
                {0x73, Font_0x73},
                {0x74, Font_0x74},
                {0x75, Font_0x75},
                {0x76, Font_0x76},
                {0x77, Font_0x77},
                {0x78, Font_0x78},
                {0x79, Font_0x79},
                {0x7a, Font_0x7a},
                {0x21, Font_0x21},
                {0x23, Font_0x23},
                {0x25, Font_0x25},
                {0x26, Font_0x26},
                {0x27, Font_0x27},
                {0x28, Font_0x28},
                {0x29, Font_0x29},
                {0x2a, Font_0x2a},
                {0x2b, Font_0x2b},
                {0x2c, Font_0x2c},
                {0x30, Font_0x30},
                {0x31, Font_0x31},
                {0x32, Font_0x32},
                {0x33, Font_0x33},
                {0x34, Font_0x34},
                {0x35, Font_0x35},
                {0x36, Font_0x36},
                {0x37, Font_0x37},
                {0x38, Font_0x38},
                {0x39, Font_0x39},
                {0x3a, Font_0x3a},
                {0x3b, Font_0x3b},
                {0x3d, Font_0x3d}
            };

        public:
            Font();

            tImage* getFontImage(int key);
    };
}
