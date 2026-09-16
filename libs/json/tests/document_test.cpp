#include "hopper/json/document.hpp"

#include <cstddef>
#include <gtest/gtest.h>
#include <random>
#include <string>
#include <vector>

namespace
{
using hopper::json::Document;
using hopper::json::Piece;

/**
 * @brief The token stream of a text tokenized whole, the reference every edit is held to.
 */
std::vector<Piece> whole(const std::string& text)
{
    return Document{text}.tokens();
}

/**
 * @brief A JSON document of nested arrays and objects with strings, numbers and literals, from a seed.
 */
std::string generate(std::mt19937& random, const int depth)
{
    std::uniform_int_distribution<int> pick{0, 5};

    switch (depth <= 0 ? pick(random) % 4 : pick(random))
    {
    case 0:
        return std::to_string(std::uniform_int_distribution<int>{-1000, 1000}(random));
    case 1:
        return "\"s" + std::to_string(std::uniform_int_distribution<int>{0, 99}(random)) + " x\"";
    case 2:
        return "true";
    case 3:
        return "null";
    case 4:
    {
        std::string out{"["};

        const int count{std::uniform_int_distribution<int>{0, 3}(random)};

        for (int i{0}; i < count; ++i)
        {
            out += (i ? ", " : "") + generate(random, depth - 1);
        }

        return out + "]";
    }
    default:
    {
        std::string out{"{"};

        const int count{std::uniform_int_distribution<int>{0, 3}(random)};

        for (int i{0}; i < count; ++i)
        {
            out += (i ? ", \"k" : "\"k") + std::to_string(i) + "\": " + generate(random, depth - 1);
        }

        return out + "}";
    }
    }
}

} // namespace

TEST(Document, The_stream_covers_the_text_and_matches_a_whole_scan_after_each_edit)
{
    std::mt19937 random{20260907};

    std::size_t edits{0};
    std::size_t rescanned{0};
    std::size_t whole_relexes{0};

    for (int round{0}; round < 40; ++round)
    {
        Document document{generate(random, 5)};

        ASSERT_TRUE(document.complete());

        for (int step{0}; step < 25; ++step)
        {
            const auto& text{document.text()};

            const auto offset{std::uniform_int_distribution<std::size_t>{0, text.size()}(random)};
            const auto removed{
                    std::min(text.size() - offset, std::uniform_int_distribution<std::size_t>{0, 3}(random))};

            static const std::vector<std::string> pieces{"",  "1",          "\"q\"", ", ", "[",
                                                         "]", "{\"a\": 2}", " ",     "tr", "\\"};

            const auto& inserted{pieces[std::uniform_int_distribution<std::size_t>{0, pieces.size() - 1}(random)]};

            const bool complete_before{document.complete()};

            const auto report{document.edit(offset, removed, inserted)};

            const Document reference{document.text()};

            EXPECT_EQ(document.tokens(), reference.tokens()) << "round " << round << " step " << step;
            EXPECT_EQ(document.complete(), reference.complete());

            // The saving is measured over edits that keep the text tokenizable; a text that stops tokenizing is
            // relexed whole by contract until it tokenizes again.
            if (complete_before && reference.complete())
            {
                ++edits;
                rescanned += report.rescanned;
                whole_relexes += report.whole ? 1 : 0;
            }
        }
    }

    // Small generated documents certify little, so the saving here is only that some edits stay local; how much a
    // real document saves is measured in docs/design.md.
    EXPECT_GT(edits, 0U);
    EXPECT_LT(whole_relexes, edits) << "whole relexes " << whole_relexes << " of " << edits;
    EXPECT_GT(rescanned, 0U);
}

TEST(Document, A_local_edit_rescans_from_the_anchor_before_it_to_the_first_shared_boundary_after_it)
{
    Document document{R"({"a": [1, 2, 3], "b": "text", "c": null})"};

    const auto report{document.edit(10, 1, "22")};

    EXPECT_FALSE(report.whole);
    EXPECT_LT(report.rescanned, 12U);
    EXPECT_EQ(document.text(), R"({"a": [1, 22, 3], "b": "text", "c": null})");
    EXPECT_EQ(document.tokens(), whole(document.text()));
}

TEST(Document, An_edit_that_breaks_tokenization_falls_back_to_the_whole_text_and_recovers_when_repaired)
{
    Document document{R"([1, "ab", 3])"};

    const auto broken{document.edit(5, 0, "\x01")};

    EXPECT_TRUE(broken.whole);
    EXPECT_FALSE(document.complete());

    const auto repaired{document.edit(5, 1, "")};

    EXPECT_TRUE(repaired.whole);
    EXPECT_TRUE(document.complete());
    EXPECT_EQ(document.tokens(), whole(document.text()));
}

TEST(Document, Ranges_outside_the_text_are_refused)
{
    Document document{"[1]"};

    EXPECT_THROW(static_cast<void>(document.edit(4, 0, "")), std::out_of_range);
    EXPECT_THROW(static_cast<void>(document.edit(2, 5, "")), std::out_of_range);
}
