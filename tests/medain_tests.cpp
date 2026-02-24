
#include "../src/out_writer/out_writer.hpp"
#include "../src/out_writer/custom_serializer.hpp"
#include "../src/out_writer/algorithm_median.hpp"
#include "../src/csv_parser/csv_parser.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <ctime>

class MedianCalculationTest : public ::testing::Test 
{
protected:
    std::filesystem::path input_file;
    std::filesystem::path expected_file;

    void SetUp() override 
    {
        std::vector<std::filesystem::path> candidates = 
        {
            std::filesystem::current_path() / "tests" / "trades.csv",
            std::filesystem::current_path() / ".." / "tests" / "trades.csv",
        };
        for (const auto& p : candidates)
        {
            if (std::filesystem::exists(p)) 
            {
                input_file = p;
                expected_file = p.parent_path() / "median_prices.csv";
                break;
            }
        }
        ASSERT_FALSE(input_file.empty()) << "Input file not found";
        ASSERT_TRUE(std::filesystem::exists(expected_file)) << "Expected file not found: " << expected_file;
    }

    void TearDown() override 
    {

    }

    bool compare_csv_files(const std::filesystem::path& file1, const std::filesystem::path& file2) 
    {
        auto read_lines = [](const std::filesystem::path& f) -> std::vector<std::string> 
        {
            std::ifstream file(f);
            if (!file.is_open()) return {};
            std::vector<std::string> lines;
            std::string line;
            while (std::getline(file, line)) {
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                lines.push_back(line);
            }
                return lines;
        };

        std::vector<std::string> lines1 = read_lines(file1);
        std::vector<std::string> lines2 = read_lines(file2);

        auto trim_trailing_empty = [](std::vector<std::string>& lines) 
        {
            while (!lines.empty() && lines.back().empty()) {
                lines.pop_back();
            }
        };
        trim_trailing_empty(lines1);
        trim_trailing_empty(lines2);

        return lines1 == lines2;
    }
};

TEST_F(MedianCalculationTest, CalculatesMedianCorrectly) 
{
    auto comp = [](const CsvParser::ParserData& a, const CsvParser::ParserData& b) 
    {
        return a.receive_ts < b.receive_ts;
    };

    auto parser = std::make_unique<CsvParser>(524288000, 4);
    auto algo = std::make_shared<MedianAlgorithm>();
    auto ser = std::make_shared<ParserDataSerializer>();
    auto out_writer = std::make_unique<OutWriter<CsvParser::ParserData, decltype(comp)>>(
        parser->get_max_elements(), ser, algo, comp);

    parser->add_file_to_parse(input_file);
    parser->wait_task_done();

    while (true) 
    {
        auto data = parser->get_ready_data();
        if (!data)
        {
            break;
        }
        out_writer->collect_data(std::move(*data));
    }
    out_writer->write_data("median_res.csv");

    ASSERT_TRUE(std::filesystem::exists("median_res.csv")) << "Output file not created: " << "median_res.csv";
    bool files_match = compare_csv_files("median_res.csv", expected_file);
    EXPECT_TRUE(files_match) << "Generated median file differs from expected.";
}

TEST_F(MedianCalculationTest, CalculatesMedianCorrectlyWithTwoFiles) 
{
    std::filesystem::path dir = input_file.parent_path();
    std::filesystem::path part1 = dir / "part1.csv";
    std::filesystem::path part2 = dir / "part2.csv";
    ASSERT_TRUE(std::filesystem::exists(part1)) << "part1.csv not found";
    ASSERT_TRUE(std::filesystem::exists(part2)) << "part2.csv not found";

    auto comp = [](const CsvParser::ParserData& a, const CsvParser::ParserData& b) 
    {
        return a.receive_ts < b.receive_ts;
    };

    auto parser = std::make_unique<CsvParser>(524288000, 4);
    auto algo = std::make_shared<MedianAlgorithm>();
    auto ser = std::make_shared<ParserDataSerializer>();
    auto out_writer = std::make_unique<OutWriter<CsvParser::ParserData, decltype(comp)>>(
        parser->get_max_elements(), ser, algo, comp);

    parser->add_file_to_parse(part1);
    parser->add_file_to_parse(part2);
    parser->wait_task_done();

    while (true) 
    {
        auto data = parser->get_ready_data();
        if (!data)
        {
            break;
        }
        out_writer->collect_data(std::move(*data));
    }
    out_writer->write_data("median_res_two.csv");

    ASSERT_TRUE(std::filesystem::exists("median_res_two.csv")) << "Output file not created";
    bool files_match = compare_csv_files("median_res_two.csv", expected_file);
    EXPECT_TRUE(files_match) << "Generated median file from two parts differs from expected.";
}