#include "CommandParser.hpp"

CommandType CommandParser::parse(const char* line)
{
    if (line == nullptr)
    {
        return CommandType::Invalid;
    }

    if (stringsEqual(line, "STATUS"))
    {
        return CommandType::Status;
    }

    if (stringsEqual(line, "FAULTS"))
    {
        return CommandType::Faults;
    }

    if (stringsEqual(line, "HEARTBEAT ON"))
    {
        return CommandType::HeartbeatOn;
    }

    if (stringsEqual(line, "HEARTBEAT OFF"))
    {
        return CommandType::HeartbeatOff;
    }

    if (stringsEqual(line, "INJECT BUTTON FAULT"))
    {
        return CommandType::InjectButtonFault;
    }

    if (stringsEqual(line, "INJECT TIMER FAULT"))
    {
        return CommandType::InjectTimerFault;
    }

    if (stringsEqual(line, "WATCHDOG TEST"))
    {
        return CommandType::WatchdogTest;
    }

    if (stringsEqual(line, "CLEAR"))
    {
        return CommandType::ClearCounters;
    }

    if (stringsEqual(line, "CLEAR FAULTS"))
    {
        return CommandType::ClearFaults;
    }

    if (stringsEqual(line, "CLEAR INJECTED FAULTS"))
    {
        return CommandType::ClearInjectedFaults;
    }

    return CommandType::Invalid;
}

bool CommandParser::stringsEqual(
    const char* first,
    const char* second)
{
    if ((first == nullptr) || (second == nullptr))
    {
        return false;
    }

    while ((*first != '\0') &&
           (*second != '\0'))
    {
        if (*first != *second)
        {
            return false;
        }

        ++first;
        ++second;
    }

    return (*first == '\0') &&
           (*second == '\0');
}
