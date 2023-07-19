#include "SegmentSystem.hpp"

void TestTask::SegmentSystem::NormalizeSegment(std::pair<uint64_t, uint64_t>& segment)
{
    if (segment.second < segment.first)
    {
        std::swap(segment.second, segment.first);
    }
}

uint64_t TestTask::SegmentSystem::GetSize(const std::pair<uint64_t, uint64_t>& segment)
{
    return segment.second - segment.first + 1;
}


void TestTask::SegmentSystem::AddSegment(std::pair<uint64_t, uint64_t> new_segment)
{
    NormalizeSegment(new_segment);
    uint64_t segment_size = GetSize(new_segment);
    auto& segment_set = size_mappings_[segment_size];
    if (segment_set.find(new_segment) != segment_set.end())
    {
        return;
    }
    // Присоединяем сегменты справа
    auto left_addition = left_sides_.find(new_segment.second + 1); 
    if (left_addition != left_sides_.end())
    {
        new_segment.second = left_addition->second;
        uint64_t old_size = GetSize(*left_addition);
        size_mappings_[old_size].erase(*left_addition);
    }
    // Слева
    auto right_addition = right_sides_.find(new_segment.first - 1); 
    if (right_addition != right_sides_.end())
    {
        std::pair<uint64_t, uint64_t> right = *right_addition;
        NormalizeSegment(right);
        new_segment.first = right.first;
        uint64_t old_size = GetSize(right);
        size_mappings_[old_size].erase(right);
    }

    uint64_t total_size = GetSize(new_segment);
    left_sides_[new_segment.first] = new_segment.second;
    right_sides_[new_segment.second] = new_segment.first;
    size_mappings_[total_size].insert(new_segment);
}

void TestTask::SegmentSystem::RemoveSegment(std::pair<uint64_t, uint64_t> segment_to_rm)
{
    NormalizeSegment(segment_to_rm);
    auto left_segment_side = left_sides_.find(segment_to_rm.first);
    if (left_segment_side == left_sides_.end())
    {
        return;
    }
    uint64_t left_side = left_segment_side->first;
    uint64_t right_side = left_segment_side->second;
    uint64_t size = GetSize({left_side, right_side});

    size_mappings_[size].erase({left_side, right_side});

    left_sides_.erase(left_side);
    if (right_side == segment_to_rm.second)
    {
        right_sides_.erase(right_side);
    }
    else
    {
        std::pair<uint64_t, uint64_t> new_segment =
            std::make_pair<uint64_t, uint64_t>(segment_to_rm.second + 1, std::move(right_side));
        auto sz = GetSize(new_segment);
        left_sides_[new_segment.first] = new_segment.second;
        right_sides_[new_segment.second] = new_segment.first;
        size_mappings_[sz].insert(new_segment);
    }
}

std::optional<std::pair<uint64_t, uint64_t>> TestTask::SegmentSystem::GetSegment(uint64_t size)
{
    auto segment_iter = size_mappings_[size].begin();
    if (segment_iter == size_mappings_[size].end())
    {
        return std::nullopt;
    }
    return *segment_iter;
}
