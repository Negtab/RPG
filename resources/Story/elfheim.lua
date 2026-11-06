-- Эльфхейм: спящий принц, корона, Астос, Матойя и Волшебный ключ.
--
-- Флаги:
--   keep_king_met    король Западной крепости попросил корону
--   astos_defeated   Астос разбит (пропадает из крепости)
--   prince_awake     принц проснулся и отдал Волшебный ключ

MYSTIC_KEY, CROWN, CRYSTAL_EYE, JOLT_TONIC = 302, 304, 305, 306

-- Сундук под охраной: бой, потом предмет. Флаг открывает сундук на карте
local function guarded(map, id, enemies, background, item)
    if not battle(enemies, background) then return false end
    setFlag("chest." .. map .. "." .. id)
    giveItem(item)
    return true
end

-- ── Замок Эльфхейма ──

function npc.elf_doctor()
    if flag("prince_awake") then
        say("physician", "doctor.after")
        return
    end
    say("physician", "doctor.1", "doctor.2")
    quest("to_elfheim", "done")
    quest("wake_prince", "active")
end

function npc.elf_prince()
    if flag("prince_awake") then
        say("prince", "prince.after")
    elseif hasItem(JOLT_TONIC) then
        say("", "prince.wake")
        takeItem(JOLT_TONIC)
        say("prince", "prince.thanks.1", "prince.thanks.2")
        giveItem(MYSTIC_KEY)
        setFlag("prince_awake")
        quest("wake_prince", "done")
        say("prince", "prince.thanks.3")
    else
        say("", "prince.asleep")
    end
end

function npc.elf_guard()
    say("guard", flag("prince_awake") and "elf_guard.after" or "elf_guard")
end

-- ── Город ──

function npc.elf_a() say("elf", "elf.a") end
function npc.elf_b() say("elf", "elf.b") end
function npc.elf_c() say("elf", "elf.c") end
function npc.elf_d() say("elf", "elf.d") end
function npc.elf_e() say("elf", flag("prince_awake") and "elf.e.after" or "elf.e") end

-- ── Западная крепость ──

function npc.keep_king()
    if hasItem(CROWN) then
        say("king_keep", "keep.crown.1")
        takeItem(CROWN)
        say("astos", "keep.crown.2", "keep.crown.3")
        if not battle({33}, "CastleHall") then return end
        setFlag("astos_defeated")
        say("", "astos.defeated")
        giveItem(CRYSTAL_EYE)
        quest("find_crown", "done")
        quest("return_eye", "active")
    elseif flag("keep_king_met") then
        say("king_keep", "keep.waiting")
    else
        say("king_keep", "keep.intro.1", "keep.intro.2")
        setFlag("keep_king_met")
        quest("find_crown", "active")
    end
end

function event.keep_left()   guarded("western_keep", "left",   {34, 34, 34}, "CastleHall", 175) end
function event.keep_middle() guarded("western_keep", "middle", {26, 26, 26, 26}, "CastleHall", 164) end
function event.keep_right()  guarded("western_keep", "right",  {34, 34, 34}, "CastleHall", 191) end

-- ── Болотная пещера ──

function event.marsh_crown()  guarded("marsh_cave_b3", "crown",  {32, 32, 32}, "GreenCave", CROWN) end
function event.marsh_silver() guarded("marsh_cave_b3", "silver", {32, 32, 32}, "GreenCave", 184) end

-- ── Пещера Матойи ──

function npc.matoya()
    if hasItem(CRYSTAL_EYE) then
        say("matoya", "matoya.eye.1", "matoya.eye.2")
        takeItem(CRYSTAL_EYE)
        giveItem(JOLT_TONIC)
        quest("return_eye", "done")
    elseif flag("prince_awake") or hasItem(JOLT_TONIC) then
        say("matoya", "matoya.after")
    else
        say("matoya", "matoya.blind.1", "matoya.blind.2")
    end
end
