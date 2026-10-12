#pragma once

#include <cstddef>
#include <cstdint>

extern "C" {

struct RblBeat {
    std::uint32_t time_ms;
    std::uint16_t number;
    std::uint16_t tempo_x100;
};

struct RblColumn {
    std::uint8_t low;
    std::uint8_t mid;
    std::uint8_t high;
    std::uint8_t peak;
};

struct RblResult {
    double bpm;
    std::uint32_t sample_rate;
    double duration_secs;
    const RblBeat* beats;
    std::size_t beats_len;
    const char* key;
    const RblColumn* columns;
    std::size_t columns_len;
    const RblColumn* overview;
    std::size_t overview_len;
};

std::uint32_t rbl_bridge_version();
RblResult* rbl_analyze_path(const char* utf8_path);
void rbl_result_free(RblResult* result);

}
