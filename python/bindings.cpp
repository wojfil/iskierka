// Created by WojFil Games 2024 - 2026
// This code is licensed under MIT license (see LICENSE.txt for details)

#include <pybind11/pybind11.h>
#include "../cpp/iskierka.h"

namespace py = pybind11;

PYBIND11_MODULE(iskierka, m)
{
    m.doc() = "Python bindings for Iskierka";

    py::register_exception<iskierka::CompilationError>(
        m, "CompilationError"
    );
    py::register_exception<iskierka::GenerationError>(
        m, "GenerationError"
    );

    py::class_<iskierka::IskierkaGen>(m, "IskierkaGen")
        .def(py::init<>())

        .def("compile", &iskierka::IskierkaGen::compile, py::arg("seed"), py::arg("path"))
        .def("set_seed", &iskierka::IskierkaGen::setSeed, py::arg("seed"))
        .def("set_level_limit", &iskierka::IskierkaGen::setLevelLimit, py::arg("limit"))
        .def("set_allowed_failures_in_row", &iskierka::IskierkaGen::setAllowedFailuresInRow, py::arg("limit"))
        .def("next", [](iskierka::IskierkaGen& gen)
        {
            std::string natural;
            std::string programming;
            gen.next(natural, programming);
            return py::make_tuple(natural, programming);
        });
}
