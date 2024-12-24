local lua = pd.Class:new():register("lua")  -- global controls (the [pdlua] object only)

function lua:initialize(sel, atoms)
  self.inlets = {}
  self.outlets = {}
  self.gui = 0

  local code_start = 0

  local i = 1
  while i <= #atoms do
      local atom = atoms[i]
      if i > 1 and atom == ";" then
         code_start = i;
         break  -- Stop if we reach the first occurrence of ";"
      end

      if type(atom) == "string" then
          if atom == "-in" and i < #atoms and type(atoms[i+1]) == "number" then
              local in_count = atoms[i+1]
              for _ = 1, in_count do
                  table.insert(self.inlets, DATA)
              end
              i = i + 1 -- Skip the next element since we've used it
          elseif atom == "-sigin" and i < #atoms and type(atoms[i+1]) == "number" then
              local sigin_count = atoms[i+1]
              for _ = 1, sigin_count do
                  table.insert(self.inlets, SIGNAL, 1)
              end
              i = i + 1 -- Skip the next element since we've used it
          elseif atom == "-out" and i < #atoms and type(atoms[i+1]) == "number" then
              local out_count = atoms[i+1]
              for _ = 1, out_count do
                  table.insert(self.outlets, DATA)
              end
              i = i + 1 -- Skip the next element since we've used it
            elseif atom == "-sigout" and i < #atoms and type(atoms[i+1]) == "number" then
              local sigout_count = atoms[i+1]
              for _ = 1, sigout_count do
                 table.insert(self.outlets, SIGNAL, 1)
              end
              i = i + 1 -- Skip the next element since we've used it
            end
        end
        i = i + 1
    end

  for _ = 1, code_start do
      table.remove(atoms, 1)
  end

  -- Concatenate atoms into a single string separated by spaces
  local lua_code = table.concat(atoms, " ")
  lua_code = string.gsub(lua_code, ";", "\n")

  self.function_prefix = "fn_" .. tostring(math.random(0, 1<<32)) .. "_"

  -- Give functions unique names to prevent namespace clashes
  lua_code = string.gsub(lua_code, "function%s+in_(%d+)_(%a+)", function(num, type)
          return "function " .. self.function_prefix .. "in_" .. num .. "_" .. type
      end)
  lua_code = string.gsub(lua_code, "function%s+in_n_(%a+)", function(type)
          return "function " .. self.function_prefix .. "in_n_" .. type
      end)
  lua_code = string.gsub(lua_code, "function%s+in_(%d+)", function(num)
          return "function " .. self.function_prefix .. "in_" .. num
      end)
  lua_code = string.gsub(lua_code, "function%s+in_n", function()
          return "function " .. self.function_prefix .. "in_n"
      end)
  lua_code = string.gsub(lua_code, "function%s$0", function()
          return "function " .. self.function_prefix
      end)
  lua_code = string.gsub(lua_code, "function%sdsp", function()
            return "function " .. self.function_prefix .. "dsp"
        end)
  lua_code = string.gsub(lua_code, "function%sperform", function()
            return "function " .. self.function_prefix .. "perform"
        end)

  -- Create a temporary file
  self.temp_name = os.tmpname()
  local temp_file = io.open(self.temp_name, 'w+b')

  if temp_file then
      -- Writing the concatenated string to the temp file
      temp_file:write(lua_code)

      -- It's important to close the file when you're done
      temp_file:close()

      self:dofile(self.temp_name)
  else
      pd.post("Error: could not create temp file")
  end

  return true
end

function lua:dsp(sample_rate, block_size)
    local m = _G[self.function_prefix .. "dsp"]
    if type(m) == "function" then
    return m(self, sample_rate, block_size)
    end
end

function lua:perform(...)
    local m = _G[self.function_prefix .. "perform"]
    if type(m) == "function" then
    return m(self, ...)
    end
end

function lua:dispatch(inlet, sel, atoms)
  if sel == "load" then
       self:dofile(atoms[1])
       return
  end

  local m = _G[self.function_prefix .. string.format("in_%d_%s", inlet, sel)]
  if type(m) == "function" then
    if sel == "bang"    then return m(self)           end
    if sel == "float"   then return m(self, atoms[1]) end
    if sel == "symbol"  then return m(self, atoms[1]) end
    if sel == "pointer" then return m(self, atoms[1]) end
    if sel == "list"    then return m(self, atoms)    end
    return m(self, atoms)
  end
  m = self[self.function_prefix .. "in_n_" .. sel]
  if type(m) == "function" then
    if sel == "bang"    then return m(self, inlet)           end
    if sel == "float"   then return m(self, inlet, atoms[1]) end
    if sel == "symbol"  then return m(self, inlet, atoms[1]) end
    if sel == "pointer" then return m(self, inlet, atoms[1]) end
    if sel == "list"    then return m(self, inlet, atoms)    end
    return m(self, inlet, atoms)
  end
  m = self[self.function_prefix .. string.format("in_%d", inlet)]
  if type(m) == "function" then
    return m(self, sel, atoms)
  end
  m = self[self.function_prefix .. "in_n"]
  if type(m) == "function" then
    return m(self, inlet, sel, atoms)
  end
  self:error(
     string.format("no method for `%s' at inlet %d of Lua object `%s'",
		   sel, inlet, self._name)
  )
end

function lua:__gc()
    os.remove(self.temp_name)
end
