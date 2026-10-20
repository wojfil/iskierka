
# Iskierka

This library contains IskierkaGen.
It uses a codebase written in Iskierka to generate pseudorandom pairs of strings: a command in English + a code in any programming language.
Useful as synthetic data for AI training, or as rich tests for AI.

## Installation

```
python -m pip install iskierka
```

## Usage

```
import iskierka

seed = 1234
folder = "data"

generator = iskierka.IskierkaGen()
generator.compile(seed, folder)

for _ in range(10):
    natural, programming = generator.next()
    
    print(natural)
    print(programming)
    print()
```

This code produces deterministic output.
Pick a random seed for data variation.

```
import secrets

seed = secrets.randbits(64)
```

## License

This project is licensed under the MIT License.
[See LICENSE](LICENSE.txt).
