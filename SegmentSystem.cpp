#include "SegmentSystem.hpp"

void TestTask::SegmentSystem::AddSegment(std::pair<uint64_t, uint64_t>& new_segment)
{
    if (new_segment.second < new_segment.first)
    {
        std::swap(new_segment.second, new_segment.first);
    }
    uint64_t segment_size = new_segment.second - new_segment.first;
    auto& segment_set = size_mappings_[segment_size];
    if (segment_set.find(new_segment) != segment_set.end())
    {
        return;
    }
    // Присоединяем сегменты слева
    auto left_addition = left_sides_.find(new_segment.first - 1); 
    if (left_addition != left_sides_.end())
    {
        new_segment.first = left_addition->second.first;
        uint64_t old_size = left_addition->second.second - left_addition->second.first;
        size_mappings_[old_size].erase({left_addition->second.first, left_addition->second.second});
    }
    // Справа
    auto right_addition = right_sides_.find(new_segment.second + 1); 
    if (right_addition != right_sides_.end())
    {
        new_segment.second = right_addition->second.second;
        uint64_t old_size = right_addition->second.second - right_addition->second.first;
        size_mappings_[old_size].erase({right_addition->second.first, right_addition->second.second});
    }

    uint64_t total_size = new_segment.second - new_segment.first;
    left_sides_[new_segment.second] = new_segment.first;
    right_sides_[new_segment.first] = new_segment.second;
    size_mappings_[total_size].insert(new_segment);
}

void TestTask::SegmentSystem::RemoveSegment(const std::pair<uint64_t, uint64_t>& segment_to_rm)
{
    /// TODO
}

std::optional<std::pair<uint64_t, uint64_t>>> TestTask::SegmentSystem::GetSegment(uint64_t size)
{
    auto segment_iter = size_mappings_[size].begin();
    if (segment_iter == size_mappings_[size].end())
    {
        return std::nullopt;
    }
    return *segment_iter;
}
