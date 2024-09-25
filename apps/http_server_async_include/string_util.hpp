#pragma once
#ifndef SERVER_ASYNC_STRING_UTIL_H
#define SERVER_ASYNC_STRING_UTIL_H

#include <string>
#include <algorithm>
#include <chrono>
#include <date/date.h>

namespace server_async
{
    // Case-insensitive character comparison
    struct case_insensitive
    {
        bool operator()(char a, char b) const
        {
            return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
        }
    };

    // Check if content type indicates multipart form data
    inline bool is_multipart_form_data(const std::string &content_type)
    {
        const std::string search_str = "multipart/form-data";
        return std::search(content_type.begin(), content_type.end(),
                           search_str.begin(), search_str.end(),
                           case_insensitive{}) != content_type.end();
    }

    /**
     * @brief Extract the next segment of a path
     * /a/b/c, first invoke will return "a", second "b", third "c", fourth empty string.
     * sv == "svalue" or sv.compare("svalue") == 0 to test equality.
     * sv.empty() means no more segment.
     *
     * @param path The path to extract the segment from
     * @param pos The position in the path to start extracting from, may change after invoke.
     *
     */
    std::string_view next_segment(std::string const &path, size_t &pos)
    {
        if (path[pos] != '/' || pos >= path.length())
            return std::string_view{};
        size_t start_pos = pos + 1;
        pos = path.find('/', start_pos) == std::string::npos
                  ? path.length()
                  : path.find('/', start_pos);
        return std::string_view{path.data() + start_pos, pos - start_pos};
    }

    /**
     * @brief Extract all segments of a path
     * /a/b/c, return ["a", "b", "c"]
     *
     * @param path The path to extract the segments from
     * @return std::vector<std::string_view> The segments of the path
     */
    std::vector<std::string_view> all_segments(std::string const &path)
    {
        std::vector<std::string_view> segments;
        size_t pos = 0;
        while (pos < path.length())
        {
            std::string_view seg = next_segment(path, pos);
            if (!seg.empty())
                segments.push_back(seg);
        }
        return segments;
    }

    template <class Precision>
    std::string getISOCurrentTimestamp()
    {
        auto now = std::chrono::system_clock::now();
        return date::format("%FT%TZ", date::floor<Precision>(now)); // ISO 8601 format
    }
}

#endif