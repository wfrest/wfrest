function main(target)
  local include_path = path.join(get_config("wfrest_inc"), "wfrest")
  if (not os.isdir(include_path)) then
    os.mkdir(include_path)
  end 
  
  os.cp(path.join("$(projectdir)", "*.h"), include_path)
  os.cp(path.join("$(projectdir)", "*.hpp"), include_path)
  os.cp(path.join("$(projectdir)", "*.inl"), include_path)
end
