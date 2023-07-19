#include <SegmentSystem.hpp>

#include <test_utils.hpp>

#include <iostream>

using namespace TestTask;

int main() {
    section("segment_system");
    test([&]() {
        SegmentSystem s;
        std::pair<uint64_t, uint64_t> seg(1, 4);
        s.AddSegment(seg);
        auto found_seg = s.GetSegment(4);
        assert(found_seg.has_value());
        assert_equal(found_seg->first, 1ull);
        assert_equal(found_seg->second, 4ull);
        s.RemoveSegment(seg);
        found_seg = s.GetSegment(4);
        assert(!found_seg.has_value());
        const auto& segs = s.GetAllSegments();
        assert(segs.find(4)->second.empty());
    }, "Just works");

    test([&]() {
        SegmentSystem s;
        s.AddSegment({11, 20});

        auto found_seg = s.GetSegment(10);
        assert(found_seg.has_value());
        assert_equal(found_seg->first, 11ull);
        assert_equal(found_seg->second, 20ull);

        s.AddSegment({5, 10});

        found_seg = s.GetSegment(16);
        assert(found_seg.has_value());
        assert_equal(found_seg->first, 5ull);
        assert_equal(found_seg->second, 20ull);

        s.AddSegment({21, 25});

        found_seg = s.GetSegment(21);
        assert(found_seg.has_value());
        assert_equal(found_seg->first, 5ull);
        assert_equal(found_seg->second, 25ull);

        s.AddSegment({1, 4});
        s.AddSegment({26, 30});

        found_seg = s.GetSegment(30);
        assert(found_seg.has_value());
        assert_equal(found_seg->first, 1ull);
        assert_equal(found_seg->second, 30ull);

    }, "Merging");

    test([&]() {
        SegmentSystem s;
        s.AddSegment({1, 5});
        s.AddSegment({11, 16});
        s.AddSegment({21, 27});

        auto found_seg = s.GetSegment(5);
        assert(found_seg.has_value());
        assert_equal(found_seg->first, 1ull);
        assert_equal(found_seg->second, 5ull);

        found_seg = s.GetSegment(6);
        assert(found_seg.has_value());
        assert_equal(found_seg->first, 11ull);
        assert_equal(found_seg->second, 16ull);
        
        found_seg = s.GetSegment(7);
        assert(found_seg.has_value());
        assert_equal(found_seg->first, 21ull);
        assert_equal(found_seg->second, 27ull);

        s.RemoveSegment({1, 3});

        found_seg = s.GetSegment(5);
        assert(!found_seg.has_value());

        found_seg = s.GetSegment(2);
        assert(found_seg.has_value());
        assert_equal(found_seg->first, 4ull);
        assert_equal(found_seg->second, 5ull);

        s.RemoveSegment({11, 11});

        found_seg = s.GetSegment(5);
        assert(found_seg.has_value());
        assert_equal(found_seg->first, 12ull);
        assert_equal(found_seg->second, 16ull);

    }, "Advanced #1");

    test([&]() {
        SegmentSystem s;
        s.AddSegment({1, 10});

        auto found_seg = s.GetSegment(10);
        assert(found_seg.has_value());
        assert_equal(found_seg->first, 1ull);
        assert_equal(found_seg->second, 10ull);

        s.RemoveSegment({1, 3});

        found_seg = s.GetSegment(5);
        assert(!found_seg.has_value());

        found_seg = s.GetSegment(7);
        assert(found_seg.has_value());
        assert_equal(found_seg->first, 4ull);
        assert_equal(found_seg->second, 10ull);

        s.RemoveSegment({4, 7});

        found_seg = s.GetSegment(3);
        assert(found_seg.has_value());
        assert_equal(found_seg->first, 8ull);
        assert_equal(found_seg->second, 10ull);
        
        s.RemoveSegment({8, 10});

        const auto& segs = s.GetAllSegments();
        assert(segs.find(4)->second.empty());

        for (const auto& seg_of_size : segs)
        {
            assert(seg_of_size.second.empty());
        }

    }, "Advanced #2");
}
