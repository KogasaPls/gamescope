#if HAVE_SCRIPTING

#include "convar.h"
#include "Script/Script.h"
#include "backend.h"

namespace gamescope
{
    namespace detail
    {
        struct ConVarScriptRegistrar
        {
            static sol::usertype<ConCommand> RegisterConCommand()
            {
                return CScriptScopedLock()->new_usertype<ConCommand>( "concommand",
                    "name", &ConCommand::m_pszName,
                    "description", &ConCommand::m_pszDescription,
                    "call", &ConCommand::CallWithArgString );
            }

            template <typename T, bool bAtomic>
            static sol::usertype<ConVar<T, bAtomic>> RegisterConVarType()
            {
                using CV = ConVar<T, bAtomic>;
                auto valueBinding = [] {
                    if constexpr ( bAtomic && std::is_same_v<T, bool> )
                        return sol::property( &CV::Get, []( CV &cv, sol::object value ) {
                            // Numeric switches are common in environment overrides.
                            cv.SetValue( value.get_type() == sol::type::number
                                ? value.as<lua_Number>() != 0 : value.as<bool>() );
                        } );
                    else if constexpr ( bAtomic )
                        return sol::property( &CV::Get, &CV::template SetValue<T> );
                    else
                        return &CV::m_Value;
                };
                return CScriptScopedLock()->new_usertype<CV>( typeid( CV ).name(),
                    "name", &CV::m_pszName,
                    "description", &CV::m_pszDescription,
                    "call", &CV::CallWithArgString,
                    "value", valueBinding() );
            }
        };
    }

    void ConCommand::RegisterScript( std::string_view name, ConCommand *cmd )
    {
        CScriptScopedLock().Manager().Gamescope().Convars.Base[name] = cmd;
    }

    template <typename T, bool bAtomic>
    void ConVar<T, bAtomic>::RegisterScript( std::string_view name, ConVar *cv )
    {
        CScriptScopedLock().Manager().Gamescope().Convars.Base[name] = cv;
    }

#define REGISTER_CONVAR_TYPE_IMPL( T, Atomic, N ) \
    static auto s_ConVarType_##N = detail::ConVarScriptRegistrar::RegisterConVarType<T, Atomic>(); \
    template void ConVar<T, Atomic>::RegisterScript( std::string_view, ConVar<T, Atomic> * );

#define REGISTER_CONVAR_TYPE_EXPAND( T, Atomic, N ) REGISTER_CONVAR_TYPE_IMPL( T, Atomic, N )
#define REGISTER_CONVAR_TYPE( T ) REGISTER_CONVAR_TYPE_EXPAND( T, false, __COUNTER__ )
#define REGISTER_ATOMIC_CONVAR_TYPE( T ) REGISTER_CONVAR_TYPE_EXPAND( T, true, __COUNTER__ )

    static auto s_ConCommandType = detail::ConVarScriptRegistrar::RegisterConCommand();
    REGISTER_CONVAR_TYPE( bool )
    REGISTER_CONVAR_TYPE( int )
    REGISTER_CONVAR_TYPE( float )
    REGISTER_CONVAR_TYPE( uint32_t )
    REGISTER_CONVAR_TYPE( uint64_t )
    REGISTER_CONVAR_TYPE( std::string )
    REGISTER_CONVAR_TYPE( VirtualConnectorStrategy )
    REGISTER_CONVAR_TYPE( TouchClickMode )
    REGISTER_ATOMIC_CONVAR_TYPE( bool )
    REGISTER_ATOMIC_CONVAR_TYPE( float )
    REGISTER_ATOMIC_CONVAR_TYPE( int )
}

#endif
