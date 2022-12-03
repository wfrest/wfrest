set_group("test")
set_default(false)

add_requires("gtest")

add_packages("workflow")
add_deps("wfrest")
add_packages("gtest") 

add_includedirs(get_config("wfrest_inc"))

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
end
