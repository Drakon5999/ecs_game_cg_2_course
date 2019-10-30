#include "GameDataLoad.h"
#include "stl.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstring>

#include "Utils.h"
#include <sfml/Graphics.hpp>

#include "Renderer.h"
#include "ECS.h"
#include "EcsComponents.h"

namespace game
{

stl::string PreprocessLvlFile(const stl::string &text)
{
    stl::string out;

    enum State
    {
        READ,
        COMMENT_BEGIN,
        COMMENT,
        NEW_LINE
    } state = READ;

    for (auto c : text)
    {
        switch (state)
        {
        case READ:
            if (c == '/')
                state = COMMENT_BEGIN;
            else if (c == '\r' || c == '\n')
            {
                out.push_back('\n');
                state = NEW_LINE;
            }
            else if (c == '[')
            {
                out.push_back('[');
                out.push_back(' ');
            }
            else if (c == ']')
            {
                out.push_back(' ');
                out.push_back(']');
            }
            else
                out.push_back(c);
            break;
        case COMMENT_BEGIN:
            if (c == '/')
                state = COMMENT;
            else if (c == '\r' || c == '\n')
            {
                out.push_back('/');
                out.push_back('\n');
                state = NEW_LINE;
            }
            else
            {
                out.push_back('/');
                out.push_back(c);
                state = READ;
            }
            break;
        case COMMENT:
            if (c == '\r' || c == '\n')
                state = NEW_LINE;
            break;
        case NEW_LINE:
            if (c == '/')
                state = COMMENT_BEGIN;
            else if (c == '\r' || c == '\n')
                state = NEW_LINE;
            else
            {
                out.push_back(c);
                state = READ;
            }
            break;
        default:
            break;
        }
    }

    return out;
}

stl::string ReadFile(const char *filename)
{
    std::ifstream f(filename);
    stl::string text;
    if (f)
    {
        std::ostringstream ss;
        ss << f.rdbuf();
        text = ss.str().c_str();
    }
    else
        return stl::string();
    f.close();
    return text;
}

stl::vector<char *> SplitProcessedLvlFile(const stl::string &str)
{
    size_t len = str.size();
    char *buf = new char[len + 1];
    strncpy(buf, str.c_str(), len);
    buf[len] = '\0';
    stl::vector<char *> words = SplitText(buf, len, " \n\r\t\v\f,=\"{;", "\"");
    return words;
}

void ClearSplittedText(stl::vector<char *> &words)
{
    if (words.size())
        delete[] words[0];
    words.clear();
}

#define PRINT_AND_BREAK(...) { printf(__VA_ARGS__); breaked = true; break; }
#define PRINT_AND_CONTINUE(...) { printf(__VA_ARGS__); continue; }
#define COLOR_MASK 0xFFFFFFU

static bool LoadLevelLayer(const char *lvlFilename, int lvlLayer, LevelData &data)
{
    stl::string rawText = ReadFile(lvlFilename);
    if (!rawText.size())
    {
        std::cout << "Cannot read " << lvlFilename << std::endl;
        return false;
    }

    bool breaked = false;

    sf::Image levelIm;
    bool hasMap = false;
    stl::map<uint32_t, stl::tuple<stl::vector<char *>::iterator, const char *, const char *>> posedEntities;
    stl::map<uint32_t, ecs::IEntity *> patternedEntities;

    stl::string procText = PreprocessLvlFile(rawText);
    stl::vector<char *> words = SplitProcessedLvlFile(procText);
    auto itr = words.begin();
    while (itr != words.end())
    {
        if (!std::strcmp(*itr, "map"))
        {
            if (!std::strcmp(*++itr, "_filename"))
            {
                if (!levelIm.loadFromFile(*++itr))
                    PRINT_AND_BREAK("Cannot load %s\n", *itr);
                hasMap = true;
            }
        }
        else if (!std::strcmp(*itr, "texture"))
        {
            const char *name = nullptr;
            const char *filename = nullptr;
            while (std::strcmp(*++itr, "}"))
            {
                if (!std::strcmp(*itr, "_name"))
                    name = *++itr;
                else if (!std::strcmp(*itr, "_filename"))
                    filename = *++itr;
            }
            if (!name || !filename)
                PRINT_AND_BREAK("Bad level format\n");
            int id = game::gRenderer.AddTexture(filename);
            data.textures.emplace(name, id);
        }
        else if (!std::strcmp(*itr, "tile"))
        {
            const char *name = nullptr;
            const char *texture = nullptr;
            const char *textureRect = nullptr;
            while (std::strcmp(*++itr, "}"))
            {
                if (!std::strcmp(*itr, "_name"))
                    name = *++itr;
                else if (!std::strcmp(*itr, "_texture"))
                    texture = *++itr;
                else if (!std::strcmp(*itr, "_textureRect"))
                    textureRect = *++itr;
            }
            if (!name || !texture)
                PRINT_AND_BREAK("Bad level format\n");
            auto tex = data.textures.find(texture);
            if (tex == data.textures.end() || tex->second < 0)
                PRINT_AND_BREAK("Invalid texture %s\n", texture);
            int tileId = -1;
            if (textureRect)
            {
                float x, y, w, h;
                sscanf(textureRect, "%f/%f/%f/%f", &x, &y, &w, &h);
                tileId = game::gRenderer.AddTile(tex->second, sf::Vector2i(x, y), sf::Vector2i(w, h));
            }
            else
                tileId = game::gRenderer.AddTile(tex->second);
            data.tiles.emplace(name, tileId);
        }
        else if (!std::strcmp(*itr, "weapon"))
        {
            Weapon weap;
            const char *name = nullptr;
            const char *tile = nullptr;
            const char *size = nullptr;
            while (std::strcmp(*++itr, "}"))
            {
                if (!std::strcmp(*itr, "_name"))
                    name = *++itr;
                else if (!std::strcmp(*itr, "_tile"))
                    tile = *++itr;
                else if (!std::strcmp(*itr, "size"))
                    size = *++itr;
                else if (!std::strcmp(*itr, "shift"))
                    sscanf(*++itr, "%d", &weap.shift);
                else if (!std::strcmp(*itr, "physDamage"))
                    sscanf(*++itr, "%f", &weap.physDamage);
                else if (!std::strcmp(*itr, "magicDamage"))
                    sscanf(*++itr, "%f", &weap.magicDamage);
                else if (!std::strcmp(*itr, "staminaConsumption"))
                    sscanf(*++itr, "%f", &weap.staminaConsumption);
                else if (!std::strcmp(*itr, "manaConsumption"))
                    sscanf(*++itr, "%f", &weap.manaConsumption);
                else if (!std::strcmp(*itr, "pattern"))
                {
                    if (!size)
                        PRINT_AND_BREAK("weapon: size should be defined before pattern\n");
                    ++itr;
                    unsigned w, h;
                    if (sscanf(size, "%u/%u", &w, &h) < 2)
                        PRINT_AND_BREAK("weapon: bad size\n");
                    weap.areaSize = sf::Vector2u(w, h);
                    weap.pattern.clear();
                    unsigned pos = 0;
                    while (**++itr != ']' && pos < (w * h))
                    {
                        weap.pattern.push_back(**itr == '1');
                        ++pos;
                    }
                    if (pos < (w * h))
                        PRINT_AND_BREAK("weapon: too little numbers in pattern\n")
                    ++itr;
                }
            }
            if (breaked)
                break;
            if (!name || !size)
                PRINT_AND_BREAK("A weapon should have _name and size\n");
            unsigned w, h;
            if (sscanf(size, "%u/%u", &w, &h) < 2)
                PRINT_AND_BREAK("weapon: bad size\n");
            weap.areaSize = sf::Vector2u(w, h);
            if (tile)
            {
                auto tilePair = data.tiles.find(tile);
                if (tilePair == data.tiles.end())
                    PRINT_AND_BREAK("weapon: bad tile %s\n", tile);
                weap.tileId = std::max(tilePair->second, 0);
            }
            data.weapons.emplace(name, stl::make_shared<Weapon>(weap));
        }
        else if (!std::strcmp(*itr, "potion"))
        {
            Potion pot;
            const char *name = nullptr;
            const char *tile = nullptr;
            while (std::strcmp(*++itr, "}"))
            {
                if (!std::strcmp(*itr, "_name"))
                    name = *++itr;
                else if (!std::strcmp(*itr, "_tile"))
                    tile = *++itr;
                else if (!std::strcmp(*itr, "healthAmount"))
                    sscanf(*++itr, "%f", &pot.healthAmount);
                else if (!std::strcmp(*itr, "magicAmount"))
                    sscanf(*++itr, "%f", &pot.magicAmount);
                else if (!std::strcmp(*itr, "staminaAmount"))
                    sscanf(*++itr, "%f", &pot.staminaAmount);
            }
            if (!name)
                PRINT_AND_BREAK("A potion should have _name\n");
            if (tile)
            {
                auto tilePair = data.tiles.find(tile);
                if (tilePair == data.tiles.end())
                    PRINT_AND_BREAK("potion: bad tile %s\n", tile);
                pot.tileId = std::max(tilePair->second, 0);
            }
            data.potions.emplace(name, stl::make_shared<Potion>(pot));
        }
        else if (!std::strcmp(*itr, "entity"))
        {
            const char *type = nullptr;
            const char *tile = nullptr;
            const char *colorBinding = nullptr;
            const char *patterned = nullptr;
            while (**++itr == '_')
            {
                if (!std::strcmp(*itr, "_type"))
                    type = *++itr;
                else if (!std::strcmp(*itr, "_tile"))
                    tile = *++itr;
                else if (!std::strcmp(*itr, "_colorBinding"))
                    colorBinding = *++itr;
                else if (!std::strcmp(*itr, "_patterned"))
                    patterned = *++itr;
            }
            --itr;
            if (!type)
                PRINT_AND_BREAK("Bad level format\n");

            if (!colorBinding || patterned)
            {
                if (patterned && (!tile || lvlLayer < 0))
                    PRINT_AND_BREAK("Bad level format\n");

                bool success;
                ecs::EntityDesc edc = ecs::gEntityManager.CreateEntityByName(type, success);
                if (!success)
                    PRINT_AND_BREAK("Invalid entity type %s\n", type);
                ecs::IEntity *ent = ecs::gEntityManager.GetEntityByDesc(edc);

                while (std::strcmp(*++itr, "}"))
                {
                    if (!ent->SetComp(*itr, *(itr + 1)))
                        PRINT_AND_CONTINUE("Bad component %s\n", *itr);
                    ++itr;
                }

                if (tile && lvlLayer >= 0)
                {
                    CompDrawTileInfo ti;
                    auto tpair = data.tiles.find(tile);
                    if (tpair == data.tiles.end())
                        PRINT_AND_BREAK("Bad tile %s\n", tile);
                    ti.tileId = tpair->second;
                    ti.layer = lvlLayer;
                    CompDrawTileInfo *comp = ent->GetComp<CompDrawTileInfo>();
                    if (comp)
                        *comp = ti;
                }

                if (patterned)
                {
                    uint32_t color;
                    if (!sscanf(colorBinding, "#%X", &color))
                        PRINT_AND_BREAK("Invalid color %s\n", colorBinding);
                    color &= COLOR_MASK;
                    patternedEntities.emplace(color, ent);
                }
            }
            else
            {
                if (!tile || lvlLayer < 0)
                    PRINT_AND_BREAK("Bad level format\n");
                uint32_t color;
                if (!sscanf(colorBinding, "#%X", &color))
                    PRINT_AND_BREAK("Invalid color %s\n", colorBinding);
                color &= COLOR_MASK;
                posedEntities.emplace(color,
                    stl::tuple<stl::vector<char *>::iterator, const char *, const char *>(itr + 1, type, tile));
                while (std::strcmp(*++itr, "}"))
                {}
            }
        }
        ++itr;
    }

    if (breaked)
    {
        ClearSplittedText(words);
        return false;
    }

    if (!hasMap)
    {
        ClearSplittedText(words);
        return true;
    }

    sf::Vector2u size = levelIm.getSize();

    stl::map<uint32_t, game::PosPattern> patterns;
    for (auto &pattern : patternedEntities)
    {
        game::PosPattern pp;
        pp.Init(size.x, size.y, 0);
        patterns.emplace(pattern.first, pp);
    }
    
    for (int i = 0; i < int(size.x); ++i)
    {
        for (int j = 0; j < int(size.y); ++j)
        {
            auto posEnt = posedEntities.find(levelIm.getPixel(i, size.y - 1 - j).toInteger() >> 8);
            if (posEnt != posedEntities.end())
            {
                bool success;
                const char *type = stl::get<1>(posEnt->second);
                const char *tile = stl::get<2>(posEnt->second);
                ecs::EntityDesc edc = ecs::gEntityManager.CreateEntityByName(type, success);
                if (!success)
                    PRINT_AND_CONTINUE("Invalid entity type %s\n", type);
                ecs::IEntity *ent = ecs::gEntityManager.GetEntityByDesc(edc);
                       
                // TODO: Optimize it.
                // There isn't need to read values for each copy.
                auto itr = stl::get<0>(posEnt->second);
                while (std::strcmp(*itr, "}"))
                {
                    if (!ent->SetComp(*itr, *(itr + 1)))
                        PRINT_AND_CONTINUE("Bad component %s\n", *itr);
                    ++itr;
                    ++itr;
                }

                CompDrawTileInfo ti;
                ti.layer = lvlLayer;
                auto tpair = data.tiles.find(tile);
                if (tpair == data.tiles.end())
                    PRINT_AND_CONTINUE("Bad tile %s\n", tile);
                ti.tileId = tpair->second;

                CompDrawTileInfo *entTi = ent->GetComp<CompDrawTileInfo>();
                CompPos *entCp = ent->GetComp<CompPos>();
                if (entTi && entCp)
                {
                    *entTi = ti;
                    entCp->v = sf::Vector2i(i, j);
                }
            }
            auto patPair = patterns.find(levelIm.getPixel(i, size.y - 1 - j).toInteger() >> 8);
            if (patPair != patterns.end())
            {
                patPair->second.pattern[i][j] = 1;
            }
        }
    }

    for (auto &pattern : patternedEntities)
    {
        CompPosPattern *pp = pattern.second->GetComp<CompPosPattern>();
        if (pp)
            pp->positions = patterns[pattern.first];
    }

    ClearSplittedText(words);

    return true;
}

LevelData LoadLevel(const char *lvlFilename)
{
    LevelData data;

    stl::string rawText = ReadFile(lvlFilename);
    if (!rawText.size())
    {
        std::cout << "Cannot read " << lvlFilename << std::endl;
        return data;
    }

    stl::string procText = PreprocessLvlFile(rawText);
    stl::vector<char *> words = SplitProcessedLvlFile(procText);
    auto itr = words.begin();
    
    bool breaked = false;

    while (itr != words.end())
    {
        if (!std::strcmp(*itr, "layer"))
        {
            const char *layer = nullptr;
            const char *filename = nullptr;
            while (std::strcmp(*++itr, "}"))
            {
                if (!std::strcmp(*itr, "_layer"))
                    layer = *++itr;
                else if (!std::strcmp(*itr, "_filename"))
                    filename = *++itr;
            }
            if (!filename)
                PRINT_AND_BREAK("Bad level format\n");
            int layerNum;
            if (layer)
            {
                if (!sscanf(layer, "%d", &layerNum))
                    PRINT_AND_BREAK("Bad layer %s\n", layer);
            }
            else
                layerNum = -1;
                
            LoadLevelLayer(filename, layerNum, data);
        }
        ++itr;
    }

    return data;
}

}
