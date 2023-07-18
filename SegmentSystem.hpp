#pragma once

#include <map>
#include <set>
#include <optional>

namespace TestTask
{
    /*
        Класс, с помощью которого мы будем отслеживать освободившиеся чанки и снижать
        фрагментацию файла, а также группировать чанки на запись в файл (либо на чтение)
    */

    class SegmentSystem
    {
        public:
            void AddSegment(std::pair<uint64_t, uint64_t>&);

            void RemoveSegment(const std::pair<uint64_t, uint64_t>&);

            std::optional<std::pair<uint64_t, uint64_t>>> GetSegment(uint64_t);

        private:
            std::map<uint64_t, std::set<std::pair<uint64_t, uint64_t>>> size_mappings_;
            std::map<uint64_t, uint64_t> left_sides_;
            std::map<uint64_t, uint64_t> right_sides_;
    }   
}
