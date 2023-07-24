#include "../MyVFS.hpp"

#include "StandardSerializer.hpp"

#include <iostream>

void TestTask::SerializeVFS(const TestTask::MyVFS& vfs, std::ostream& out)
{
    const auto& files = vfs.files_;
    size_t sz = files.size();
    out.write(reinterpret_cast<const char*>(&sz), sizeof(sz));
    for (const auto& f : files)
    {
        TestTask::Serialize(f.filename_, out);
        TestTask::Serialize(f.full_filename_, out);
        TestTask::Serialize(f.location_.chunks_info_, out);
        TestTask::Serialize(f.manager_idx_, out);
    }

    auto rr_idx = vfs.round_robin_idx.load(std::memory_order_acquire);
    TestTask::Serialize(rr_idx, out);
    TestTask::Serialize(vfs.workers_, out);
    
    const auto& managers = vfs.managers_;
    int k = 0;
    for (const auto& w : managers)
    {
        TestTask::Serialize(w->phys_file_.file_size_, out);

        const auto& seg_sys = w->phys_file_.free_segments_;
        TestTask::Serialize(seg_sys.left_sides_, out);
        TestTask::Serialize(seg_sys.right_sides_, out);
        sz = seg_sys.size_mappings_.size();
        out.write(reinterpret_cast<const char*>(&sz), sizeof(sz));
        for (const auto& seg : seg_sys.size_mappings_)
        {
            TestTask::Serialize(seg.first, out);
            sz = seg.second.size();
            out.write(reinterpret_cast<const char*>(&sz), sizeof(sz));
            for (const auto& segment : seg.second)
            {
                TestTask::Serialize(segment.points_, out);
            }
        }
        ++k;
    }
}

void TestTask::DeserializeVFS(std::istream& in, TestTask::MyVFS& vfs)
{
    size_t sz;
    in.read(reinterpret_cast<char*>(&sz), sizeof(sz));
    for (size_t i = 0; i < sz; ++i)
    {
        std::string filename;
        std::string full_filename;
        TestTask::PhysicalLocation location;
        size_t manager_idx;

        TestTask::Deserialize(in, filename);
        TestTask::Deserialize(in, full_filename);
        TestTask::Deserialize(in, location.chunks_info_);
        TestTask::Deserialize(in, manager_idx);
        vfs.files_.emplace_back(filename, full_filename, location, manager_idx);
    }

    uint32_t rr_idx;
    TestTask::Deserialize(in, rr_idx);
    vfs.round_robin_idx.store(rr_idx, std::memory_order_release);
    TestTask::Deserialize(in, vfs.workers_);
    
    for (size_t i = 0; i < vfs.workers_; ++i)
    {
        size_t f_sz;
        TestTask::Deserialize(in, f_sz);

        SegmentSystem free_seg;
        TestTask::Deserialize(in, free_seg.left_sides_);
        TestTask::Deserialize(in, free_seg.right_sides_);

        std::map<uint64_t, std::set<Segment, SegmentComp>> size_mappings;
        size_t inner_sz;
        in.read(reinterpret_cast<char*>(&inner_sz), sizeof(inner_sz));
        for (size_t j = 0; j < inner_sz; ++j)
        {
            std::pair<uint64_t, std::set<Segment, SegmentComp>> seg_set;
            TestTask::Deserialize(in, seg_set.first);
            size_t set_sz;
            in.read(reinterpret_cast<char*>(&set_sz), sizeof(set_sz));
            for (size_t k = 0; k < set_sz; ++k)
            {
                Segment sg;
                TestTask::Deserialize(in, sg.points_);
                seg_set.second.insert(sg);
            }
            size_mappings.insert(seg_set);
        }
        free_seg.size_mappings_ = size_mappings;
        
        vfs.managers_[i]->phys_file_.free_segments_ = free_seg;
        vfs.managers_[i]->phys_file_.file_size_ = f_sz;
    }
}
