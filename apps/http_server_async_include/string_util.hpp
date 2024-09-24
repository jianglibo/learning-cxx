#pragma once
#ifndef SERVER_ASYNC_STRING_UTIL_H
#define SERVER_ASYNC_STRING_UTIL_H

#include <string>
#include <algorithm>

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
}

#endif