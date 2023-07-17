#pragma once

#include <map>
#include <set>

namespace TestTask
{
    /*
        Класс, с помощью которого мы будем отслеживать освободившиеся чанки и снижать
        фрагментацию файла, а также группировать чанки на запись в файл (либо на чтение)
    */

    class SegmentSystem
    {
        public:
            void AddSegment(const std::pair<size_t, size_t>&);

            std::vector<std::pair<size_t, size_t>> GetSegments(uint64_t);

        private:
            std::map<size_t, std::vector<std::pair<size_t, size_t>>> size_mappings_;
            std::set<std::pair<size_t, size_t>> segments_;
    }   
}
