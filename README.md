
![Logo](iskierkaLogo.png)

# Iskierka

Iskierka is a declarative programming language used to associate natural language text with programming language code.

```
#output
greet
print('hello, world');
```

## IskierkaGen

IskierkaGen establishes an infinite source of synthetic data for AI based on rules from some Iskierka codebase.
It constructs a pair of random strings (natural language text and programming language code) that are correlated.
See below the process of Perun2 code generation along with text in English.

![Codegen example](perun2gen.gif)

Directory IskierkaGen contains a header-only C++ implementation of IskierkaGen. It automatically supports both Win32 API and POSIX.

## Codebases

Iskierka has been initially designed by WojFil Games for code generation of the Perun2 programming language.
Go [here](https://github.com/wojfil/perun2-iskierka) to see the Perun2 codebase.

## Other links

[Right here](TUTORIAL.md) you can find a quick tutorial.
Iskierka has some interesting and unique [design patterns](TERMINOLOGY.md).

## How to run?

In code below we use the header-only C++ implementation of IskierkaGen.
Make sure to put 'iskierka.h' in the same directory as 'main.cpp' and to prepare an Iskierka codebase here (directory 'data' with *.iski files).

```
#include "iskierka.h"

int main(void)
{
    // 'data' is the name of the relative directory with *.iski codes
    iskierka::IskierkaGen iskierka("data");

    if (! iskierka.isParsed())
    {
        return 1;
    }

    std::string natural;
    std::string programming;

    for (int i = 0; i < 10; i++)
    {
        if (iskierka.next(natural, programming))
        {
            std::cout << natural     << std::endl;
            std::cout << programming << std::endl;
            std::cout << std::endl;
        }
    }

    return 0;
}
```
