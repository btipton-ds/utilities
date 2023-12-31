/*

This file is part of the VulkanQuickStart Project.

	The VulkanQuickStart Project is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	The VulkanQuickStart Project is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	This link provides the exact terms of the GPL license <https://www.gnu.org/licenses/>.

	The author's interpretation of GPL 3 is that if you receive money for the use or distribution of the TriMesh Library or a derivative product, GPL 3 no longer applies.

	Under those circumstances, the author expects and may legally pursue a reasoble share of the income. To avoid the complexity of agreements and negotiation, the author makes
	no specific demands in this regard. Compensation of roughly 1% of net or $5 per user license seems appropriate, but is not legally binding.

	In lay terms, if you make a profit by using the VulkanQuickStart Project (violating the spirit of Open Source Software), I expect a reasonable share for my efforts.

	Robert R Tipton - Author

	Dark Sky Innovative Solutions http://darkskyinnovation.com/

*/

#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) flat in float fragAmbient;
layout(location = 3) flat in int fragNumLights;
layout(location = 4) flat in vec3 fragLights[8];

layout(location = 0) out vec4 outColor;

void main() {
	float C_PI = radians(180);
    float intensity = 0.0;

	float az0 = 45 / 180 * C_PI;
	float el0 = 30 / 180 * C_PI;
	float az1 = -45 / 180 * C_PI;
	float el1 = 30 / 180 * C_PI;

	float cosAz0 = cos(az0);
	float sinAz0 = sin(az0);
	float cosEl0 = cos(el0);
	float sinEl0 = sin(el0);

	float cosAz1 = cos(az1);
	float sinAz1 = sin(az1);
	float cosEl1 = cos(el1);
	float sinEl1 = sin(el1);

	vec3 lights[2] = {
		vec3(cosEl0 * sinAz0, sinEl0, cosEl0 * cosAz0),
		vec3(cosEl1 * sinAz1, sinEl1, cosEl1 * cosAz1),
	};
	
    for (int i = 0; i < fragNumLights; i++) {
        float dp = abs(dot(lights[i], fragNormal));
		if (dp > 0)
			intensity += dp;
    }

    intensity = min(intensity, 1.0);

    float ambient = fragAmbient;
    intensity = ambient + (1.0 - ambient) * intensity;

    outColor = intensity * vec4(fragColor, 1);
}
