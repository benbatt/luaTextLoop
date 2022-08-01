package.cpath = package.cpath .. ";..\\bin\\?.dll"

require("luaTextLoop")

local overlay = {
  "----------------------",
  "|    Press a Key     |",
  "|                    |",
  "|        o.O         |",
  "----------------------",
}

overlay.x = 80 - #overlay[1]

local WIDTH = 18

TextLoop.setOverlay(overlay)

TextLoop.start(10, function(keyCodes)
    for _,code in ipairs(keyCodes) do
      local keyName = TextLoop.KeyName[code] or tostring(code)

      print(keyName)

      local rightPadding = string.rep(" ", (WIDTH - #keyName) / 2)

      overlay[4] = string.format("| %18s |", keyName .. rightPadding)
      TextLoop.setOverlay(overlay)

      if code == TextLoop.KeyCode.Escape then
        return false
      elseif code == TextLoop.KeyCode.PageDown then
        for i = 1,10 do
          print(string.format("  Row %d", i))
        end
      end
    end

    return true
  end)
