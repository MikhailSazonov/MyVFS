#pragma once

#include <map>
#include <set>
#include <optional>
#include <cstdint>
#include <utility>

namespace TestTask
{
    /*
        Класс, с помощью которого мы будем отслеживать освободившееся место в файле
        для записи новых данных, а также группировать данные для их записи в файл
    */

    class SegmentSystem
    {
        public:
            void AddSegment(std::pair<uint64_t, uint64_t>);

            /*
                Негласно договариваемся, что будем убирать части сегментов (или сегменты целиком)
                находящиеся с левого конца
            */
            void RemoveSegment(std::pair<uint64_t, uint64_t>);

            std::optional<std::pair<uint64_t, uint64_t>> GetSegment(uint64_t);

            inline const std::map<uint64_t, std::set<std::pair<uint64_t, uint64_t>>>& GetAllSegments()
            {
                return size_mappings_;
            }

        private:
            void NormalizeSegment(std::pair<uint64_t, uint64_t>&);

            uint64_t GetSize(const std::pair<uint64_t, uint64_t>&);

        private:
            std::map<uint64_t, std::set<std::pair<uint64_t, uint64_t>>> size_mappings_;
            std::map<uint64_t, uint64_t> left_sides_;
            std::map<uint64_t, uint64_t> right_sides_;
    };   
}
