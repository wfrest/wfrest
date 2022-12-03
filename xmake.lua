set_project("wfrest")
set_languages("c90", "c++11")
set_version("0.1.0")

if is_mode("debug") then
    set_symbols("debug")
    set_optimize("none")
end

if is_mode("release") then
    set_symbols("hidden")
    set_optimize("fastest")
    set_strip("all")
end

add_requires("workflow", {system = false})

add_includedirs("$(projectdir)/src/base")
add_includedirs("$(projectdir)/src/core")
add_includedirs("$(projectdir)/src/util")

add_files("src/**.c")
add_files("src/**.cc")
add_cxflags("-fPIC")
add_packages("workflow")
set_targetdir("$(projectdir)/_lib")
before_build("modules.before_build")
after_clean("modules.after_clean")

target("wfrest_static")
    set_kind("static")
    set_basename("wfrest")

target("wfrest_shared")
    set_kind("shared")
    set_basename("wfrest")

-- add_subdirs('test')
-- add_subdirs('example')
