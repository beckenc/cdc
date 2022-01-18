/**
 * Copyright Claus Beckenbauer 2004 - 2022.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
 */

#include "io/io.hpp"

#include <array>
#include <iostream>

namespace io
{

static auto io_buffer = std::array<char, io::frame_size>{};

void get_data(std::span<char, frame_size> const& output)
{
    // prepare output buffer (usually done by HW) ...
    static auto buffer_fill_cnt = 0;
    io_buffer.fill('a' + (buffer_fill_cnt++ % 26));

    // ... and transfer it to the caller
    std::copy(io_buffer.begin(), io_buffer.end(), output.begin());

    // std::cout << "[io] get_data " << output[0] << "\n";
}

void send_data(std::span<const char, frame_size> const& output)
{
    // std::cout << "[io] send_data " << output[0] << "\n";
}
}  // namespace io