#pragma once

/*
This file is part of the DistFieldHexMesh application/library.

    The DistFieldHexMesh application/library is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    The DistFieldHexMesh application/library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    This link provides the exact terms of the GPL license <https://www.gnu.org/licenses/>.

    The author's interpretation of GPL 3 is that if you receive money for the use or distribution of the DistFieldHexMesh application/library or a derivative product, GPL 3 no longer applies.

    Under those circumstances, the author expects and may legally pursue a reasoble share of the income. To avoid the complexity of agreements and negotiation, the author makes
    no specific demands in this regard. Compensation of roughly 1% of net or $5 per user license seems appropriate, but is not legally binding.

    In lay terms, if you make a profit by using the DistFieldHexMesh application/library (violating the spirit of Open Source Software), I expect a reasonable share for my efforts.

    Robert R Tipton - Author

    Dark Sky Innovative Solutions http://darkskyinnovation.com/
*/

#include <stdint.h>

struct rgbaColor {
//    explicit rgbaColor(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 0xff);
    explicit rgbaColor();
    explicit rgbaColor(float r, float g, float b, float a = 1);

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
    : _rgba{0, 0, 0, 1}
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

rgbaColor curvatureToColor(double cur);
float HueToRGB(float v1, float v2, float vH);
rgbaColor HSVToRGB(float h, float s, float v);
