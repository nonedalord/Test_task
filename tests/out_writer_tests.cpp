#include "../src/out_writer/out_writer.hpp"
#include "../src/out_writer/serializer.hpp"
#include "../src/out_writer/algorithm.hpp"

#include <gtest/gtest.h>

#include <vector>
#include <memory>
#include <filesystem>
#include <iostream>

struct TestData 
{
    int a;
    int b;
    int c;
};

class TestDataSerializer : public ISerializer<TestData> 
{
public:
    void write(std::ostream& os, const TestData& value) override 
    {
        os.write(reinterpret_cast<const char*>(&value.a), sizeof(value.a));
        os.write(reinterpret_cast<const char*>(&value.b), sizeof(value.b));
        os.write(reinterpret_cast<const char*>(&value.c), sizeof(value.c));
    }

    void read(std::istream& is, TestData& value) override 
    {
        is.read(reinterpret_cast<char*>(&value.a), sizeof(value.a));
        is.read(reinterpret_cast<char*>(&value.b), sizeof(value.b));
        is.read(reinterpret_cast<char*>(&value.c), sizeof(value.c));
    }
};


class TestAlgorithmFile : public IAlgorithm<TestData>
{
public:
    void process_in_memory(std::vector<TestData>&& sorted_data, const std::string& output_file) override
    {
        int dummy_val;
    }
    void process_file(const std::shared_ptr<ISerializer<TestData>> serializer, const std::string& sorted_input_file, const std::string& output_file) override 
    {
        std::ifstream in(sorted_input_file, std::ios::binary);
        if (!in.is_open()) 
        {
            throw std::runtime_error("Cannot open sorted input file: " + sorted_input_file);
        }

        uint64_t total_elements = 0;
        in.read(reinterpret_cast<char*>(&total_elements), sizeof(total_elements));
        m_sorted_data.clear();
        m_sorted_data.reserve(total_elements);
        for (uint64_t i = 0; i < total_elements; ++i) 
        {
            TestData tmp;
            serializer->read(in, tmp);
            m_sorted_data.push_back(tmp);
        }
        try 
        {
            std::filesystem::remove(sorted_input_file);
        }
        catch(const std::exception& err)
        {
            std::cout << "Error while deleting file " << sorted_input_file << std::endl;
        }
    }

    std::vector<TestData>& get_sorted_data()
    {
        return m_sorted_data;
    }
private:
    std::vector<TestData> m_sorted_data;
};

class TestAlgorithmImMemory : public IAlgorithm<TestData>
{
public:
    void process_in_memory(std::vector<TestData>&& sorted_data, const std::string& output_file) override
    {
        for (const auto& data : sorted_data)
        {
            m_sorted_data.push_back(data);
        }
    }
    void process_file(const std::shared_ptr<ISerializer<TestData>> serializer, const std::string& sorted_input_file, const std::string& output_file) override 
    {
        int dummy_val;
    }

    std::vector<TestData>& get_sorted_data()
    {
        return m_sorted_data;
    }
private:
    std::vector<TestData> m_sorted_data;
};


bool is_sorted_by_c(const std::vector<TestData>& data) 
{
    for (size_t i = 1; i < data.size(); ++i) 
    {
        if (data[i-1].c > data[i].c) 
        {
            return false;
        }
    }
    return true;
}

class OutWriterTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        for (const auto& entry : std::filesystem::directory_iterator(".")) 
        {
            if (entry.path().extension() == ".bin" &&entry.path().string().find("binary_data_") != std::string::npos) 
            {
                std::filesystem::remove(entry.path());
            }
        }
    }

    void TearDown() override 
    {
        for (const auto& entry : std::filesystem::directory_iterator(".")) 
        {
            if (entry.path().extension() == ".bin" && entry.path().string().find("binary_data_") != std::string::npos) 
            {
                std::filesystem::remove(entry.path());
            }
        }
    }
};

TEST_F(OutWriterTest, InMemoryMode) 
{
    const uint64_t max_elements = 10;
    auto serializer = std::make_shared<TestDataSerializer>();
    auto algorithm = std::make_shared<TestAlgorithmImMemory>();
    auto comp = [](const TestData& a, const TestData& b)
    {
        return a.c < b.c;
    };

    OutWriter<TestData, decltype(comp)> writer(max_elements, serializer, algorithm, comp);

    std::vector<TestData> chunk1 = {{1, 10, 5}, {2, 20, 3}, {3, 30, 7}};
    std::vector<TestData> chunk2 = {{4, 40, 1}, {5, 50, 9}, {6, 60, 2}};
    std::vector<TestData> chunk3 = {{7, 70, 4}, {4, 32, 6}, {65, 43, 8}};

    writer.collect_data(std::move(chunk1));
    writer.collect_data(std::move(chunk2));
    writer.collect_data(std::move(chunk3));

    writer.write_data("dummy_output.txt");
    std::vector<TestData> res(algorithm->get_sorted_data());

    ASSERT_EQ(res.size(), 9);
    EXPECT_TRUE(is_sorted_by_c(res));
}


TEST_F(OutWriterTest, FileBasedMode) 
{
    const uint64_t max_elements = 10;
    auto serializer = std::make_shared<TestDataSerializer>();
    auto algorithm = std::make_shared<TestAlgorithmFile>();
    auto comp = [](const TestData& a, const TestData& b)
    {
        return a.c < b.c;
    };

    OutWriter<TestData, decltype(comp)> writer(max_elements, serializer, algorithm, comp);

    std::vector<TestData> chunk1, chunk2, chunk3;
    for (int i = 0; i < 6; ++i)
    {
        chunk1.push_back({i, i * 10, 9 - i});
    }

    for (int i = 0; i < 6; ++i)
    {
        chunk2.push_back({i + 6, i * 10, 15 - i});
    }
    for (int i = 0; i < 6; ++i)
    {
        chunk3.push_back({i + 12, i * 10, 3 - i});
    }

    writer.collect_data(std::move(chunk1));
    writer.collect_data(std::move(chunk2));
    writer.collect_data(std::move(chunk3));


    writer.write_data("dummy_output.txt");

    std::vector<TestData> res = algorithm->get_sorted_data();

    ASSERT_EQ(res.size(), 18);

    EXPECT_TRUE(is_sorted_by_c(res));
}