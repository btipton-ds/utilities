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

	Copyright Robert R Tipton, 2022, all rights reserved except those granted in prior license statement.

	Dark Sky Innovative Solutions http://darkskyinnovation.com/
*/

#include <rgbaColor.h>
#include <cmath>

float HueToRGB(float v1, float v2, float vH) {
    if (vH < 0)
        vH += 1;

    if (vH > 1)
        vH -= 1;

    if ((6 * vH) < 1)
        return (v1 + (v2 - v1) * 6 * vH);

    if ((2 * vH) < 1)
        return v2;

    if ((3 * vH) < 2)
        return (v1 + (v2 - v1) * ((2.0f / 3) - vH) * 6);

    return v1;
}

rgbaColor HSVToRGB(float h, float s, float v) {
    float C = s * v;
    float X = (float) (C * (1 - fabs(fmod(h / 60.0f, 2) - 1)));
    float m = v - C;
    float r, g, b;
    if (h >= 0 && h < 60) {
        r = C, g = X, b = 0;
    }
    else if (h >= 60 && h < 120) {
        r = X, g = C, b = 0;
    }
    else if (h >= 120 && h < 180) {
        r = 0, g = C, b = X;
    }
    else if (h >= 180 && h < 240) {
        r = 0, g = X, b = C;
    }
    else if (h >= 240 && h < 300) {
        r = X, g = 0, b = C;
    }
    else {
        r = C, g = 0, b = X;
    }
    uint8_t R = (uint8_t)((r + m) * 255);
    uint8_t G = (uint8_t)((g + m) * 255);
    uint8_t B = (uint8_t)((b + m) * 255);
    return rgbaColor(r + m, g + m, b + m, 1);
}

rgbaColor curvatureToColor(double curvature)
{
    const float maxRadius = 1.0f; // meter
    const float minRadius = 0.0001f; // meter
    const float logBase = log(5.0f);
    const float maxLogRadius = log(maxRadius) / logBase;
    const float minLogRadius = log(minRadius) / logBase;

    if (curvature < 0 || curvature > 1 / minRadius)
        return rgbaColor(1, 0, 0);
    else if (curvature < 1.0e-4)
        return rgbaColor(0, 0, 0);


    float radius = 1 / (float)curvature;
    float logRadius = log(radius) / logBase;
    float t = (logRadius - minLogRadius) / (maxLogRadius - minLogRadius);
    if (t < 0)
        t = 0;
    else if (t > 1)
        t = 1;

//    t = pow(t, 0.5);

    t = 1 - t;
    float hue = 2 / 3.0f - 2 / 3.0f * t;
    hue = 360 * hue;
    return HSVToRGB(hue, 1, 1);
}
