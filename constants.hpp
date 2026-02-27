#pragma once
#include <string_view>

namespace chat::responses {
    using namespace std::literals;

    // Use 'inline' to avoid "multiple definition" errors if included in multiple .cpp files
    inline constexpr std::string_view WELCOME      = "Welcome to the Professional Chat Server!\n"sv;
    inline constexpr std::string_view PROMPT_NAME  = "Please enter your name: "sv;
    inline constexpr std::string_view ROOM_CREATED = "Success: Room created.\n"sv;
    inline constexpr std::string_view HELP_MENU    = "Commands: /list, /create, /join {id},/kick {id} /leave\n"sv;
}

