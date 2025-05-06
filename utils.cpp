#include "utils.h"


Time::Time(const std::string& s) {
    auto incorrect_format = [&]() {
        throw std::invalid_argument("Incorrect time format: \"" + s + "\", expected hh:mm" );
    };
    if (s.length() != 5 || !isdigit(s[0]) || !isdigit(s[1]) || s[2] != ':' || !isdigit(s[3]) || !isdigit(s[4])) {
        incorrect_format();
    }
    const int hours = std::stoi(s.substr(0, 2));
    const int minutes = std::stoi(s.substr(3, 2));
    if (hours > 23 || minutes > 59) {
        incorrect_format();
    }
    val_ = static_cast<int16_t>(hours * 60 + minutes);
}

Time::Time(const int val) : val_(static_cast<int16_t>(val)) {}

std::string Time::to_string() const {
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << val_ / 60 << ":"
       << std::setfill('0') << std::setw(2) << val_ % 60;
    return ss.str();
}

int16_t Time::GetInMinutes() const {
    return val_;
}

Time& Time::operator+=(Time rhs) {
    val_ += rhs.val_;
    return *this;
}

Time& Time::operator-=(Time rhs) {
    val_ -= rhs.val_;
    return *this;
}

int16_t Time::RoundedHours() const {
    if (val_ < 0) {
        throw std::runtime_error("Trying to get rounded hours for negative time");
    }
    return (val_ + 59) / 60;
}


// Operator overloads for Time
Time operator+(Time lhs, Time rhs) {
    lhs += rhs;
    return lhs;
}

Time operator-(Time lhs, Time rhs) {
    lhs -= rhs;
    return lhs;
}

Event::Event(Time time, int id, std::string body) : time(time), id(id), body(std::move(body)) {}

std::ostream& operator<<(std::ostream& os, const Event& event) {
    os << event.time.to_string() << " " << event.id << " " << event.body;
    return os;
}

std::ostream& operator<<(std::ostream& os, Time time) {
    os << time.to_string();
    return os;
}

bool IsValidClientName(const std::string& name) {
    if (name.empty()) {
        return false;
    }
    return std::ranges::all_of(name, [](char c){
        return std::isalnum(c) || c == '_' || c == '-';
    });
}

ClubManager::ClubManager(const size_t number_of_tables) {
    for (size_t table = 1; table <= number_of_tables; ++table) {
        free_tables.insert(table);
    }
    table_total_time.resize(number_of_tables + 1, Time(0));
    table_total_rounded_hours.resize(number_of_tables + 1);
    table_last_start_time.resize(number_of_tables + 1, Time(-1));
}

[[nodiscard]] bool ClubManager::HaveFreeTables() const {
    return !free_tables.empty();
}

[[nodiscard]] bool ClubManager::IsClientInClub(const std::string& client) const {
    return clients_in_club.contains(client);
}

bool ClubManager::IsClientInQueue(const std::string& client) const {
    return queue_pos.contains(client);
}

bool ClubManager::IsTableFree(size_t table) const {
    CheckTableValidity(table);
    return table_last_start_time[table] == -1;
}

void ClubManager::RegisterClient(const std::string& client) {
    clients_in_club.insert(client);
}

[[nodiscard]] size_t ClubManager::QueueSize() const {
    return waiting_queue.size();
}

bool ClubManager::UnregisterClient(const std::string& client) {
    return clients_in_club.erase(client);
}

void ClubManager::PutToQueue(const std::string& client) {
    waiting_queue.push_back(client);
    queue_pos[client] = --waiting_queue.end();
}

bool ClubManager::RemoveFromQueue(const std::string& client) {
    if (auto node = queue_pos.extract(client)) {
        waiting_queue.erase(node.mapped());
        return true;
    }
    return false;
}

bool ClubManager::PutToTable(const std::string& client, Time time, size_t table) {
    if (table != 0) {
        if (!free_tables.erase(table)) {
            return false;
        }
    } else {
        if (free_tables.empty()) {
            return false;
        }
        table = *free_tables.begin();
        free_tables.erase(free_tables.begin());
    }
    client_table[client] = table;
    OpenTableSession(table, time);
    return true;
}

bool ClubManager::RemoveFromTable(const std::string& client, Time time) {
    if (auto node = client_table.extract(client)) {
        size_t table = node.mapped();
        free_tables.insert(table);
        CloseTableSession(table, time);
        return true;
    }
    return false;
}

bool ClubManager::RemoveFromEverywhere(const std::string& client, Time time) {
    if (!UnregisterClient(client)) {
        return false;
    }
    if (RemoveFromQueue(client)) {
        return true;
    }
    RemoveFromTable(client, time);
    return true;
}

size_t ClubManager::GetClientTableNumber(const std::string& client) const {
    if (auto iter = client_table.find(client); iter != client_table.end()) {
        return iter->second;
    }
    return 0;
}

std::optional<std::string> ClubManager::PropelQueue(Time time) {
    if (!waiting_queue.empty() && PutToTable(waiting_queue.front(), time)) {
        queue_pos.erase(waiting_queue.front());
        std::string client = std::move(waiting_queue.front());
        waiting_queue.pop_front();
        return client;
    }
    return std::nullopt;
}

std::vector<std::string> ClubManager::KickEveryoneOut(Time time) {
    // Оставляет менеджер в некорректном состоянии во имя производительности
    CloseAllTableSessions(time);
    std::vector<std::string> res;
    while (!clients_in_club.empty()) {
        auto node = clients_in_club.extract(clients_in_club.begin());
        res.emplace_back(std::move(node.value()));
    }
    return res;
}

[[nodiscard]] std::pair<int16_t, Time> ClubManager::GetTableStat(size_t table) const {
    CheckTableValidity(table);
    return {table_total_rounded_hours[table], table_total_time[table]};
}

// Private
void ClubManager::OpenTableSession(size_t table, Time start_time) {
    CheckTableValidity(table);
    if (table_last_start_time[table] != -1) {
        throw std::runtime_error("Trying to open already opened table session");
    }
    table_last_start_time[table] = start_time;
}

void ClubManager::CloseTableSession(size_t table, Time end_time) {
    CheckTableValidity(table);
    if (table_last_start_time[table] == -1) {
        throw std::runtime_error("Trying to close already closed table session");
    }
    Time session_duration = end_time - table_last_start_time[table];
    table_total_time[table] += session_duration;
    table_total_rounded_hours[table] += session_duration.RoundedHours();
    table_last_start_time[table] = -1;
}

void ClubManager::CloseAllTableSessions(Time end_time) {
    for (size_t table = 1; table < table_last_start_time.size(); ++table) {
        if (table_last_start_time[table] != -1) {
            CloseTableSession(table, end_time);
        }
    }
}

void ClubManager::CheckTableValidity(size_t table) const {
    if (table == 0 || table >= table_total_time.size()) {
        throw std::out_of_range("Invalid table index");
    }
}