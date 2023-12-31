#pragma once

#include <map>
#include <set>
#include <optional>
#include <cstdint>
#include <utility>
#include <string>
#include <vector>

#include "Journal.hpp"
#include "VFSfwd.hpp"

namespace TestTask
{
    struct Segment
    {
        friend void SerializeSegment(const Segment&, std::ostream&);
        friend void DeserializeSegments(std::istream&, Segment&);

        std::pair<uint64_t, uint64_t> points_;
        std::string data_ = "";
        std::vector<Task*> tasks_ = {};
    };

    struct SegmentComp
    {
        bool operator()(const Segment& left, const Segment& right) const
        {
            return left.points_ < right.points_;
        }
    };

    /*
        Класс, с помощью которого мы будем отслеживать освободившееся место в файле
        для записи новых данных, а также группировать данные
        для их записи в файл/группировать запросы на чтение файлов
    */

    class SegmentSystem
    {
        friend void SerializeVFS(const MyVFS&, std::ostream&);
        friend void DeserializeVFS(std::istream&, MyVFS&);

        public:
            void AddSegment(Segment&&);

            /*
                Негласно договариваемся, что будем убирать части сегментов (или сегменты целиком)
                находящиеся с левого конца
            */
            void RemoveSegment(Segment&&);

            std::optional<Segment> GetSegmentBySize(uint64_t, uint64_t idx = 0);

            const Segment& GetSegmentByPoints(uint64_t, uint64_t) const;

            inline const std::map<uint64_t, std::set<Segment, SegmentComp>>& GetAllSegments()
            {
                return size_mappings_;
            }

            inline const std::map<uint64_t, uint64_t>& GetAllSegmentsSorted() const
            {
                return left_sides_;
            }

        private:
            void NormalizeSegment(Segment&);

            uint64_t GetSize(const Segment&);

        private:
            std::map<uint64_t, std::set<Segment, SegmentComp>> size_mappings_;
            std::map<uint64_t, uint64_t> left_sides_;
            std::map<uint64_t, uint64_t> right_sides_;
    };
}
