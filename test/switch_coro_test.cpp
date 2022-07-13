#include "switch_coro.h"

#include <string>
#include <iostream>

#define reenter(c) CORO_REENTER(c)
#define yield CORO_YIELD
#define fork CORO_FORK

// [characters]-[numbers]-[characters]
class Parser : coroutine
{
public:
    bool parse(char c)
    {
        reenter(this)
        {
            // parse characters
            while(std::isalpha(c))
            {
                characters.push_back(c);
                yield return true;
            }
            if (characters.empty())
            {
                std::cout << "no characters\n";
                yield return false;
            }

            // connecting symbol
            if (c != '-')
            {
                std::cout << "characters should tailing with '-'\n";
                yield return false;
            }
            else
            {
                yield return true;
            }

            // parse numbers
            while (std::isalnum(c))
            {
                numbers.push_back(c);
                yield return true;
            }
            if (numbers.empty())
            {
                std::cout << "no numbers\n";
                yield return false;
            }
        }
        return false;
    }

    std::string to_string() const
    {
        return std::string("characters: ") + characters + "\nnumbers: " + numbers;
    }

private:
    std::string characters;
    std::string numbers;
};

int main()
{
    std::string test = "abc-123";
    Parser p;
    for (char c : test)
        if (!p.parse(c))
        {
            std::cout << "failed to parse\n";
            return -1;
        }
    std::cout << p.to_string() << '\n';

    return 0;
}