set_group("test")
set_default(false)

add_requires("gtest")

add_deps("wfrest")
add_packages("workflow", "zlib")
add_packages("gtest")
add_links("gtest_main")

function all_tests()
    local res = {}
    for _, x in ipairs(os.files("**.cc")) do
        local item = {}
        local s = path.filename(x)
        table.insert(item, s:sub(1, #s - 3))       -- target
        table.insert(item, path.relative(x, "."))  -- source
        table.insert(res, item)
    end
    return res
end

for _, test in ipairs(all_tests()) do
target(test[1])
    set_kind("binary")
    add_files(test[2])
    if has_config("memcheck") then
        on_run(function (target)
            local argv = {}
            table.insert(argv, target:targetfile())
            table.insert(argv, "--leak-check=full")
            os.execv("valgrind", argv)
        end)
    end
end
