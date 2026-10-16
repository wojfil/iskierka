
# Iskierka

Using Iskierka from Python is very easy.
You can install it from PyPI and use like any typical library.

```
python -m pip install iskierka
```

Here is the example code.

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
