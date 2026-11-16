#include <catch2/catch_test_macros.hpp>

#include "convar.h"
#include "Script/Script.h"
#include "capture_tonemap_settings.hpp"

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

TEST_CASE("HDR capture LUT settings follow Lua, environment and console writes", "[convar]") {
    CScriptScopedLock script;
    script.Manager().RunScriptText(R"(
        gamescope.convars.capture_sdr_white_nits.value = 203
        gamescope.convars.capture_hdr_peak_nits.value = 1000
        gamescope.convars.capture_tonemap_operator.value = 2
    )");
    REQUIRE(capture_tonemap::CurrentSettings() == capture_tonemap::Resolve(203.f, 1000.f, 2));

    const char *environment[] = {
        "gamescope_capture_sdr_white_nits=180",
        "gamescope_capture_hdr_peak_nits=800",
        "gamescope_capture_tonemap_operator=1",
        nullptr
    };
    script.Manager().ApplyConVarEnvironment(environment);
    REQUIRE(capture_tonemap::CurrentSettings() == capture_tonemap::Resolve(180.f, 800.f, 1));

    std::string_view white[] = {"capture_sdr_white_nits", "500"};
    std::string_view peak[] = {"capture_hdr_peak_nits", "0"};
    std::string_view op[] = {"capture_tonemap_operator", "3"};
    REQUIRE(ConCommand::Exec(white));
    REQUIRE(ConCommand::Exec(peak));
    REQUIRE(ConCommand::Exec(op));
    REQUIRE(capture_tonemap::CurrentSettings() == capture_tonemap::CaptureTonemap{});
}
