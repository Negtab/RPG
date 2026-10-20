#include "mapLoader.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

using nlohmann::json;

namespace {
template <class T>
T prop(const json& obj, const char* name, T def)
{
    const auto it = obj.find("properties");
    if (it == obj.end() || !it->is_array()) return def;
    for (const auto& p : *it)
    {
        if (p.value("name", std::string{}) != name || !p.contains("value")) continue;
        try { return p["value"].get<T>(); }
        catch (const json::exception&) { return def; }
    }
    return def;
}

std::vector<int> parseList(const std::string& s)
{
    std::vector<int> out;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ','))
        try { out.push_back(std::stoi(item)); } catch (...) {}
    return out;
}

int cellOf(double px, double tile) { return static_cast<int>(std::floor(px / tile)); }

Point nearestWalkable(const MapData& m, int cx, int cy)
{
    Point best{cx, cy};
    long bestD = -1;
    for (int y = 0; y < m.h; ++y)
        for (int x = 0; x < m.w; ++x)
            if (m.isWalkable(x, y))
            {
                const long d = static_cast<long>(x - cx) * (x - cx) + static_cast<long>(y - cy) * (y - cy);
                if (bestD < 0 || d < bestD) { bestD = d; best = {x, y}; }
            }
    return best;
}
}

MapData loadMap(const std::string& path)
{
    std::ifstream f(path);
    if (!f) throw std::runtime_error("cannot open map: " + path);

    const json j = json::parse(f);

    MapData m;
    m.w = j.at("width").get<int>();
    m.h = j.at("height").get<int>();
    const double tw = j.at("tilewidth").get<double>();
    const double th = j.at("tileheight").get<double>();
    if (m.w <= 0 || m.h <= 0 || tw <= 0 || th <= 0)
        throw std::runtime_error("bad map size");
    m.solid.assign(static_cast<size_t>(m.w) * m.h, 0);

    const std::filesystem::path file(path);
    m.name  = file.stem().string();
    m.mapId = prop<int>(j, "mapId", -1);              // свойство самой карты (Map → Map Properties)
    if (m.mapId < 0)
    {
        if (m.name == "overworld") m.mapId = 0;       // старый overworld.tmj без свойства
        else throw std::runtime_error("map property 'mapId' (int) is missing");
    }
    if (m.mapId > 65535) throw std::runtime_error("mapId must be 0..65535");

    bool haveSpawn = false;

    for (const auto& L : j.at("layers"))
    {
        const std::string name = L.value("name", std::string{});
        const std::string type = L.value("type", std::string{});

        if (type == "imagelayer" && name == "background")
            m.backgroundPath = (file.parent_path() / L.at("image").get<std::string>()).lexically_normal().string();
        else if (type == "tilelayer" && name == "collision")
        {
            const auto& data = L.at("data");
            if (!data.is_array() || data.size() != m.solid.size())
                throw std::runtime_error("collision layer: expected w*h tiles (Tile Layer Format = CSV)");
            for (size_t i = 0; i < m.solid.size(); ++i)
                m.solid[i] = data[i].get<uint32_t>() != 0;
        }
        else if (type == "objectgroup" && name == "zones")
        {
            for (const auto& o : L.at("objects"))
            {
                const double x = o.value("x", 0.0), y = o.value("y", 0.0);
                const double w = o.value("width", 0.0), h = o.value("height", 0.0);

                Zone z;
                z.name = o.value("name", std::string{});
                z.x0 = cellOf(x, tw);
                z.y0 = cellOf(y, th);
                z.x1 = std::max(z.x0, static_cast<int>(std::ceil((x + w) / tw)) - 1);
                z.y1 = std::max(z.y0, static_cast<int>(std::ceil((y + h) / th)) - 1);
                z.safe          = prop<bool>(o, "safe", false);
                z.encounterRate = prop<int>(o, "encounterRate", 8);
                z.enemies       = parseList(prop<std::string>(o, "enemies", ""));
                z.encounter = prop<std::string>(o, "encounter", "");
                z.river     = prop<bool>(o, "river", false);   // по воде здесь ходит каноэ
                m.zones.push_back(std::move(z));
            }
        }
        else if (type == "objectgroup" && name == "portals")
        {
            for (const auto& o : L.at("objects"))
            {
                Portal p;
                p.x = cellOf(o.value("x", 0.0), tw);
                p.y = cellOf(o.value("y", 0.0), th);
                p.targetMap = prop<std::string>(o, "targetMap", "");
                p.targetX   = prop<int>(o, "targetX", 0);
                p.targetY   = prop<int>(o, "targetY", 0);
                m.portals.push_back(std::move(p));
            }
        }
        else if (type == "objectgroup" && name == "markers")
        {
            for (const auto& o : L.at("objects"))
            {
                const std::string marker = o.value("name", std::string{});
                const Point at{ cellOf(o.value("x", 0.0), tw), cellOf(o.value("y", 0.0), th) };
                if (marker == "spawn")
                {
                    m.spawn = at;
                    haveSpawn = true;
                }
                // Стоянки транспорта: точки canoe, ship, airship
                for (Vehicle v : {Vehicle::Canoe, Vehicle::Ship, Vehicle::Airship})
                {
                    std::string lower = vehicleName(v);
                    for (char& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                    if (marker == lower) m.vehicleMarkers[vehicleIndex(v)] = at;
                }
            }
        }
    }

    if (!haveSpawn || !m.isWalkable(m.spawn.x, m.spawn.y))      // нет точки или она в стене
        m.spawn = nearestWalkable(m, m.w / 2, m.h / 2);

    return m;
}