import os
from conan import ConanFile
from conan.tools.cmake import cmake_layout, CMake
from conan.tools.build import check_min_cppstd


class MelonBenchmarkConan(ConanFile):
    name = "melon_benchmark"
    version = "1.0.0"

    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"
    build_policy = "missing"

    def requirements(self):
        self.requires("benchmark/1.9.4")
        self.requires("melon/1.0.0")
        # self.requires("gmp/6.3.0")
        # self.requires("mppp/1.0.3")

    def validate(self):
        check_min_cppstd(self, 23)

    def layout(self):
        cmake_layout(self)

    def generate(self):
        print(
            'conanfile.py: Include directories:\n\t"{}"'.format(
                '",\n\t"'.join(
                    [
                        dir
                        for lib, dep in self.dependencies.items()
                        if lib.headers
                        for dir in dep.cpp_info.includedirs
                    ]
                )
            )
        )

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package_info(self):
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []

    def package_id(self):
        self.info.clear()
