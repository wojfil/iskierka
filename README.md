
![Logo](iskierkaLogo.png)

# Iskierka

Iskierka is a declarative programming language whose purpose is to associate natural language text with programming language code.

```
#output
do something
print('hello, world');
```

## External links

- [YouTube presentation video](https://www.youtube.com/watch?v=Y6TnbvZ4cko)

## IskierkaGen

IskierkaGen establishes an infinite source of synthetic data for AI based on rules from some Iskierka codebase.
It constructs pairs of random strings (natural language text and programming language code) that are correlated.
See below the animated process of Perun2 code generation along with text in English.

![Codegen example](perun2gen.gif)

## How does it work?

This is a context-free rewriting system similar to the Lindenmayer system ([link](https://en.wikipedia.org/wiki/L-system)). We define production rules and at runtime they are applied to variables until only constants are left. 

There are several differences between Iskierka and L-systems. The most notable one is that we produce two strings at once and variables are treated as inseparable pairs of strings. The system is stochastic as we can assign multiple production rules to one variable. If the same variable appears more than once in a production rule, then randomization is performed only once locally and all variable instances get the same value. Unlike the classic Lindenmayer system, we have to use the depth-first algorithm for variable evaluation here.

## Implementation

The file *'src/iskierka.h'* is a header-only C++ implementation of IskierkaGen. 
It automatically supports both Win32 API and POSIX.
Its directory also contains a Python wrapper around this C++ header file.
As a result, Iskierka can be used from both C++ and Python.

## Codebases

Initially, Iskierka has been designed by WojFil Games for code generation of the Perun2 programming language.
Go [here](https://github.com/wojfil/perun2-iskierka) to see an example Iskierka codebase prepared for Perun2.

## Other links

[Right here](TUTORIAL.md) you can find a quick tutorial.
Iskierka has some interesting and unique [design patterns](TERMINOLOGY.md).

## How to run? (C++)

In the code below we use the header-only C++ implementation of IskierkaGen.
Make sure to put *'iskierka.h'* in the same directory as *'main.cpp'*.
After compilation, you should prepare an Iskierka codebase (a directory 'data') in the location with the compiled program.

```
#include "iskierka.h"

int main(void)
{
    // the seed makes this engine deterministic even between operating systems
    // change the seed to generate different data
    const uint64_t seed = 1234;
    const std::string folder = "data";
    iskierka::IskierkaGen iskierka;

    try 
    {
        iskierka.compile(seed, folder);
    }
    catch (const iskierka::CompilationError& error) 
    {
        std::cerr << error.what() << '\n';
        return 1;
    }

    std::string natural;
    std::string programming;

    for (int i = 0; i < 10; i++)
    {
        try 
        {
            iskierka.next(natural, programming);
            std::cout << natural     << std::endl;
            std::cout << programming << std::endl;
            std::cout << std::endl;
        }
        catch (const iskierka::GenerationError& error) 
        {
            std::cerr << error.what() << '\n';
            return 1;
        }
    }

    return 0;
}
```

## How to run? (Python)

Go to [designated README.md](python/README.md).

## License

This project is licensed under the MIT License.
[See LICENSE](LICENSE.txt).
