#pragma once

struct rgbaColor {
//    explicit rgbaColor(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 0xff);
    explicit rgbaColor();
    explicit rgbaColor(float r, float g, float b, float a = 0xff);

    unsigned int asUInt() const;

    union {
        unsigned int _rgbaVal;
        uint8_t _rgba[4];
    };
};

inline unsigned int rgbaColor::asUInt() const
{
    return _rgbaVal;
}

/*

inline rgbaColor::rgbaColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    : _rgba{ r, g, b, a }
{}
*/

inline rgbaColor::rgbaColor()
    : _rgba{0, 0, 0, 0xff}
{

}

inline rgbaColor::rgbaColor(float r, float g, float b, float a)
    : _rgba{
        (uint8_t)(r * 0xff + 0.5f),
        (uint8_t)(g * 0xff + 0.5f),
        (uint8_t)(b * 0xff + 0.5f),
        (uint8_t)(a * 0xff + 0.5f)
    }
{}
