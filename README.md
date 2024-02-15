
# VMangos-eluna
This project is a [VMangos](https://github.com/vmangos/core) version with [Eluna](https://github.com/ElunaLuaEngine/Eluna) code merged.

The original version is from [Vale](https://github.com/ValeTheVioletMote/core) and [CoolZoom](https://github.com/coolzoom),thanks for them!

### Project guidelines
- Build: Same with VMangos,use cmake to generate project file,and build with vs2017.
- Run: Put lua file to lua_scripts folder in your vmangos server folder,and modify the "Eluna.ScriptPath" line in the mangosd.conf file point to "lua_scripts" folder.
- Database: use this https://github.com/bing2008/VMangos-eluna/releases/tag/db_latest
