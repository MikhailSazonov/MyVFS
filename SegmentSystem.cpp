#include "SegmentSystem.hpp"

void TestTask::SegmentSystem::NormalizeSegment(Segment& segment)
{
    if (segment.points_.second < segment.points_.first)
    {
        std::swap(segment.points_.second, segment.points_.first);
    }
}

uint64_t TestTask::SegmentSystem::GetSize(const Segment& segment)
{
    return segment.points_.second - segment.points_.first + 1;
}

void TestTask::SegmentSystem::AddSegment(Segment&& new_segment)
{
    NormalizeSegment(new_segment);
    uint64_t segment_size = GetSize(new_segment);
    auto& segment_set = size_mappings_[segment_size];
    if (segment_set.find(new_segment) != segment_set.end())
    {
        return;
    }
    // Присоединяем сегменты справа
    auto left_addition = left_sides_.find(new_segment.points_.second + 1);
    if (left_addition != left_sides_.end())
    {
        new_segment.points_.second = left_addition->second;
        uint64_t old_size = left_addition->second - left_addition->first + 1;

        auto segment_iter = size_mappings_[old_size].find({{left_addition->first, left_addition->second}});
        new_segment.data_ += segment_iter->data_;
        std::copy(segment_iter->tasks_.begin(), segment_iter->tasks_.end(), std::back_inserter(new_segment.tasks_));

        size_mappings_[old_size].erase(segment_iter);
        left_sides_.erase(left_addition);
    }

    // Слева
    auto right_addition = right_sides_.find(new_segment.points_.first - 1);
    if (right_addition != right_sides_.end())
    {
        new_segment.points_.first = right_addition->second;
        uint64_t old_size = right_addition->first - right_addition->second + 1;

        auto segment_iter = size_mappings_[old_size].find({{right_addition->second, right_addition->first}});
        new_segment.data_ = segment_iter->data_ + new_segment.data_;
        std::copy(segment_iter->tasks_.begin(), segment_iter->tasks_.end(), std::back_inserter(new_segment.tasks_));

        size_mappings_[old_size].erase(segment_iter);
        right_sides_.erase(right_addition);
    }

    uint64_t total_size = GetSize(new_segment);
    left_sides_[new_segment.points_.first] = new_segment.points_.second;
    right_sides_[new_segment.points_.second] = new_segment.points_.first;
    size_mappings_[total_size].insert(new_segment);
}

void TestTask::SegmentSystem::RemoveSegment(Segment&& segment_to_rm)
{
    NormalizeSegment(segment_to_rm);
    auto left_segment_side = left_sides_.find(segment_to_rm.points_.first);
    if (left_segment_side == left_sides_.end())
    {
        return;
    }
    uint64_t left_side = left_segment_side->first;
    uint64_t right_side = left_segment_side->second;
    Segment big_seg{{left_side, right_side}};

    uint64_t size = GetSize(big_seg);

    size_mappings_[size].erase(big_seg);

    left_sides_.erase(left_side);
    if (right_side == segment_to_rm.points_.second)
    {
        right_sides_.erase(right_side);
    }
    else
    {
        Segment new_segment{{segment_to_rm.points_.second + 1, right_side}};
        auto sz = GetSize(new_segment);
        left_sides_[new_segment.points_.first] = new_segment.points_.second;
        right_sides_[new_segment.points_.second] = new_segment.points_.first;
        size_mappings_[sz].insert(new_segment);
    }
}

std::optional<TestTask::Segment> TestTask::SegmentSystem::GetSegmentBySize(uint64_t size, uint64_t idx)
{
    auto segment_iter = size_mappings_[size].begin();
    if (segment_iter == size_mappings_[size].end())
    {
        return std::nullopt;
    }
    for (size_t i = 0; i < idx; ++i)
    {
        ++segment_iter;
        if (segment_iter == size_mappings_[size].end())
        {
            return std::nullopt;
        }
    }
    return *segment_iter;
}

const TestTask::Segment& TestTask::SegmentSystem::GetSegmentByPoints(uint64_t left, uint64_t right) const
{
    uint64_t size = right - left + 1;
    return *size_mappings_.find(size)->second.find({{left, right}});
}
