#ifndef COMMAND_PARSER_HPP
#define COMMAND_PARSER_HPP

enum class CommandType
{
    Status,
    Faults,
    ResetCause,
    HeartbeatOn,
    HeartbeatOff,
    InjectButtonFault,
    InjectTimerFault,
    WatchdogTest,
    ClearCounters,
    ClearFaults,
    ClearInjectedFaults,
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
