#pragma once
#include "AppError.h"
#include <optional>

namespace fsnext {

template<typename T>
class ApiResponse {
public:
    static ApiResponse success(T data, int httpCode = 200) {
        ApiResponse r;
        r.m_data = std::move(data);
        r.m_httpCode = httpCode;
        return r;
    }

    static ApiResponse failure(AppError error, int httpCode = 0) {
        ApiResponse r;
        r.m_error = std::move(error);
        r.m_httpCode = httpCode;
        return r;
    }

    bool isSuccess() const { return !m_error.isError(); }
    bool isError() const { return m_error.isError(); }

    const T &data() const { return m_data.value(); }
    T takeData() { return std::move(m_data.value()); }

    const AppError &error() const { return m_error; }
    int httpCode() const { return m_httpCode; }

private:
    std::optional<T> m_data;
    AppError m_error;
    int m_httpCode = 0;
};

// Specialization for void (no data)
template<>
class ApiResponse<void> {
public:
    static ApiResponse success(int httpCode = 200) {
        ApiResponse r;
        r.m_httpCode = httpCode;
        return r;
    }

    static ApiResponse failure(AppError error, int httpCode = 0) {
        ApiResponse r;
        r.m_error = std::move(error);
        r.m_httpCode = httpCode;
        return r;
    }

    bool isSuccess() const { return !m_error.isError(); }
    bool isError() const { return m_error.isError(); }

    const AppError &error() const { return m_error; }
    int httpCode() const { return m_httpCode; }

private:
    AppError m_error;
    int m_httpCode = 0;
};

} // namespace fsnext
