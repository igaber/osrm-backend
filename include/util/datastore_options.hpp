#ifndef DATASTORE_OPTIONS_HPP
#define DATASTORE_OPTIONS_HPP

#include "util/version.hpp"
#include "util/ini_file.hpp"
#include "util/osrm_exception.hpp"
#include "util/simple_logger.hpp"

#include <boost/any.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <string>
#include <unordered_map>

namespace osrm
{
namespace util
{

// generate boost::program_options object for the routing part
bool GenerateDataStoreOptions(const int argc,
                              const char *argv[],
                              std::unordered_map<std::string, boost::filesystem::path> &paths)
{
    // declare a group of options that will be allowed only on command line
    boost::program_options::options_description generic_options("Options");
    generic_options.add_options()("version,v", "Show version")("help,h", "Show this help message")(
        "springclean,s", "Remove all regions in shared memory")(
        "config,c", boost::program_options::value<boost::filesystem::path>(&paths["config"])
                        ->default_value("server.ini"),
        "Path to a configuration file");

    // declare a group of options that will be allowed both on command line
    // as well as in a config file
    boost::program_options::options_description config_options("Configuration");
    config_options.add_options()(
        "hsgrdata", boost::program_options::value<boost::filesystem::path>(&paths["hsgrdata"]),
        ".hsgr file")("nodesdata",
                      boost::program_options::value<boost::filesystem::path>(&paths["nodesdata"]),
                      ".nodes file")(
        "edgesdata", boost::program_options::value<boost::filesystem::path>(&paths["edgesdata"]),
        ".edges file")("geometry",
                       boost::program_options::value<boost::filesystem::path>(&paths["geometry"]),
                       ".geometry file")(
        "ramindex", boost::program_options::value<boost::filesystem::path>(&paths["ramindex"]),
        ".ramIndex file")(
        "fileindex", boost::program_options::value<boost::filesystem::path>(&paths["fileindex"]),
        ".fileIndex file")("core",
                           boost::program_options::value<boost::filesystem::path>(&paths["core"]),
                           ".core file")(
        "namesdata", boost::program_options::value<boost::filesystem::path>(&paths["namesdata"]),
        ".names file")("timestamp",
                       boost::program_options::value<boost::filesystem::path>(&paths["timestamp"]),
                       ".timestamp file");

    // hidden options, will be allowed both on command line and in config
    // file, but will not be shown to the user
    boost::program_options::options_description hidden_options("Hidden options");
    hidden_options.add_options()(
        "base,b", boost::program_options::value<boost::filesystem::path>(&paths["base"]),
        "base path to .osrm file");

    // positional option
    boost::program_options::positional_options_description positional_options;
    positional_options.add("base", 1);

    // combine above options for parsing
    boost::program_options::options_description cmdline_options;
    cmdline_options.add(generic_options).add(config_options).add(hidden_options);

    boost::program_options::options_description config_file_options;
    config_file_options.add(config_options).add(hidden_options);

    boost::program_options::options_description visible_options(
        boost::filesystem::basename(argv[0]) + " [<options>] <configuration>");
    visible_options.add(generic_options).add(config_options);

    // parse command line options
    boost::program_options::variables_map option_variables;
    boost::program_options::store(boost::program_options::command_line_parser(argc, argv)
                                      .options(cmdline_options)
                                      .positional(positional_options)
                                      .run(),
                                  option_variables);

    if (option_variables.count("version"))
    {
        SimpleLogger().Write() << OSRM_VERSION;
        return false;
    }

    if (option_variables.count("help"))
    {
        SimpleLogger().Write() << visible_options;
        return false;
    }

    boost::program_options::notify(option_variables);

    const bool parameter_present =
        (paths.find("hsgrdata") != paths.end() &&
         !paths.find("hsgrdata")->second.string().empty()) ||
        (paths.find("nodesdata") != paths.end() &&
         !paths.find("nodesdata")->second.string().empty()) ||
        (paths.find("edgesdata") != paths.end() &&
         !paths.find("edgesdata")->second.string().empty()) ||
        (paths.find("geometry") != paths.end() &&
         !paths.find("geometry")->second.string().empty()) ||
        (paths.find("ramindex") != paths.end() &&
         !paths.find("ramindex")->second.string().empty()) ||
        (paths.find("fileindex") != paths.end() &&
         !paths.find("fileindex")->second.string().empty()) ||
        (paths.find("core") != paths.end() && !paths.find("core")->second.string().empty()) ||
        (paths.find("timestamp") != paths.end() &&
         !paths.find("timestamp")->second.string().empty());

    if (parameter_present)
    {
        if ((paths.find("config") != paths.end() &&
             boost::filesystem::is_regular_file(paths.find("config")->second)) ||
            option_variables.count("base"))
        {
            SimpleLogger().Write(logWARNING) << "conflicting parameters";
            SimpleLogger().Write() << visible_options;
            return false;
        }
    }

    // parse config file
    auto path_iterator = paths.find("config");
    if (path_iterator != paths.end() && boost::filesystem::is_regular_file(path_iterator->second) &&
        !option_variables.count("base"))
    {
        SimpleLogger().Write() << "Reading options from: " << path_iterator->second.string();
        std::string ini_file_contents = read_file_lower_content(path_iterator->second);
        std::stringstream config_stream(ini_file_contents);
        boost::program_options::store(parse_config_file(config_stream, config_file_options),
                                      option_variables);
        boost::program_options::notify(option_variables);
    }
    else if (option_variables.count("base"))
    {
        path_iterator = paths.find("base");
        BOOST_ASSERT(paths.end() != path_iterator);
        std::string base_string = path_iterator->second.string();

        path_iterator = paths.find("hsgrdata");
        if (path_iterator != paths.end())
        {
            path_iterator->second = base_string + ".hsgr";
        }

        path_iterator = paths.find("nodesdata");
        if (path_iterator != paths.end())
        {
            path_iterator->second = base_string + ".nodes";
        }

        path_iterator = paths.find("edgesdata");
        if (path_iterator != paths.end())
        {
            path_iterator->second = base_string + ".edges";
        }

        path_iterator = paths.find("geometry");
        if (path_iterator != paths.end())
        {
            path_iterator->second = base_string + ".geometry";
        }

        path_iterator = paths.find("ramindex");
        if (path_iterator != paths.end())
        {
            path_iterator->second = base_string + ".ramIndex";
        }

        path_iterator = paths.find("fileindex");
        if (path_iterator != paths.end())
        {
            path_iterator->second = base_string + ".fileIndex";
        }

        path_iterator = paths.find("core");
        if (path_iterator != paths.end())
        {
            path_iterator->second = base_string + ".core";
        }

        path_iterator = paths.find("namesdata");
        if (path_iterator != paths.end())
        {
            path_iterator->second = base_string + ".names";
        }

        path_iterator = paths.find("timestamp");
        if (path_iterator != paths.end())
        {
            path_iterator->second = base_string + ".timestamp";
        }
    }

    path_iterator = paths.find("hsgrdata");
    if (path_iterator == paths.end() || path_iterator->second.string().empty() ||
        !boost::filesystem::is_regular_file(path_iterator->second))
    {
        throw exception("valid .hsgr file must be specified");
    }

    path_iterator = paths.find("nodesdata");
    if (path_iterator == paths.end() || path_iterator->second.string().empty() ||
        !boost::filesystem::is_regular_file(path_iterator->second))
    {
        throw exception("valid .nodes file must be specified");
    }

    path_iterator = paths.find("edgesdata");
    if (path_iterator == paths.end() || path_iterator->second.string().empty() ||
        !boost::filesystem::is_regular_file(path_iterator->second))
    {
        throw exception("valid .edges file must be specified");
    }

    path_iterator = paths.find("geometry");
    if (path_iterator == paths.end() || path_iterator->second.string().empty() ||
        !boost::filesystem::is_regular_file(path_iterator->second))
    {
        throw exception("valid .geometry file must be specified");
    }

    path_iterator = paths.find("ramindex");
    if (path_iterator == paths.end() || path_iterator->second.string().empty() ||
        !boost::filesystem::is_regular_file(path_iterator->second))
    {
        throw exception("valid .ramindex file must be specified");
    }

    path_iterator = paths.find("fileindex");
    if (path_iterator == paths.end() || path_iterator->second.string().empty() ||
        !boost::filesystem::is_regular_file(path_iterator->second))
    {
        throw exception("valid .fileindex file must be specified");
    }

    path_iterator = paths.find("namesdata");
    if (path_iterator == paths.end() || path_iterator->second.string().empty() ||
        !boost::filesystem::is_regular_file(path_iterator->second))
    {
        throw exception("valid .names file must be specified");
    }

    path_iterator = paths.find("timestamp");
    if (path_iterator == paths.end() || path_iterator->second.string().empty() ||
        !boost::filesystem::is_regular_file(path_iterator->second))
    {
        throw exception("valid .timestamp file must be specified");
    }

    return true;
}
}
}

#endif /* DATASTORE_OPTIONS_HPP */
