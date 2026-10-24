import iskierka

from pathlib import Path

TESTS_DIR = Path(__file__).resolve().parent
DATA_DIR = str(TESTS_DIR / "gendata2")


def test_generation_works():
    generator = iskierka.IskierkaGen()
    generator.compile(1234, DATA_DIR)

    natural, programming = generator.next()

    assert natural == "first" and programming == "second"
