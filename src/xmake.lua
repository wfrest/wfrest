add_includedirs("$(projectdir)/src/base")
add_includedirs("$(projectdir)/src/core")
add_includedirs("$(projectdir)/src/util")

if (get_config("type") == "static") then
    set_kind("static")
else
    set_kind("shared")
end

set_targetdir(get_config("wfrest_lib"))

target("wfrest")
    add_files("**.c")
    add_files("**.cc")
    add_cxflags("-fPIC")
    add_packages("workflow", "zlib")


