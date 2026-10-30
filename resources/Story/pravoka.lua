-- Правока: пираты Бикки и корабль.
--
-- Флаги:
--   bikke_defeated  пираты разбиты (пропадают с улиц)
--   got_ship        корабль у отряда: движок ставит его у причала Правоки (см. Game::onStoryChanged)

PIRATE = 17

function npc.pravoka_bikke()
    if flag("bikke_defeated") then
        say("bikke", "bikke.after")
        return
    end
    say("bikke", "bikke.1", "bikke.2")
    local pirates = {}
    for i = 1, 9 do pirates[i] = PIRATE end          -- девять пиратов, как в FF1
    if not battle(pirates, "Town") then return end
    setFlag("bikke_defeated")
    say("bikke", "bikke.beg.1", "bikke.beg.2")
    setFlag("got_ship")
    say("", "ship.received")
    quest("to_pravoka", "done")
    quest("to_elfheim", "active")
end

function npc.pravoka_pirate()
    say("pirate", "pirate")
end

function npc.pravoka_woman()
    say("woman", flag("bikke_defeated") and "pravoka.woman.after" or "pravoka.woman")
end

function npc.pravoka_oldman()
    say("oldman", "pravoka.oldman.1", "pravoka.oldman.2")
end

function npc.pravoka_merchant()
    say("merchant", flag("bikke_defeated") and "pravoka.merchant.after" or "pravoka.merchant")
end

function npc.pravoka_dancer()
    say("dancer", "pravoka.dancer")
end
