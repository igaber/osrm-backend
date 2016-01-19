#ifndef SCRIPTING_ENVIRONMENT_HPP
#define SCRIPTING_ENVIRONMENT_HPP

#include <string>
#include <memory>
#include <mutex>
#include <tbb/enumerable_thread_specific.h>

struct lua_State;

namespace osrm
{
namespace extractor
{

/**
 * Creates a lua context and binds osmium way, node and relation objects and
 * ExtractionWay and ExtractionNode to lua objects.
 *
 * Each thread has its own lua state which is implemented with thread specific
 * storage from TBB.
 */
class ScriptingEnvironment
{
  public:
    ScriptingEnvironment() = delete;
    explicit ScriptingEnvironment(const std::string &file_name);

    lua_State *GetLuaState();

  private:
    void InitLuaState(lua_State *lua_state);
    std::mutex init_mutex;
    std::string file_name;
    tbb::enumerable_thread_specific<std::shared_ptr<lua_State>> script_contexts;
};
}
}

#endif /* SCRIPTING_ENVIRONMENT_HPP */
