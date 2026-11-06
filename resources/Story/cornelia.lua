-- Корнелия: король, Гарланд, принцесса Сара, Лютня и мост на восток.
-- Сцены: npc.<script> (свойство script у жителя в Tiled) и event.<script> (слой events).
-- Реплики — id из text_en.json / text_ru.json; говорящий — текст name.<speaker>.
--
-- Флаги:
--   king_met          король попросил спасти принцессу
--   garland_defeated  Гарланд побеждён (пропадает из храма)
--   princess_saved    принцесса в замке (появляется рядом с троном)
--   got_lute          принцесса отдала Лютню
--   bridge_built      король построил мост на восток (мост появляется на карте мира)
--   prologue_seen     пролог на мосту показан

LUTE = 301

-- ── Замок ──

function npc.king()
    if flag("princess_saved") and not flag("bridge_built") then
        say("king", "king.thanks.1", "king.thanks.2", "king.thanks.3")
        setFlag("king_met")
        setFlag("bridge_built")
        quest("save_princess", "done")
        quest("to_pravoka", "active")
        say("", "bridge.built")
    elseif flag("bridge_built") then
        say("king", "king.after")
    elseif flag("king_met") then
        say("king", "king.waiting")
    else
        say("king", "king.intro.1", "king.intro.2", "king.intro.3", "king.intro.4")
        setFlag("king_met")
        quest("save_princess", "active")
    end
end

function npc.princess()
    if flag("got_lute") then
        say("princess", "princess.after")
        return
    end
    say("princess", "princess.lute.1", "princess.lute.2")
    giveItem(LUTE)
    setFlag("got_lute")
    say("princess", "princess.lute.3")
end

function npc.throne_guard()
    say("guard", flag("princess_saved") and "throne_guard.after" or "throne_guard")
end

function npc.castle_guard_gate()
    say("guard", flag("princess_saved") and "gate_guard.after" or "gate_guard")
end

function npc.castle_guard_hall()
    say("guard", "hall_guard")
end

function npc.chancellor()
    if flag("princess_saved") then
        say("chancellor", "chancellor.after")
    else
        say("chancellor", "chancellor.1", "chancellor.2")
    end
end

function npc.castle_maid()
    say("maid", flag("princess_saved") and "maid.after" or "maid")
end

function npc.castle_sage()
    say("sage", "sage.1", "sage.2")
end

-- ── Храм Хаоса ──

function npc.garland()
    say("garland", "garland.1", "garland.2")
    if not battle({12}, "BlueShrine") then return end
    setFlag("garland_defeated")
    say("garland", "garland.defeated")
    say("", "garland.rescue")
    setFlag("princess_saved")
    teleport("cornelia_castle_2f", 12, 4)
    npc.king()                                      -- король благодарит сразу, как в FF1
end

-- ── Город ──

function npc.cornelia_dancer()
    say("dancer", "dancer")
end

function npc.cornelia_townsman()
    say("townsman", flag("princess_saved") and "townsman.after" or "townsman")
end

function npc.cornelia_woman()
    say("woman", flag("princess_saved") and "woman.after" or "woman")
end

function npc.cornelia_oldman()
    say("oldman", "oldman.1", "oldman.2")
end

function npc.cornelia_girl()
    say("girl", flag("bridge_built") and "girl.after" or "girl")
end

function npc.cornelia_scholar()
    say("scholar", "scholar")
end

-- ── Мост на восток: первый шаг по нему — пролог, как в FF1 ──

function event.bridge_prologue()
    if not flag("bridge_built") or flag("prologue_seen") then return end
    setFlag("prologue_seen")
    say("", "prologue.1", "prologue.2", "prologue.3", "prologue.4")
end
