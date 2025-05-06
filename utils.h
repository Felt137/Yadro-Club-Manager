#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <set>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <algorithm>
#include <cctype>
#include <utility>
#include <limits>
#include <ranges>

class Time;
struct Event;
class ClubManager;

class Time {
public:
    Time() = default;
    explicit Time(const std::string& s);

    Time(const int val);

    [[nodiscard]] std::string to_string() const;

    auto operator<=>(const Time &) const = default;

    [[nodiscard]] int16_t GetInMinutes() const;

    Time& operator+=(Time rhs);

    Time& operator-=(Time rhs);

    [[nodiscard]] int16_t RoundedHours() const;

private:
    int16_t val_;
};

Time operator+(Time lhs, Time rhs);
Time operator-(Time lhs, Time rhs);

struct Event {
    Event() = default;
    Event(Time time, int id, std::string body);

    std::string body;
    int id;
    Time time;
};

std::ostream& operator<<(std::ostream& os, const Event& event);

std::ostream& operator<<(std::ostream& os, Time time);

bool IsValidClientName(const std::string& name);

class ClubManager {
    // Делает ровно то, что просят - не реализует логику клуба.
    // Также считает суммарное время занятости каждого стола.
public:
    ClubManager() = delete;
    explicit ClubManager(const size_t number_of_tables);

    [[nodiscard]] bool HaveFreeTables() const;

    [[nodiscard]] bool IsClientInClub(const std::string& client) const;
    bool IsClientInQueue(const std::string& client) const;
    bool IsTableFree(size_t table) const;
    void RegisterClient(const std::string& client);

    [[nodiscard]] size_t QueueSize() const;

    bool UnregisterClient(const std::string& client);

    void PutToQueue(const std::string& client);

    bool RemoveFromQueue(const std::string& client);

   bool PutToTable(const std::string& client, Time time, size_t table = 0);
    bool RemoveFromTable(const std::string& client, Time time);
    bool RemoveFromEverywhere(const std::string& client, Time time);
    size_t GetClientTableNumber(const std::string& client) const;
    std::optional<std::string> PropelQueue(Time time);
    std::vector<std::string> KickEveryoneOut(Time time);
    [[nodiscard]] std::pair<int16_t, Time> GetTableStat(size_t table) const;

private:
    std::set<std::string> clients_in_club;
    std::unordered_set<size_t> free_tables;
    std::unordered_map<std::string, size_t> client_table;
    std::list<std::string> waiting_queue;
    std::unordered_map<std::string, std::list<std::string>::iterator> queue_pos;
    std::vector<Time> table_last_start_time;
    std::vector<Time> table_total_time;
    std::vector<int16_t> table_total_rounded_hours;
    void OpenTableSession(size_t table, Time start_time);
    void CloseTableSession(size_t table, Time end_time);
    void CloseAllTableSessions(Time end_time);
    void CheckTableValidity(size_t table) const;
};
