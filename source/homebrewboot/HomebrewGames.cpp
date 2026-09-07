#include "HomebrewGames.h"
#include "HomebrewXML.h"
#include "BootHomebrew.h"
#include "settings/CSettings.h"
#include "settings/CGameCategories.hpp"
#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <set>
#include <string>

namespace {
bool regularFile(const std::string &path)
{
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

std::string appID(const std::string &slug, unsigned salt)
{
    // Six-character cover/category key; not a channel title ID.
    unsigned hash = 2166136261u;
    for (size_t i = 0; i < slug.size(); ++i)
        hash = (hash ^ (unsigned char)slug[i]) * 16777619u;
    hash += salt;
    const char alphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string id("H00000");
    for (int i = 5; i > 0; --i) { id[i] = alphabet[hash % 36]; hash /= 36; }
    return id;
}

void addCategory(const std::string &id, const char *name)
{
    if (!name || !*name || strlen(name) > 48) return;
    GameCategories.CategoryList.AddCategory(name);
    if (GameCategories.CategoryList.findCategory(name))
        GameCategories.SetCategory(id, GameCategories.CategoryList.getCurrentID());
}
}

std::vector<discHdr> &HomebrewGames::Headers()
{
    // Keep headers stable while asynchronous cover/banner workers use them.
    static std::vector<discHdr> headers;
    static bool loaded = false;
    if (loaded) return headers;
    std::string root(Settings.homebrewapps_path);
    if (root.empty()) root = "sd:/apps/";
    if (root[root.size()-1] != '/') root += '/';
    DIR *dir = opendir(root.c_str());
    if (!dir) return headers;
    loaded = true;
    std::vector<std::string> folders;
    struct dirent *entry;
    while ((entry = readdir(dir)))
        if (entry->d_name[0] != '.') folders.push_back(entry->d_name);
    closedir(dir);
    std::sort(folders.begin(), folders.end());
    std::set<std::string> ids;
    pugi::xml_document categories;
    std::string categoryPath(Settings.ConfigPath);
    if (!categoryPath.empty() && categoryPath[categoryPath.size()-1] != '/') categoryPath += '/';
    categories.load_file((categoryPath + "homebrew_categories.xml").c_str());
    for (size_t i = 0; i < folders.size(); ++i)
    {
        const std::string folder = root + folders[i] + '/';
        std::string boot = folder + "boot.dol";
        if (!regularFile(boot)) boot = folder + "boot.elf";
        if (!regularFile(boot) || boot.size() >= sizeof(((discHdr*)0)->path)) continue;
        HomebrewXML meta((folder + "meta.xml").c_str());
        discHdr header;
        memset(&header, 0, sizeof(header));
        unsigned salt = 0;
        std::string id;
        do { id = appID(folders[i], salt++); } while (!ids.insert(id).second);
        memcpy(header.id, id.data(), 6);
        header.type = TYPE_GAME_HOMEBREW;
        snprintf(header.path, sizeof(header.path), "%s", boot.c_str());
        const char *name = meta.GetName();
        snprintf(header.title, sizeof(header.title), "%s", name && *name ? name : folders[i].c_str());
        headers.push_back(header);

        // Optional app-local metadata, imported only when no user categories exist.
        const std::vector<unsigned> &existing = GameCategories[id.c_str()];
        if (existing.size() > 1 || (existing.size() == 1 && existing[0] != 0)) continue;
        for (pugi::xml_node app = categories.child("homebrew").child("app"); app; app = app.next_sibling("app"))
            if (folders[i] == app.attribute("folder").value())
                for (pugi::xml_node category = app.child("category"); category; category = category.next_sibling("category"))
                    addCategory(id, category.child_value());
        pugi::xml_document document;
        if (document.load_file((folder + "meta.xml").c_str()))
            for (pugi::xml_node category = document.child("app").child("category"); category; category = category.next_sibling("category"))
                addCategory(id, category.child_value());
        addCategory(id, "Homebrew");
    }
    return headers;
}

int HomebrewGames::Launch(const discHdr *header)
{
    if (!header || header->type != TYPE_GAME_HOMEBREW) return -1;
    FILE *file = fopen(header->path, "rb");
    if (!file) return -1;
    if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return -1; }
    long size = ftell(file);
    if (size < 256 || size > 16 * 1024 * 1024) { fclose(file); return -2; }
    rewind(file);
    u8 *data = (u8*)malloc(size);
    if (!data) { fclose(file); return -2; }
    size_t got = fread(data, 1, size, file);
    fclose(file);
    if (got != (size_t)size) { free(data); return -1; }
    FreeHomebrewBuffer();
    CopyHomebrewMemory(data, 0, size);
    free(data);
    AddBootArgument(header->path);
    std::string path(header->path);
    HomebrewXML meta((path.substr(0, path.find_last_of('/') + 1) + "meta.xml").c_str());
    for (size_t i = 0; i < meta.GetArguments().size(); ++i)
        AddBootArgument(meta.GetArguments()[i].c_str());
    return BootHomebrewFromMem();
}
