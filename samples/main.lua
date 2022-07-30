package.cpath = package.cpath .. ";..\\bin\\?.dll"

require("luaTextLoop")

TextLoop.start(10, function(keyCodes)
    for _,code in ipairs(keyCodes) do
      print(TextLoop.KeyName[code] or code)

      if code == TextLoop.KeyCode.Escape then
        return false
      end
    end

    return true
  end)
