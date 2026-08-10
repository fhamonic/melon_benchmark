import os
from conan import ConanFile
from conan.tools.cmake import cmake_layout, CMake
from conan.tools.build import check_min_cppstd


class BoostBenchmarkConan(ConanFile):
    name = "boost_benchmark"
    version = "1.0.0"

    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"
    build_policy = "missing"

    def requirements(self):
        self.requires("benchmark/1.9.4")
        self.requires("boost/1.88.0")
        
    def validate(self):
        check_min_cppstd(self, 20)

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package_info(self):
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []

    def package_id(self):
        self.info.clear()
