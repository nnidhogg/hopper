#include <algorithm>
#include <filesystem>
#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "hopper/json/parser.hpp"
#include "hopper/parse/parse_error.hpp"

namespace
{
using hopper::json::Parser;
using hopper::parse::Parse_error;

/**
 * @brief The parsing cases of JSONTestSuite, sorted by name so the run is the same everywhere.
 */
std::vector<std::filesystem::path> cases()
{
    const std::filesystem::path directory{std::string{SOURCE_DIR} + "/libs/json/tests/data/JSONTestSuite/test_parsing"};

    std::vector<std::filesystem::path> files;

    for (const auto& entry : std::filesystem::directory_iterator{directory})
    {
        if (entry.path().extension() == ".json")
        {
            files.push_back(entry.path());
        }
    }

    std::ranges::sort(files);

    return files;
}

/**
 * @brief Whether the parser accepts a file, with any Parse_error counted as rejection.
 *
 * Any other exception, or a crash, is a defect the test reports as such.
 */
bool accepts(const std::filesystem::path& file)
{
    try
    {
        Parser parser{file};

        static_cast<void>(parser.parse());

        return true;
    }
    catch (const Parse_error&)
    {
        return false;
    }
}

} // namespace

TEST(Conformance, Every_y_case_is_accepted_and_every_n_case_rejected)
{
    const auto files{cases()};

    ASSERT_EQ(files.size(), 318U);

    std::size_t accepted{0};
    std::size_t rejected{0};
    std::size_t implementation_defined{0};

    for (const auto& file : files)
    {
        const auto name{file.filename().string()};

        const bool ok{accepts(file)};

        if (name.starts_with("y_"))
        {
            EXPECT_TRUE(ok) << name;
            ++accepted;
        }
        else if (name.starts_with("n_"))
        {
            EXPECT_FALSE(ok) << name;
            ++rejected;
        }
        else
        {
            ++implementation_defined;
        }
    }

    EXPECT_EQ(accepted, 95U);
    EXPECT_EQ(rejected, 188U);
    EXPECT_EQ(implementation_defined, 35U);
}

// What the parser does on the implementation-defined cases is recorded, not judged: the list is the parser's
// documented behaviour on them, and a change here is a change of behaviour a release note has to carry.
TEST(Conformance, The_implementation_defined_cases_behave_as_documented)
{
    const std::vector<std::string> accepted_i_cases{
            "i_number_double_huge_neg_exp.json",  "i_number_huge_exp.json",
            "i_number_neg_int_huge_exp.json",     "i_number_pos_double_huge_exp.json",
            "i_number_real_neg_overflow.json",    "i_number_real_pos_overflow.json",
            "i_number_real_underflow.json",       "i_number_too_big_neg_int.json",
            "i_number_too_big_pos_int.json",      "i_number_very_big_negative_int.json",
            "i_structure_500_nested_arrays.json",
    };

    for (const auto& file : cases())
    {
        const auto name{file.filename().string()};

        if (!name.starts_with("i_"))
        {
            continue;
        }

        const bool expected{std::ranges::find(accepted_i_cases, name) != accepted_i_cases.end()};

        EXPECT_EQ(accepts(file), expected) << name;
    }
}
