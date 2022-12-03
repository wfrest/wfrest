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
add_requires("zlib", {system=false})

option("wfrest_inc")
    set_default("$(projectdir)/_include")
    set_showmenu(true)
    set_description("wfrest inc")
option_end()

option("wfrest_lib")
    set_default("$(projectdir)/_lib")
    set_showmenu(true)
    set_description("wfrest lib")
option_end()

option("type")
    set_default("static")
    set_showmenu(true)
    set_description("build lib static/shared")
option_end()

-- script
before_build("modules.before_build")
after_clean("modules.after_clean")
on_install("modules.on_install")

includes("src", "test", "example")
