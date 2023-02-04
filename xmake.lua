set_project("wfrest")
set_version("0.1.0")

add_rules("mode.release", "mode.debug")
set_languages("c90", "c++11")
set_warnings("all")
set_exceptions("no-cxx")

option("wfrest_inc",  {description = "wfrest inc", default = "$(projectdir)/_include"})
option("wfrest_lib",  {description = "wfrest lib", default = "$(projectdir)/_lib"})
option("memcheck",    {description = "valgrind memcheck", default = false})

add_requires("workflow", {system = false})
add_requires("zlib", {system=false})

add_includedirs(get_config("wfrest_inc"))
add_includedirs(path.join(get_config("wfrest_inc"), "wfrest"))

set_config("buildir", "build.xmake")

add_cflags("-fPIC", "-pipe")
add_cxxflags("-fPIC", "-pipe", "-Wno-invalid-offsetof")

after_clean(function (target)
    os.rm(get_config("wfrest_inc"))
    os.rm(get_config("wfrest_lib"))
    os.rm("$(buildir)")
end)

includes("src", "test", "example")
