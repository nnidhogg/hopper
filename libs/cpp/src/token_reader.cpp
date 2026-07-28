#include "hopper/cpp/token_reader.hpp"

#include <fstream>

namespace hopper::cpp
{
Token_reader::Token_reader(munch::core::Lexer lexer) : tokenizer_{std::move(lexer)}
{}

Token_reader::Token_reader(munch::core::Lexer lexer, const std::string& input)
    : tokenizer_{std::move(lexer), normalize(input)}
{}

Token_reader::Token_reader(munch::core::Lexer lexer, const std::filesystem::path& file)
    : tokenizer_{std::move(lexer), normalize(file)}
{}

void Token_reader::load(const std::string& input)
{
    tokenizer_.load(normalize(input));

    lookahead_.reset();
}

void Token_reader::load(const std::filesystem::path& file)
{
    tokenizer_.load(normalize(file));

    lookahead_.reset();
}

void Token_reader::reset() noexcept
{
    tokenizer_.reset();

    lookahead_.reset();
}

Token_reader::Result_t Token_reader::peek()
{
    if (const auto& token = lookahead_.token(); token)
    {
        return *token;
    }

    for (;;)
    {
        const auto result{tokenizer_.next<Token_kind>()};

        if (!result.has_token())
        {
            return result;
        }

        const auto& token{result.token()};

        lookahead_.advance(token.kind(), token.lexeme());

        if (skip_token(token.kind()))
        {
            lookahead_.consume();

            continue;
        }

        return *lookahead_.token();
    }
}

Token_reader::Result_t Token_reader::next()
{
    if (const auto expected{peek()}; !expected.has_token())
    {
        return expected;
    }

    return *lookahead_.consume();
}

const Token_location& Token_reader::location() const noexcept
{
    return lookahead_.location();
}

bool Token_reader::skip_token(const Token_kind kind) noexcept
{
    return kind == Token_kind::Whitespace || kind == Token_kind::Newline;
}

std::string Token_reader::normalize(const std::string& input)
{
    std::string output;

    output.reserve(input.size());

    for (auto iterator = input.cbegin(); iterator != input.cend();)
    {
        if (const char c = *iterator++; c == '\r')
        {
            if (iterator != input.cend() && *iterator == '\n')
            {
                ++iterator;
            }

            output.push_back('\n');
        }
        else
        {
            output.push_back(c);
        }
    }

    return output;
}

std::string Token_reader::normalize(const std::filesystem::path& file)
{
    return normalize(read(file));
}

std::string Token_reader::read(const std::filesystem::path& file)
{
    if (std::ifstream stream{file, std::ios::binary}; stream.is_open())
    {
        return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
    }

    throw std::runtime_error("Token_reader: cannot open file: " + file.string());
}

} // namespace hopper::cpp
