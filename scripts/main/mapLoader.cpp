#include "mapLoader.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
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

// Тайл из тайлсета-коллекции (props.tsx): своя картинка у каждого тайла
struct PropTile { std::string sprite; int w = 0, h = 0; PropCollision collision = PropCollision::Solid; };

PropCollision parseCollision(const std::string& s)
{
    if (s == "walk") return PropCollision::Walk;
    if (s == "none") return PropCollision::None;
    return PropCollision::Solid;
}

std::string xmlAttr(const std::string& tag, const std::string& name)
{
    const std::string key = name + "=\"";
    const auto a = tag.find(key);
    if (a == std::string::npos) return {};
    const auto b = tag.find('"', a + key.size());
    return b == std::string::npos ? std::string{} : tag.substr(a + key.size(), b - a - key.size());
}

// Разбираем .tsx без XML-библиотеки: Tiled пишет его предсказуемо, нужны только тайлы с картинками
void loadPropTiles(const std::filesystem::path& tsx, int firstGid, std::map<int, PropTile>& out)
{
    std::ifstream f(tsx);
    if (!f) return;
    const std::string xml((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

    for (size_t pos = xml.find("<tile "); pos != std::string::npos; pos = xml.find("<tile ", pos + 1))
    {
        const size_t end = xml.find("</tile>", pos);
        if (end == std::string::npos) break;
        const std::string body = xml.substr(pos, end - pos);
        const size_t img = body.find("<image ");
        if (img == std::string::npos) continue;
        const std::string imageTag = body.substr(img, body.find('>', img) - img);

        PropTile t;
        t.sprite = std::filesystem::path(xmlAttr(imageTag, "source")).stem().string();
        t.w = std::atoi(xmlAttr(imageTag, "width").c_str());
        t.h = std::atoi(xmlAttr(imageTag, "height").c_str());
        if (const size_t p = body.find("name=\"collision\""); p != std::string::npos)
            t.collision = parseCollision(xmlAttr(body.substr(p, body.find('>', p) - p), "value"));
        out[firstGid + std::atoi(xmlAttr(body.substr(0, body.find('>')), "id").c_str())] = t;
    }
}

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
    m.exitMode = prop<std::string>(j, "exit", "");
    m.entrance = prop<std::string>(j, "entrance", "");

    bool haveSpawn = false;

    std::map<int, PropTile> propTiles;                      // gid → картинка объекта
    for (const auto& ts : j.value("tilesets", json::array()))
        if (ts.contains("source"))
            loadPropTiles(file.parent_path() / ts.at("source").get<std::string>(), ts.value("firstgid", 1), propTiles);

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
                z.background = prop<std::string>(o, "background", "");
                m.zones.push_back(std::move(z));
            }
        }
        else if (type == "objectgroup" && name == "props")
        {
            for (const auto& o : L.at("objects"))
            {
                const uint32_t gid = o.value("gid", 0u) & 0x1FFFFFFFu;       // старшие биты — отражения в Tiled
                const auto it = propTiles.find(static_cast<int>(gid));
                if (it == propTiles.end()) continue;
                const PropTile& t = it->second;

                // У тайла-объекта Tiled хранит левый НИЖНИЙ угол. Объект мог стоять не по сетке:
                // рисуем его точно по пикселям, а стеной считаем клетки, покрытые больше чем наполовину
                Prop p;
                p.sprite = t.sprite;
                p.px = static_cast<float>(o.value("x", 0.0));
                p.py = static_cast<float>(o.value("y", 0.0));
                p.pw = t.w;
                p.ph = t.h;
                const int left   = static_cast<int>(std::lround(p.px / tw));
                const int right  = static_cast<int>(std::lround((p.px + t.w) / tw));    // не включая
                const int top    = static_cast<int>(std::lround((p.py - t.h) / th));
                const int bottom = static_cast<int>(std::lround(p.py / th));            // не включая
                p.x0 = left;
                p.w = std::max(1, right - left);
                p.bottom = std::max(top + 1, bottom) - 1;
                p.h = std::max(1, bottom - top);
                p.collision = parseCollision(prop<std::string>(o, "collision",
                    t.collision == PropCollision::Walk ? "walk" : t.collision == PropCollision::None ? "none" : "solid"));
                m.props.push_back(std::move(p));
            }
        }
        else if (type == "objectgroup" && name == "portals")
        {
            for (const auto& o : L.at("objects"))
            {
                Portal p;
                p.name = o.value("name", std::string{});
                const double px = o.value("x", 0.0), py = o.value("y", 0.0);
                p.x = cellOf(px, tw);
                p.y = cellOf(py, th);
                p.w = std::max(1, static_cast<int>(std::ceil((px + o.value("width", 0.0)) / tw)) - p.x);
                p.h = std::max(1, static_cast<int>(std::ceil((py + o.value("height", 0.0)) / th)) - p.y);
                p.targetMap = prop<std::string>(o, "targetMap", "");
                p.targetX   = prop<int>(o, "targetX", -1);
                p.targetY   = prop<int>(o, "targetY", -1);
                p.hasTargetCell = p.targetX >= 0 && p.targetY >= 0;
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
                if (marker == "port") m.portMarkers.push_back(at);   // свой причал для корабля
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