#ifndef __TOKENIZER_HPP_INCLUDED__
#define __TOKENIZER_HPP_INCLUDED__

#include <vector>
#include <string>

#include <boost/tokenizer.hpp>

namespace imgdupl
{

typedef std::vector<std::string> Tokens;
typedef boost::char_separator<char> Separator;
typedef boost::tokenizer<Separator> Tokenizer;

void tokenize(const std::string& data, Tokens& tokens, const char* separator);

} // namespace imgdupl

#endif
