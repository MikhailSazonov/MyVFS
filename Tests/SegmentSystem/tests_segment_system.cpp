#include <SegmentSystem.hpp>

#include <test_utils.hpp>

#include <iostream>

using namespace TestTask;

int main() {
    section("segment_system");
    test([&]() {
        SegmentSystem s;
        Segment seg{{1, 4}, "aaaa"};
        s.AddSegment(std::move(seg));
        auto found_seg = s.GetSegmentBySize(4);
        assert(found_seg.has_value());
        assert_equal(found_seg->points_.first, 1ull);
        assert_equal(found_seg->points_.second, 4ull);
        assert_equal(found_seg->data_, "aaaa");
        s.RemoveSegment({{1, 4}});
        found_seg = s.GetSegmentBySize(4);
        assert(!found_seg.has_value());
        const auto& segs = s.GetAllSegments();
        assert(segs.find(4)->second.empty());
    }, "Just works");

    test([&]() {
        SegmentSystem s;
        std::string str1 = "ababababab";
        std::string str2 = "qqqqqq";
        std::string str3 = "eeeee";
        std::string str4 = "wwww";
        std::string str5 = "ttttt";

        s.AddSegment({{11, 20}, str1});

        auto found_seg = s.GetSegmentBySize(10);
        assert(found_seg.has_value());
        assert_equal(found_seg->points_.first, 11ull);
        assert_equal(found_seg->points_.second, 20ull);
        assert_equal(found_seg->data_, str1);

        s.AddSegment({{5, 10}, str2});

        found_seg = s.GetSegmentBySize(16);
        assert(found_seg.has_value());
        assert_equal(found_seg->points_.first, 5ull);
        assert_equal(found_seg->points_.second, 20ull);
        assert_equal(found_seg->data_, str2 + str1);

        s.AddSegment({{21, 25}, str3});

        found_seg = s.GetSegmentBySize(21);
        assert(found_seg.has_value());
        assert_equal(found_seg->points_.first, 5ull);
        assert_equal(found_seg->points_.second, 25ull);
        assert_equal(found_seg->data_, str2 + str1 + str3);

        s.AddSegment({{1, 4}, str4});
        s.AddSegment({{26, 30}, str5});

        found_seg = s.GetSegmentBySize(30);
        assert(found_seg.has_value());
        assert_equal(found_seg->points_.first, 1ull);
        assert_equal(found_seg->points_.second, 30ull);
        assert_equal(found_seg->data_, str4 + str2 + str1 + str3 + str5);

    }, "Merging");

    test([&]() {
        SegmentSystem s;
        s.AddSegment({{1, 5}});
        s.AddSegment({{11, 16}});
        s.AddSegment({{21, 27}});

        auto found_seg = s.GetSegmentBySize(5);
        assert(found_seg.has_value());
        assert_equal(found_seg->points_.first, 1ull);
        assert_equal(found_seg->points_.second, 5ull);

        found_seg = s.GetSegmentBySize(6);
        assert(found_seg.has_value());
        assert_equal(found_seg->points_.first, 11ull);
        assert_equal(found_seg->points_.second, 16ull);
        
        found_seg = s.GetSegmentBySize(7);
        assert(found_seg.has_value());
        assert_equal(found_seg->points_.first, 21ull);
        assert_equal(found_seg->points_.second, 27ull);

        s.RemoveSegment({{1, 3}});

        found_seg = s.GetSegmentBySize(5);
        assert(!found_seg.has_value());

        found_seg = s.GetSegmentBySize(2);
        assert(found_seg.has_value());
        assert_equal(found_seg->points_.first, 4ull);
        assert_equal(found_seg->points_.second, 5ull);

        s.RemoveSegment({{11, 11}});

        found_seg = s.GetSegmentBySize(5);
        assert(found_seg.has_value());
        assert_equal(found_seg->points_.first, 12ull);
        assert_equal(found_seg->points_.second, 16ull);

    }, "Advanced #1");

    test([&]() {
        SegmentSystem s;
        s.AddSegment({{1, 10}});

        auto found_seg = s.GetSegmentBySize(10);
        assert(found_seg.has_value());
        assert_equal(found_seg->points_.first, 1ull);
        assert_equal(found_seg->points_.second, 10ull);

        s.RemoveSegment({{1, 3}});

        found_seg = s.GetSegmentBySize(5);
        assert(!found_seg.has_value());

        found_seg = s.GetSegmentBySize(7);
        assert(found_seg.has_value());
        assert_equal(found_seg->points_.first, 4ull);
        assert_equal(found_seg->points_.second, 10ull);

        s.RemoveSegment({{4, 7}});

        found_seg = s.GetSegmentBySize(3);
        assert(found_seg.has_value());
        assert_equal(found_seg->points_.first, 8ull);
        assert_equal(found_seg->points_.second, 10ull);
        
        s.RemoveSegment({{8, 10}});

        const auto& segs = s.GetAllSegments();
        assert(segs.find(4)->second.empty());

        for (const auto& seg_of_size : segs)
        {
            assert(seg_of_size.second.empty());
        }

    }, "Advanced #2");
}
