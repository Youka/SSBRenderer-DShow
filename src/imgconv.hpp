/*
Project: SSBRenderer-DShow
File: imgconv.hpp

Copyright (c) 2013, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

    The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include <vector>

// Inverts frame vertically (copied from SSBRenderer Aegisub interface)
inline void frame_flip_y(unsigned char* data, long int pitch, int height){
    // Row buffer
    std::vector<unsigned char> temp_row(pitch);
    // Data last row
    unsigned char* data_end = data + (height - 1) * pitch;
    // Copy inverted from old to new
    for(int y = 0; y < height >> 1; ++y){
        std::copy(data, data+pitch, temp_row.begin());
        std::copy(data_end, data_end+pitch, data);
        std::copy(temp_row.begin(), temp_row.end(), data_end);
        data += pitch;
        data_end -= pitch;
    }
}
