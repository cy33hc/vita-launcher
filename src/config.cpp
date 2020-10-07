#include <string>
#include <cstring>

#include "config.h"
#include "game.h"
#include "json.h"
#include "fs.h"
#include "net.h"

extern "C" {
	#include "inifile.h"
}

using json = nlohmann::json;

namespace CONFIG {

    void SetupCategory(GameCategory *category, int category_id, const char* category_name, const char* default_prefixes)
    {
        const char* valid_title_prefixes;

		category->id = category_id;
		sprintf(category->title, "%s", category_name);
		category->page_num = 1;
		category->max_page = 1;
		category->view_mode = ReadInt(category->title, CONFIG_VIEW_MODE, -1);
        category->opened = false;
        if (default_prefixes != nullptr)
        {
            valid_title_prefixes = ReadString(category->title, VALID_TITLE_ID_PREFIXES, default_prefixes);
            ParseTitlePrefixes(valid_title_prefixes, category->valid_title_ids);
            WriteString(category->title, VALID_TITLE_ID_PREFIXES, valid_title_prefixes);
        }
		if (category->view_mode == -1)
		{
			category->view_mode = VIEW_MODE_GRID;
			WriteInt(category->title, CONFIG_VIEW_MODE, VIEW_MODE_GRID);
		}

    }

    void LoadConfig()
    {
		OpenIniFile (CONFIG_INI_FILE);

        SetupCategory(&game_categories[VITA_GAMES], VITA_GAMES, "Vita", VITA_TITLE_ID_PREFIXES);
        SetupCategory(&game_categories[PSP_GAMES], PSP_GAMES, "PSP", PSP_TITLE_ID_PREFIXES);
        SetupCategory(&game_categories[PS1_GAMES], PS1_GAMES, "PSX", PS1_TITLE_ID_PREFIXES);
        SetupCategory(&game_categories[PS_MIMI_GAMES], PS_MIMI_GAMES, "PSP Mini", PSP_MINI_TITLE_ID_PREFIXES);
        SetupCategory(&game_categories[PS_MOBILE_GAMES], PS_MOBILE_GAMES, "PS Mobile", PS_MOBILE_TITLE_ID_PREFIXES);
        SetupCategory(&game_categories[NES_GAMES], NES_GAMES, "NES", NES_TITLE_ID_PREFIXES);
        SetupCategory(&game_categories[SNES_GAMES], SNES_GAMES, "SNES", SNES_TITLE_ID_PREFIXES);
        SetupCategory(&game_categories[GB_GAMES], GB_GAMES, "GB", GB_TITLE_ID_PREFIXES);
        SetupCategory(&game_categories[GBA_GAMES], GBA_GAMES, "GBA", GBA_TITLE_ID_PREFIXES);
        SetupCategory(&game_categories[N64_GAMES], N64_GAMES, "N64", N64_TITLE_ID_PREFIXES);
        SetupCategory(&game_categories[PORT_GAMES], PORT_GAMES, "Ports", nullptr);
        SetupCategory(&game_categories[ORIGINAL_GAMES], ORIGINAL_GAMES, "Originals", nullptr);
        SetupCategory(&game_categories[EMULATORS], EMULATORS, "Emulators", nullptr);
        SetupCategory(&game_categories[UTILITIES], UTILITIES, "Utilities", nullptr);
        SetupCategory(&game_categories[HOMEBREWS], HOMEBREWS, "Homebrews", nullptr);
        SetupCategory(&game_categories[FAVORITES], FAVORITES, "Favorites", nullptr);

		WriteIniFile(CONFIG_INI_FILE);
        CloseIniFile();

        if (!FS::FileExists(VITADB_JSON_FILE))
        {
            NET::Download(VITADB_URL, VITADB_JSON_FILE);
        }

        if (FS::FileExists(VITADB_JSON_FILE))
        {
            json vitadb = json::parse(FS::Load(VITADB_JSON_FILE));
            int arrayLength = static_cast<int>(vitadb.size());

            for (int i=0; i < arrayLength; i++)
            {
                std::string type = vitadb[i]["type"].get<std::string>();
                if (type == "1")
                {
                    game_categories[PORT_GAMES].valid_title_ids.push_back(vitadb[i]["titleid"].get<std::string>());
                }
                else if (type == "2")
                {
                    game_categories[ORIGINAL_GAMES].valid_title_ids.push_back(vitadb[i]["titleid"].get<std::string>());
                }
                else if (type == "4")
                {
                    game_categories[UTILITIES].valid_title_ids.push_back(vitadb[i]["titleid"].get<std::string>());
                } else if (type == "5")
                {
                    game_categories[EMULATORS].valid_title_ids.push_back(vitadb[i]["titleid"].get<std::string>());
                }
            }
        }
    }

    void ParseTitlePrefixes(const char* prefix_list, std::vector<std::string> &prefixes)
    {
        std::string prefix = "";
        int length = strlen(prefix_list);
        for (int i=0; i<length; i++)
        {
            char c = prefix_list[i];
            if (c != ' ' && c != '\t' && c != ',')
            {
                prefix += c;
            }
            
            if (c == ',' || i==length-1)
            {
                prefixes.push_back(prefix);
                prefix = "";
            }
        }
    }
}
