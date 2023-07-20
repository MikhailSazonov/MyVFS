#pragma once

#include <stdexcept>

namespace TestTask
{
    class FileNotFoundError : public std::exception
    {
        const char* what() noexcept
        {
            return "File not found.";
        }
    };

    class FileClosedError : public std::exception
    {
        const char* what() noexcept
        {
            return "File already closed.";
        }
    };

    class OpenStorageError : public std::exception
    {
        const char* what() noexcept
        {
            return "Cannot open the storage.";
        }
    };
}
