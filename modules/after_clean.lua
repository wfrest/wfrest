function main(target)
  os.rm(get_config("wfrest_inc"))
  os.rm(get_config("wfrest_lib"))
  os.rm("$(buildir)")
end
