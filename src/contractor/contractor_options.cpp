#include "contractor/contractor_options.hpp"

#include "util/simple_logger.hpp"
#include "util/version.hpp"

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <tbb/task_scheduler_init.h>

namespace osrm
{
namespace contractor
{

return_code
ContractorOptions::ParseArguments(int argc, char *argv[], ContractorConfig &contractor_config)
{
    // declare a group of options that will be allowed only on command line
    boost::program_options::options_description generic_options("Options");
    generic_options.add_options()("version,v", "Show version")("help,h", "Show this help message")(
        "config,c",
        boost::program_options::value<boost::filesystem::path>(&contractor_config.config_file_path)
            ->default_value("contractor.ini"),
        "Path to a configuration file.");

    // declare a group of options that will be allowed both on command line and in config file
    boost::program_options::options_description config_options("Configuration");
    config_options.add_options()(
        "profile,p",
        boost::program_options::value<boost::filesystem::path>(&contractor_config.profile_path)
            ->default_value("profile.lua"),
        "Path to LUA routing profile")(
        "threads,t",
        boost::program_options::value<unsigned int>(&contractor_config.requested_num_threads)
            ->default_value(tbb::task_scheduler_init::default_num_threads()),
        "Number of threads to use")(
        "core,k",
        boost::program_options::value<double>(&contractor_config.core_factor)->default_value(1.0),
        "Percentage of the graph (in vertices) to contract [0..1]")(
        "segment-speed-file",
        boost::program_options::value<std::string>(&contractor_config.segment_speed_lookup_path),
        "Lookup file containing nodeA,nodeB,speed data to adjust edge weights")(
        "level-cache,o", boost::program_options::value<bool>(&contractor_config.use_cached_priority)
                             ->default_value(false),
        "Use .level file to retain the contaction level for each node from the last run.");

#ifdef DEBUG_GEOMETRY
    config_options.add_options()(
        "debug-geometry",
        boost::program_options::value<std::string>(&contractor_config.debug_geometry_path),
        "Write out edge-weight debugging geometry data in GeoJSON format to this file");
#endif

    // hidden options, will be allowed both on command line and in config file, but will not be
    // shown to the user
    boost::program_options::options_description hidden_options("Hidden options");
    hidden_options.add_options()("input,i", boost::program_options::value<boost::filesystem::path>(
                                                &contractor_config.osrm_input_path),
                                 "Input file in .osm, .osm.bz2 or .osm.pbf format");

    // positional option
    boost::program_options::positional_options_description positional_options;
    positional_options.add("input", 1);

    // combine above options for parsing
    boost::program_options::options_description cmdline_options;
    cmdline_options.add(generic_options).add(config_options).add(hidden_options);

    boost::program_options::options_description config_file_options;
    config_file_options.add(config_options).add(hidden_options);

    boost::program_options::options_description visible_options(
        "Usage: " + boost::filesystem::basename(argv[0]) + " <input.osrm> [options]");
    visible_options.add(generic_options).add(config_options);

    // parse command line options
    boost::program_options::variables_map option_variables;
    boost::program_options::store(boost::program_options::command_line_parser(argc, argv)
                                      .options(cmdline_options)
                                      .positional(positional_options)
                                      .run(),
                                  option_variables);

    const auto &temp_config_path = option_variables["config"].as<boost::filesystem::path>();
    if (boost::filesystem::is_regular_file(temp_config_path))
    {
        boost::program_options::store(boost::program_options::parse_config_file<char>(
                                          temp_config_path.string().c_str(), cmdline_options, true),
                                      option_variables);
    }

    if (option_variables.count("version"))
    {
        util::SimpleLogger().Write() << OSRM_VERSION;
        return return_code::exit;
    }

    if (option_variables.count("help"))
    {
        util::SimpleLogger().Write() << "\n" << visible_options;
        return return_code::exit;
    }

    boost::program_options::notify(option_variables);

    if (!option_variables.count("input"))
    {
        util::SimpleLogger().Write() << "\n" << visible_options;
        return return_code::fail;
    }

    return return_code::ok;
}

void ContractorOptions::GenerateOutputFilesNames(ContractorConfig &contractor_config)
{
    contractor_config.level_output_path = contractor_config.osrm_input_path.string() + ".level";
    contractor_config.core_output_path = contractor_config.osrm_input_path.string() + ".core";
    contractor_config.graph_output_path = contractor_config.osrm_input_path.string() + ".hsgr";
    contractor_config.edge_based_graph_path = contractor_config.osrm_input_path.string() + ".ebg";
    contractor_config.edge_segment_lookup_path =
        contractor_config.osrm_input_path.string() + ".edge_segment_lookup";
    contractor_config.edge_penalty_path =
        contractor_config.osrm_input_path.string() + ".edge_penalties";
}
}
}
