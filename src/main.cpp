#include "./logger/logger.hpp"
#include "./csv_parser/csv_parser.hpp"
#include "./out_writer/out_writer.hpp"
#include "./out_writer/algorithm_median.hpp"
#include "./out_writer/custom_serializer.hpp"
#include "./config_reader/config_reader.hpp"

#include <boost/program_options.hpp>

#include <memory>
#include <cstdlib>

namespace po = boost::program_options;

int main(int argc, char* argv[])
{
    constexpr std::chrono::milliseconds flushing_interval_ms(1000); 
    logger_helper::create_log_dir();
    logger_helper::init_logger("logs/csv_parser", 1024 * 1024, 3, flushing_interval_ms);
    spdlog::set_level(spdlog::level::level_enum::trace);

    constexpr size_t DEFAULT_MAX_MEMORY = 524288000;
    constexpr unsigned DEFAULT_MAX_THREAD = 4;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Show help")
        ("config", po::value<std::string>(), "Path to config file (TOML)")
        ("cfg", po::value<std::string>(), "Alternative to --config")
        ("max-memory", po::value<size_t>(), "Maximum memory buffer size in bytes (default: 524288000)")
        ("max-thread", po::value<unsigned>(), "Maximum number of threads for parsing (default: 4)");

    po::variables_map vm;
    try 
    {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    } 
    catch (const po::error& e) 
    {
        std::ostringstream oss;
        oss << desc;
        spdlog::error("Error parsing command line: {}. Allowed commands {}", e.what(), oss.str());
        return EXIT_FAILURE;
    }

    if (vm.count("help")) 
    {
        std::ostringstream oss;
        oss << desc;
        spdlog::info("Allowed commands {}", oss.str());
        return EXIT_SUCCESS;
    }

    std::string config_path = "./config.toml";
    if (vm.count("config")) 
    {
        config_path = vm["config"].as<std::string>();
    } 
    else if (vm.count("cfg")) 
    {
        config_path = vm["cfg"].as<std::string>();
    }

    size_t max_memory = DEFAULT_MAX_MEMORY;
    unsigned max_thread = DEFAULT_MAX_THREAD;

    if (vm.count("max-memory"))
    {
        max_memory = vm["max-memory"].as<size_t>();
        if (max_memory == 0)
        {
            spdlog::error("max-memory must be > 0");
            return EXIT_FAILURE;
        }
    }

    if (vm.count("max-thread"))
    {
        max_thread = vm["max-thread"].as<unsigned>();
        if (max_thread == 0)
        {
            spdlog::error("max-thread must be > 0");
            return EXIT_FAILURE;
        }
    }

    spdlog::info("CSV Parser started! max_memory={}, max_thread={}", max_memory, max_thread);

    ConfigReader::Config cfg;
    std::vector<std::filesystem::path> files{};
    try 
    {
        cfg = ConfigReader::load_from_file(config_path);
        spdlog::info("Configuration : input directory {}, output directory {}", cfg.input.string(), cfg.output.string());
        spdlog::info("Masks:");
        for (const auto& masks: cfg.filename_mask)
        {
            spdlog::info(masks);
        }
        files = std::move(ConfigReader::find_files(cfg));
        spdlog::info("Files to read:");
        for (const auto& file : files)
        {
            spdlog::info(file.string());
        }
    }
    catch (const std::exception& err)
    {
        spdlog::error("Error while parsing config file: {}", err.what());
        return EXIT_FAILURE;
    }

    auto comp = [](const CsvParser::ParserData& a, const CsvParser::ParserData& b)
    {
        return a.receive_ts < b.receive_ts;
    };

    auto parser = std::make_unique<CsvParser>(max_memory, max_thread);

    auto algo = std::make_shared<MedianAlgorithm>();
    auto ser = std::make_shared<ParserDataSerializer>();

    auto out_writer = std::make_unique<OutWriter<CsvParser::ParserData, decltype(comp)>>(parser->get_max_elements(), ser, algo, comp, max_thread);
    for (const auto& data : files)
    {
        parser->add_file_to_parse(data);
        spdlog::info("Added file to parse {}", data.string());
    }
    parser->wait_task_done();
    while(true)
    {
        std::optional<std::vector<CsvParser::ParserData>> data = parser->get_ready_data();
        if (data != std::nullopt)
        {
            out_writer->collect_data(std::move(*data));
        }
        else 
        {
            break;
        }
    }
    out_writer->write_data(cfg.output.string() + "/output.csv");
    return EXIT_SUCCESS;
}