-- List files. Usage: ll "path" or just ll()
function ll (path)
  local list = pico.ls (path)
  for k, file in pairs (list) do
    if (file ~= "." and file ~= "..") then
      local fullpath
      if (path ~= nil and path ~= "/") then
        fullpath = path .. "/" .. file
      else
        fullpath = file
      end
      info = pico.stat (fullpath)
      local name = info["name"]
      local type = info["type"]
      local size = info["size"]
      local stype, ssize
      if type == "directory" then
        stype = "d "
        ssize = " ";
      else
        stype = "- "
        ssize = "" .. math.floor (size);
      end
      local pad = ""
      local i
      for i = 0, 6 - string.len (ssize) do
        pad = pad .. " "
      end
      local line = stype .. ssize .. pad  .. name;
      print (line)
    end
  end
end

