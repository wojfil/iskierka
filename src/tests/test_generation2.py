import iskierka

from pathlib import Path

TESTS_DIR = Path(__file__).resolve().parent
DATA_DIR = str(TESTS_DIR / "gendata2")

# This test is actually very important.
# It checks if the parser supports all new line patterns: \r\n, \n, \r.
# Do not remove it.

def test_generation_works():
    generator = iskierka.IskierkaGen()
    generator.compile(1234, DATA_DIR)

    natural, programming = generator.next()

    assert natural == "first" and programming == "second"
