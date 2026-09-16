// Measures what an edit costs a JSON document whose token stream is kept current by relexing only between
// certified positions, which is the edit theorem of the certified-splitting report run as a program: after an
// edit the scan restarts at the last certified token start whose evidence the edit left untouched and stops at
// the first boundary after the edit that the old stream also had, so the bytes rescanned are the distance
// between two certificates rather than the size of the document.
//
// What runs as a test. Without arguments the probe generates a deterministic JSON document, applies a fixed
// schedule of edits that keep the text tokenizable, a digit changed at the end of a number, letters inserted
// or one deleted inside a string away from escapes and multi-byte characters, whitespace added after a comma,
// and requires after every edit that the document's stream equals the stream of the edited text tokenized
// whole, with the document complete throughout and the edits mostly local. The figures printed are not pinned;
// they depend on where the corpus certifies.
//
// Corpus mode. With a JSON file argument the probe applies the same edit schedule to that text and prints the
// bytes, the token count, how many edits were applied, how many relexed the whole document and the bytes
// rescanned per edit as mean, median, ninetieth percentile and maximum, then checks the final stream against a
// whole scan. The optional second argument is the number of schedule steps; each step that lands on an
// unsuitable token is skipped, so the edits applied are fewer.

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <numeric>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include "hopper/json/document.hpp"

namespace
{
using hopper::json::Document;
using hopper::json::Piece;
using hopper::json::Token_kind;

/**
 * @brief Expectations that did not hold, which decide the exit status.
 */
std::size_t failures{0};

/**
 * @brief Records an expectation, printing the failed ones.
 * @param condition Whether it held.
 * @param what What was expected, printed when it did not.
 */
void expect(const bool condition, const std::string_view what)
{
    if (!condition)
    {
        ++failures;
        std::printf("FAIL: %.*s\n", static_cast<int>(what.size()), what.data());
    }
}

/**
 * @brief A JSON document of nested arrays and objects with strings, numbers and literals, from a seed.
 * @param random The generator, advanced.
 * @param depth How deep containers may nest from here.
 * @return The text.
 */
std::string generate(std::mt19937& random, const int depth)
{
    std::uniform_int_distribution<int> pick{0, 5};

    switch (depth <= 0 ? pick(random) % 4 : pick(random))
    {
    case 0:
        return std::to_string(std::uniform_int_distribution<int>{-100000, 100000}(random));
    case 1:
        return "\"string number " + std::to_string(std::uniform_int_distribution<int>{0, 9999}(random)) +
               " with a few words in it\"";
    case 2:
        return "true";
    case 3:
        return "null";
    case 4:
    {
        std::string out{"["};

        const int count{std::uniform_int_distribution<int>{0, 6}(random)};

        for (int i{0}; i < count; ++i)
        {
            out += (i ? ", " : "") + generate(random, depth - 1);
        }

        return out + "]";
    }
    default:
    {
        std::string out{"{"};

        const int count{std::uniform_int_distribution<int>{0, 6}(random)};

        for (int i{0}; i < count; ++i)
        {
            out += (i ? ", \"key" : "\"key") + std::to_string(i) + "\": " + generate(random, depth - 1);
        }

        return out + "}";
    }
    }
}

/**
 * @brief One edit of the schedule.
 */
struct Edit
{
    /**
     * @brief Where the edit begins.
     */
    std::size_t offset;

    /**
     * @brief How many bytes it removes there.
     */
    std::size_t removed;

    /**
     * @brief The bytes it inserts in their place.
     */
    std::string inserted;
};

/**
 * @brief Whether a string token is one the string edits may touch: no escape anywhere in it.
 * @param text The text the token lies in.
 * @param token The token.
 * @return True when the token is a string without a backslash.
 */
bool plain_string(const std::string& text, const Piece& token)
{
    return token.kind == Token_kind::String &&
           std::string_view{text}.substr(token.offset, token.length).find('\\') == std::string_view::npos;
}

/**
 * @brief Whether a byte may stand next to an edit: not part of a multi-byte character.
 * @param byte The byte.
 * @return True when the byte is ASCII.
 */
bool ascii(const char byte)
{
    return (static_cast<unsigned char>(byte) & 0x80) == 0;
}

/**
 * @brief The edit a schedule step makes on a token, when the token suits the step's kind.
 * @param text The text the token lies in.
 * @param token The token picked.
 * @param step The schedule step, which chooses the edit's kind and its position within the token.
 * @return The edit, or std::nullopt when the token does not suit the step.
 */
std::optional<Edit> edit_for(const std::string& text, const Piece& token, const int step)
{
    switch (step % 4)
    {
    case 0:
    {
        // A digit changed at the end of a number; a lone minus is left alone.
        if (token.kind != Token_kind::Number || text[token.offset + token.length - 1] == '-')
        {
            return std::nullopt;
        }

        return Edit{
                .offset = token.offset + token.length - 1,
                .removed = 1,
                .inserted = std::string(1, static_cast<char>('1' + step % 9))};
    }
    case 1:
    {
        // Two letters inserted inside a string without escapes, between two ASCII bytes.
        if (!plain_string(text, token) || token.length < 3)
        {
            return std::nullopt;
        }

        const auto offset{token.offset + 1 + step % (token.length - 2)};

        if (!ascii(text[offset - 1]) || !ascii(text[offset]))
        {
            return std::nullopt;
        }

        return Edit{.offset = offset, .removed = 0, .inserted = "xy"};
    }
    case 2:
    {
        // One ASCII byte deleted inside a string without escapes, its neighbours ASCII too.
        if (!plain_string(text, token) || token.length < 4)
        {
            return std::nullopt;
        }

        const auto offset{token.offset + 1 + step % (token.length - 3)};

        if (!ascii(text[offset - 1]) || !ascii(text[offset]) || !ascii(text[offset + 1]))
        {
            return std::nullopt;
        }

        return Edit{.offset = offset, .removed = 1, .inserted = ""};
    }
    default:
    {
        // A line break and indentation after a comma.
        if (token.kind != Token_kind::Comma)
        {
            return std::nullopt;
        }

        return Edit{.offset = token.offset + 1, .removed = 0, .inserted = "\n  "};
    }
    }
}

/**
 * @brief What the schedule measured.
 */
struct Figures
{
    /**
     * @brief Edits applied.
     */
    std::size_t edits;

    /**
     * @brief Edits that relexed the whole text.
     */
    std::size_t whole;

    /**
     * @brief Bytes rescanned per edit, sorted.
     */
    std::vector<std::size_t> rescanned;
};

/**
 * @brief Applies the edit schedule to a document, holding its stream to a whole scan after every edit.
 * @param document The document, edited in place.
 * @param steps How many schedule steps to take.
 * @param check Whether to compare the stream against a whole scan after every edit, which costs a whole scan each.
 * @return The figures.
 */
Figures run(Document& document, const int steps, const bool check)
{
    std::mt19937 random{7};

    Figures figures{.edits = 0, .whole = 0, .rescanned = {}};

    // An empty text has no token to pick.
    for (int step{0}; step < steps && !document.tokens().empty(); ++step)
    {
        const auto& tokens{document.tokens()};

        const auto& token{tokens[std::uniform_int_distribution<std::size_t>{0, tokens.size() - 1}(random)]};

        const auto edit{edit_for(document.text(), token, step)};

        if (!edit)
        {
            continue;
        }

        const auto report{document.edit(edit->offset, edit->removed, edit->inserted)};

        expect(document.complete(), "the edited text tokenizes completely");

        if (check)
        {
            expect(document.tokens() == Document{document.text()}.tokens(),
                   "the stream equals the edited text tokenized whole");
        }

        ++figures.edits;
        figures.whole += report.whole ? 1 : 0;
        figures.rescanned.push_back(report.rescanned);
    }

    std::ranges::sort(figures.rescanned);

    return figures;
}

/**
 * @brief A corpus of generated values in one array, large enough for the schedule to land on many tokens.
 * @param random The generator, advanced.
 * @return The text.
 */
std::string generate_corpus(std::mt19937& random)
{
    std::string out{"["};

    for (int i{0}; i < 400; ++i)
    {
        out += (i ? ",\n " : "") + generate(random, 4);
    }

    return out + "]";
}

/**
 * @brief Prints the figures of a schedule.
 * @param figures The figures; a schedule that applied no edit prints so.
 */
void print(const Figures& figures)
{
    if (figures.edits == 0)
    {
        std::printf("edits 0\n");
        return;
    }

    const auto sum{std::accumulate(figures.rescanned.begin(), figures.rescanned.end(), std::size_t{0})};

    const auto& r{figures.rescanned};

    std::printf(
            "edits %zu whole %zu rescanned bytes mean %.1f median %zu p90 %zu max %zu\n", figures.edits, figures.whole,
            static_cast<double>(sum) / static_cast<double>(figures.edits), r[r.size() / 2], r[r.size() * 9 / 10],
            r.back());
}

} // namespace

int main(const int argc, char** argv)
{
    if (argc > 1)
    {
        std::ifstream in{argv[1], std::ios::binary};

        if (!in)
        {
            std::fprintf(stderr, "cannot open %s\n", argv[1]);
            return 1;
        }

        Document document{std::string{std::istreambuf_iterator<char>{in}, {}}};

        if (!document.complete())
        {
            std::fprintf(stderr, "%s does not tokenize completely\n", argv[1]);
            return 1;
        }

        std::printf("bytes %zu tokens %zu\n", document.text().size(), document.tokens().size());

        print(run(document, argc > 2 ? std::atoi(argv[2]) : 4000, false));

        expect(document.tokens() == Document{document.text()}.tokens(), "the final stream equals a whole scan");

        return failures == 0 ? 0 : 1;
    }

    std::mt19937 random{20260907};

    Document document{generate_corpus(random)};

    expect(document.complete(), "the generated corpus tokenizes completely");

    std::printf("bytes %zu tokens %zu\n", document.text().size(), document.tokens().size());

    const auto figures{run(document, 2000, true)};

    print(figures);

    expect(figures.edits > 100, "the schedule applied a hundred edits or more");
    expect(figures.whole * 20 < figures.edits, "at most one edit in twenty relexed the whole corpus");

    std::printf("%s\n", failures == 0 ? "OK" : "FAILED");

    return failures == 0 ? 0 : 1;
}
