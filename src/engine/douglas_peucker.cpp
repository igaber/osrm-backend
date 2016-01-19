#include "engine/douglas_peucker.hpp"
#include "util/coordinate_calculation.hpp"

#include <boost/assert.hpp>
#include "osrm/coordinate.hpp"

#include <cmath>
#include <algorithm>
#include <iterator>
#include <stack>
#include <utility>

namespace osrm
{
namespace engine
{

namespace
{
struct CoordinatePairCalculator
{
    CoordinatePairCalculator() = delete;
    CoordinatePairCalculator(const util::FixedPointCoordinate &coordinate_a,
                             const util::FixedPointCoordinate &coordinate_b)
    {
        // initialize distance calculator with two fixed coordinates a, b
        first_lat = (coordinate_a.lat / COORDINATE_PRECISION) * util::RAD;
        first_lon = (coordinate_a.lon / COORDINATE_PRECISION) * util::RAD;
        second_lat = (coordinate_b.lat / COORDINATE_PRECISION) * util::RAD;
        second_lon = (coordinate_b.lon / COORDINATE_PRECISION) * util::RAD;
    }

    int operator()(util::FixedPointCoordinate &other) const
    {
        // set third coordinate c
        const float float_lat1 = (other.lat / COORDINATE_PRECISION) * util::RAD;
        const float float_lon1 = (other.lon / COORDINATE_PRECISION) * util::RAD;

        // compute distance (a,c)
        const float x_value_1 = (first_lon - float_lon1) * cos((float_lat1 + first_lat) / 2.f);
        const float y_value_1 = first_lat - float_lat1;
        const float dist1 = std::hypot(x_value_1, y_value_1) * util::EARTH_RADIUS;

        // compute distance (b,c)
        const float x_value_2 = (second_lon - float_lon1) * cos((float_lat1 + second_lat) / 2.f);
        const float y_value_2 = second_lat - float_lat1;
        const float dist2 = std::hypot(x_value_2, y_value_2) * util::EARTH_RADIUS;

        // return the minimum
        return static_cast<int>(std::min(dist1, dist2));
    }

    float first_lat;
    float first_lon;
    float second_lat;
    float second_lon;
};
}

void douglasPeucker(std::vector<SegmentInformation>::iterator begin,
                    std::vector<SegmentInformation>::iterator end,
                    const unsigned zoom_level)
{
    using Iter = decltype(begin);
    using GeometryRange = std::pair<Iter, Iter>;

    std::stack<GeometryRange> recursion_stack;

    const auto size = std::distance(begin, end);
    if (size < 2)
    {
        return;
    }

    begin->necessary = true;
    std::prev(end)->necessary = true;

    {
        BOOST_ASSERT_MSG(zoom_level < detail::DOUGLAS_PEUCKER_THRESHOLDS_SIZE,
                         "unsupported zoom level");
        auto left_border = begin;
        auto right_border = std::next(begin);
        // Sweep over array and identify those ranges that need to be checked
        do
        {
            // traverse list until new border element found
            if (right_border->necessary)
            {
                // sanity checks
                BOOST_ASSERT(left_border->necessary);
                BOOST_ASSERT(right_border->necessary);
                recursion_stack.emplace(left_border, right_border);
                left_border = right_border;
            }
            ++right_border;
        } while (right_border != end);
    }

    // mark locations as 'necessary' by divide-and-conquer
    while (!recursion_stack.empty())
    {
        // pop next element
        const GeometryRange pair = recursion_stack.top();
        recursion_stack.pop();
        // sanity checks
        BOOST_ASSERT_MSG(pair.first->necessary, "left border must be necessary");
        BOOST_ASSERT_MSG(pair.second->necessary, "right border must be necessary");
        BOOST_ASSERT_MSG(std::distance(pair.second, end) > 0, "right border outside of geometry");
        BOOST_ASSERT_MSG(std::distance(pair.first, pair.second) >= 0,
                         "left border on the wrong side");

        int max_int_distance = 0;
        auto farthest_entry_it = pair.second;
        const CoordinatePairCalculator dist_calc(pair.first->location, pair.second->location);

        // sweep over range to find the maximum
        for (auto it = std::next(pair.first); it != pair.second; ++it)
        {
            const int distance = dist_calc(it->location);
            // found new feasible maximum?
            if (distance > max_int_distance &&
                distance > detail::DOUGLAS_PEUCKER_THRESHOLDS[zoom_level])
            {
                farthest_entry_it = it;
                max_int_distance = distance;
            }
        }

        // check if maximum violates a zoom level dependent threshold
        if (max_int_distance > detail::DOUGLAS_PEUCKER_THRESHOLDS[zoom_level])
        {
            //  mark idx as necessary
            farthest_entry_it->necessary = true;
            if (1 < std::distance(pair.first, farthest_entry_it))
            {
                recursion_stack.emplace(pair.first, farthest_entry_it);
            }
            if (1 < std::distance(farthest_entry_it, pair.second))
            {
                recursion_stack.emplace(farthest_entry_it, pair.second);
            }
        }
    }
}
} // ns engine
} // ns osrm
