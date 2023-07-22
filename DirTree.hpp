#pragma once

#include <deque>
#include <string>

#include "File.hpp"

namespace TestTask
{
    struct Dir
    {
        void AddFile(File*);

        Dir* AddDir(const std::string&);

        Dir* FindDir(const std::string&);

        void Clear();

        std::string name_;
        std::deque<File*> files_{};
        std::deque<Dir*> dirs_{};
    };
}
