#ifndef EXTRACTION_HELPER_FUNCTIONS_HPP
#define EXTRACTION_HELPER_FUNCTIONS_HPP

#include "util/cast.hpp"
#include "util/iso_8601_duration_parser.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string_regex.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/regex.hpp>

#include <limits>
#include <string>

namespace osrm
{
namespace extractor
{

bool simple_duration_is_valid(const std::string &s)
{
    boost::regex simple_format(
        "((\\d|\\d\\d):(\\d|\\d\\d):(\\d|\\d\\d))|((\\d|\\d\\d):(\\d|\\d\\d))|(\\d|\\d\\d)",
        boost::regex_constants::icase | boost::regex_constants::perl);

    const bool simple_matched = regex_match(s, simple_format);

    if (simple_matched)
    {
        return true;
    }
    return false;
}

bool iso_8601_duration_is_valid(const std::string &s)
{
    util::iso_8601_grammar<std::string::const_iterator> iso_parser;
    const bool result = boost::spirit::qi::parse(s.begin(), s.end(), iso_parser);

    // check if the was an error with the request
    if (result && (0 != iso_parser.get_duration()))
    {
        return true;
    }
    return false;
}

bool durationIsValid(const std::string &s)
{
    return simple_duration_is_valid(s) || iso_8601_duration_is_valid(s);
}

unsigned parseDuration(const std::string &s)
{
    if (simple_duration_is_valid(s))
    {
        unsigned hours = 0;
        unsigned minutes = 0;
        unsigned seconds = 0;
        boost::regex e(
            "((\\d|\\d\\d):(\\d|\\d\\d):(\\d|\\d\\d))|((\\d|\\d\\d):(\\d|\\d\\d))|(\\d|\\d\\d)",
            boost::regex_constants::icase | boost::regex_constants::perl);

        std::vector<std::string> result;
        boost::algorithm::split_regex(result, s, boost::regex(":"));
        const bool matched = regex_match(s, e);
        if (matched)
        {
            if (1 == result.size())
            {
                minutes = std::stoul(result[0]);
            }
            if (2 == result.size())
            {
                minutes = std::stoul(result[1]);
                hours = std::stoul(result[0]);
            }
            if (3 == result.size())
            {
                seconds = std::stoul(result[2]);
                minutes = std::stoul(result[1]);
                hours = std::stoul(result[0]);
            }
            return (3600 * hours + 60 * minutes + seconds);
        }
    }
    else if (iso_8601_duration_is_valid(s))
    {
        util::iso_8601_grammar<std::string::const_iterator> iso_parser;
        boost::spirit::qi::parse(s.begin(), s.end(), iso_parser);

        return iso_parser.get_duration();
    }

    return std::numeric_limits<unsigned>::max();
}
}
}

#endif // EXTRACTION_HELPER_FUNCTIONS_HPP
