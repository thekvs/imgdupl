#include "tokenizer.hpp"

namespace imgdupl
{

void
tokenize(const std::string& data, Tokens& tokens, const char* sep)
{
    Separator separator(sep);
    Tokenizer tokenizer(data, separator);

    tokens.clear();

    std::insert_iterator<Tokens> inserter(tokens, tokens.begin());

    std::copy(tokenizer.begin(), tokenizer.end(), inserter);
}

} // namespace
