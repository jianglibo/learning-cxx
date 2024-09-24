#pragma once
#ifndef SERVER_ASYNC_MODELS_H
#define SERVER_ASYNC_MODELS_H

#include <string>
#include <vector>
#include <optional> // For std::optional
#include <boost/json.hpp>

// Aliases for convenience
namespace json = boost::json;

namespace server_async
{
    // Struct to represent an individual error
    struct Error
    {
        std::string field;      // The field that caused the error
        std::string error_code; // A unique code for the error
        std::string message;    // A human-readable error message
                                // Define to_json for Error struct
    };

    // Struct to represent a successful response data
    struct ResponseData
    {
        std::string task_id;
    };

    // Main response struct
    struct ApiResponse
    {
        std::string status;   // "success" or "error"
        std::string message;  // Description of the result
        std::string begin_at; //
        std::string end_at;   //

        // Constructor for success
        ApiResponse(const std::string &status, const std::string &message, const std::string &begin_at, const std::string &end_at)
            : status(status), message(message), begin_at(begin_at), end_at(end_at) {}
        // Default constructor
        ApiResponse() = default;
    };

    struct ApiResponseSuccess : public ApiResponse
    {
        ResponseData data;
        ApiResponseSuccess(const std::string &message, const std::string &begin_at, const std::string &end_at, const ResponseData &responseData)
            : ApiResponse("success", message, begin_at, end_at), data(std::move(responseData)) {}
    };

    struct ApiResponseError : public ApiResponse
    {
        std::vector<Error> errors; // Optional list of errors for failed responses
        ApiResponseError(const std::string &message, const std::string &begin_at, const std::string &end_at, const std::vector<Error> &errorList)
            : ApiResponse("error", message, begin_at, end_at), errors(std::move(errorList)) {}
    };

    // Serialize the `Error` struct
    void tag_invoke(json::value_from_tag, json::value &jv, const Error &e)
    {
        jv = {
            {"field", e.field},
            {"error_code", e.error_code},
            {"message", e.message}};
    }

    // Serialize the `ResponseData` struct
    void tag_invoke(json::value_from_tag, json::value &jv, const ResponseData &d)
    {
        jv = {
            {"task_id", d.task_id}};
    }

    // Serialize the `ApiResponse` struct
    void tag_invoke(json::value_from_tag, json::value &jv, const ApiResponseSuccess &res)
    {
        jv = {
            {"status", res.status},
            {"message", res.message},
            {"begin_at", res.begin_at},
            {"end_at", res.end_at},
            {"data", json::value_from(res.data)}};
        // jv.as_object()["data"] = res.data;
    }
    void tag_invoke(json::value_from_tag, json::value &jv, const ApiResponseError &res)
    {
        jv = {
            {"status", res.status},
            {"message", res.message},
            {"begin_at", res.begin_at},
            {"end_at", res.end_at},
            {"errors", json::value_from(res.errors)}};
    }

    // Deserialization helpers
    // ApiResponse tag_invoke(json::value_to_tag<ApiResponse> &, const json::value &jv)
    // {
    //     ApiResponse res;
    //     res.status = json::value_to<std::string>(jv.at("status"));
    //     res.message = json::value_to<std::string>(jv.at("message"));
    //     res.begin_at = json::value_to<std::string>(jv.at("begin_at"));
    //     res.end_at = json::value_to<std::string>(jv.at("end_at"));
    //     res.data = json::value_to<std::optional<ResponseData>>(jv.at("data"));
    //     res.errors = json::value_to<std::optional<std::vector<Error>>>(jv.at("errors"));
    //     return res;
    // }
}

#endif