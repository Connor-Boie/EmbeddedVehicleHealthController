#ifndef COMMAND_PARSER_HPP
#define COMMAND_PARSER_HPP

enum class CommandType
{
    Status,
    HeartbeatOn,
    HeartbeatOff,
    Clear,
    Invalid
};

class CommandParser
{
public:
    [[nodiscard]] static CommandType parse(const char* line);

private:
    [[nodiscard]] static bool stringsEqual(
        const char* first,
        const char* second);
};

#endif
