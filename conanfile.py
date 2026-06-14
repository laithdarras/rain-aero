"""Conan recipe for the C++ quickstart project.

Pulls in GoogleTest (and any other dependencies a candidate adds) and
emits CMake toolchain + dependency files so the standard
`cmake --preset conan-<buildtype>` workflow works out of the box.
"""

from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain


class CppQuickstartRecipe(ConanFile):
    name = "cpp-quickstart"
    version = "0.1.0"
    description = "C++ quickstart"
    settings = "os", "compiler", "build_type", "arch"

    options = {
        "build_tests": [True, False],
    }
    default_options = {
        "build_tests": True,
    }

    # def requirements(self):
    #     # Header-only utility; add more dependencies here as needed.
    #     self.requires("fmt/10.2.1")
    #     self.requires("ffmpeg/6.1")     # Read and demux the TS file
    #     self.requires("nlohmann_json/3.11.3")       # Serialize extracted pose to JSON

    def requirements(self):
        self.requires("fmt/10.2.1")
        self.requires("ffmpeg/6.1")
        self.requires("nlohmann_json/3.11.3")

    def configure(self):
    # Disable all hardware acceleration and display features
    # We only need software demuxing
        self.options["ffmpeg"].with_vaapi = False
        self.options["ffmpeg"].with_vdpau = False
        self.options["ffmpeg"].with_vulkan = False
        self.options["ffmpeg"].with_xcb = False
        self.options["ffmpeg"].with_libalsa = False
        self.options["ffmpeg"].with_pulse = False
        self.options["ffmpeg"].with_sdl = False
        self.options["ffmpeg"].with_xlib = False
        self.options["ffmpeg"].with_freetype = False
        self.options["ffmpeg"].with_openjpeg = False
        self.options["ffmpeg"].with_openh264 = False
        self.options["ffmpeg"].with_opus = False
        self.options["ffmpeg"].with_vorbis = False
        self.options["ffmpeg"].with_zeromq = False
        self.options["ffmpeg"].with_libiconv = False
        self.options["ffmpeg"].with_libfdk_aac = False
        self.options["ffmpeg"].with_libmp3lame = False
        self.options["ffmpeg"].with_libvpx = False
        self.options["ffmpeg"].with_libwebp = False
        self.options["ffmpeg"].with_ssl = "openssl"

    def build_requirements(self):
        if self.options.build_tests:
            self.test_requires("gtest/1.15.0")

    # No `layout()` on purpose: we want the Conan files to land directly
    # in whatever `--output-folder` build.sh hands us (i.e. one folder
    # per build type), keeping the toolchain path predictable.

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        tc = CMakeToolchain(self)
        tc.variables["BUILD_TESTS"] = bool(self.options.build_tests)
        tc.generate()
