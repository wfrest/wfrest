import("core.project.config")

function main(package)
  os.cp(path.join(config.get("wfrest_inc"), "wfrest"), path.join(package:installdir(), "include")) 
  if (get_config("type") == "static") then
    os.cp(path.join(config.get("wfrest_lib"), "*.a"), path.join(package:installdir(), "lib")) 
  else
    os.cp(path.join(config.get("wfrest_lib"), "*.so"), path.join(package:installdir(), "lib")) 
  end
end
