#include <catch2/catch_test_macros.hpp>

#include "convar.h"
#include "Script/Script.h"

using namespace gamescope;

TEST_CASE("ConVar", "[convar]") {
	ConVar<bool> cv_bool_var {"foobar", true, "switch on and off"};
	REQUIRE(cv_bool_var.GetName() == "foobar");
	REQUIRE(cv_bool_var.GetDescription() == "switch on and off");
	REQUIRE(cv_bool_var.Get() == true);
	cv_bool_var.SetValue(false);
	REQUIRE(cv_bool_var.Get() == false);

	{
		auto proxy = CScriptScopedLock().Manager().Gamescope().Convars.Base["foobar"];
		REQUIRE(proxy.valid() == true);
		ConVar<bool>* cv = proxy;
		REQUIRE(cv->GetName() == "foobar");
		REQUIRE(cv->GetDescription() == "switch on and off");
		REQUIRE(cv->Get() == false);
		cv->SetValue(true);
		REQUIRE(cv_bool_var.Get() == true);
	}

	{
		auto proxy = CScriptScopedLock().Manager().Gamescope().Convars.Base["nobar"];
		REQUIRE(proxy.valid() == false);
	}

	{
		auto proxy = CScriptScopedLock()->script("return gamescope.convars.foobar");
		REQUIRE(proxy.valid() == true);
		ConVar<bool>* cv = proxy;
		REQUIRE(cv->GetName() == "foobar");
		REQUIRE(cv->GetDescription() == "switch on and off");
		REQUIRE(cv->Get() == true);
		cv->SetValue(false);
		REQUIRE(cv_bool_var.Get() == false);
	}

	{
		auto proxy = CScriptScopedLock()->script("return gamescope.convars.nobar");
		REQUIRE(proxy.valid() == true);
		ConVar<bool>* cv = proxy;
		REQUIRE(cv == nullptr);
	}

	{
		// The trailing space leaves the token not null-terminated.
		REQUIRE(cv_bool_var.Get() == false);
		cv_bool_var.CallWithArgString("true ");
		REQUIRE(cv_bool_var.Get() == true);
	}
}

#if HAVE_PIPEWIRE
#include "pipewire_device_local.hpp"

TEST_CASE("PipeWire allocation sees Lua, environment and console settings", "[convar]") {
    auto &deviceLocal = pipewire_capture::cv_device_local;
    CScriptScopedLock script;
    script.Manager().RunScriptText("gamescope.convars.pipewire_device_local.value = true");
    REQUIRE(deviceLocal.Get());

    const char *environment[] = {"gamescope_pipewire_device_local=0", nullptr};
    script.Manager().ApplyConVarEnvironment(environment);
    REQUIRE_FALSE(deviceLocal.Get());
    environment[0] = "gamescope_pipewire_device_local=1";
    script.Manager().ApplyConVarEnvironment(environment);
    REQUIRE(deviceLocal.Get());

    std::string_view args[] = {"pipewire_device_local", "false"};
    REQUIRE(ConCommand::Exec(args));
    REQUIRE_FALSE(deviceLocal.Get());
    REQUIRE_FALSE(script->script("return gamescope.convars.pipewire_device_local.value").get<bool>());
}
#endif
