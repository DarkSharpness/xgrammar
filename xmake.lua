option("XGRAMMAR_BUILD_PYTHON_BINDINGS", {
    default = true, description = "Build Python bindings"
})
option("XGRAMMAR_BUILD_CXX_TESTS", {
    default = false, description = "Build C++ tests"
})

add_requires("dlpack 1.1", "nanobind v2.5.0")
add_rules("mode.debug", "mode.release")

if has_config("XGRAMMAR_BUILD_CXX_TESTS") then
    add_requires("gtest 1.16.0", {configs = {main = true}})
end

set_languages("c++17")

target("xgrammar")
    set_kind("static")
    add_includedirs("3rdparty/picojson")
    add_includedirs("include", {public = true})
    add_files("cpp/*.cc")
    add_files("cpp/support/*.cc")
    add_packages("dlpack")
    if has_config("XGRAMMAR_BUILD_PYTHON_BINDINGS") then
        add_files("cpp/nanobind/*.cc")
        add_packages("nanobind")
    end

if has_config("XGRAMMAR_BUILD_CXX_TESTS") then
    target("test")
        set_kind("binary")
        add_includedirs("3rdparty/picojson")
        add_includedirs("cpp", {public = true})
        add_files("tests/cpp/*.cc")
        add_deps("xgrammar")
        add_packages("gtest", "dlpack")
        add_tests("default")
end
