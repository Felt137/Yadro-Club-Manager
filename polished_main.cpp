#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <optional>
#include <algorithm>
#include <cctype>
#include <utility>
#include <limits>
#include <ranges>

#include "utils.h"

// Не обёрнут в do while(0), поэтому осторожнее.
// Причина этому, а также причина макросу вместо лямбды - нужно делать continue во внешнем цикле.
#define CHECK_ERROR(condition, msg) \
    if (condition) { \
        std::cout << Event(event.time, 13, msg) << '\n'; \
        continue; \
    }

int main(int argc, char* argv[]) {
    std::string program_name = argv[0];
    if (argc != 2) {
        std::cerr << "Expected usage: " + program_name + " <input_file>" << std::endl;
        return 1;
    }
    std::string filename = argv[1];
    std::ifstream fin(filename);
    if (!fin) {
        std::cerr << "Error opening file \"" + filename + "\"" + '\n';
        return 1;
    }
    std::string line;
    size_t number_of_tables = 0;
    Time opening_time, closing_time;
    size_t hourly_rate;
    std::vector<Event> events;
    Time last_event_time(0);
    auto check_input_error = [&line](bool condition, const std::string& msg) {
        if (condition) {
            std::cout << line << '\n';
            std::exit(0);
        }
    };
    auto string_to_positive_number = [check_input_error] (const std::string& str, const std::string& param_name) {
        try {
             size_t pos = 0;
             unsigned long long res_ull = std::stoull(str, &pos);
             check_input_error(pos != str.length() || res_ull == 0 ||
                 res_ull > std::numeric_limits<size_t>::max(), "Invalid " + param_name);
            return static_cast<size_t>(res_ull);
        } catch (const std::exception& e) {
            check_input_error(true, "Format error in " + param_name + ": " + e.what());
        }
         // Чтобы компилятор был счастлив, на самом деле check_input_error(true, ...) всегда выбросит исключение
        return static_cast<size_t>(0);
    };
    // Считываем количество столов
    check_input_error(!getline(fin, line), "Number of tables not found");
    number_of_tables = string_to_positive_number(line, "number of tables");

    // Считываем время
    check_input_error(!getline(fin, line), "Opening/closing times not found");
    std::stringstream ss(line);
    std::string t1_str, t2_str, extra_data;
    check_input_error(!(ss >> t1_str >> t2_str) || (ss >> extra_data), "Opening/closing times line format error");
    try {
        opening_time = Time(t1_str);
        closing_time = Time(t2_str);
    } catch (const std::exception& e) {
        check_input_error(true, "Invalid opening or closing time: " + std::string(e.what()));
    }
    check_input_error(opening_time > closing_time, "Opening time > closing time");

    // Считываем стоимость часа
    check_input_error(!getline(fin, line), "Hourly rate not found");
    hourly_rate = string_to_positive_number(line, "hourly rate");

    // Считываем события
    while (getline(fin, line)) {
         if (line.empty() || std::all_of(line.begin(), line.end(), ::isspace)) continue;

        std::stringstream ss_event(line);
        std::string time_str, id_str, client_name, table_str;
        size_t id;
        Time time;
        try {
            check_input_error(!(ss_event >> time_str >> id_str), "Invalid event format");
            time = Time(time_str);
            check_input_error(time < last_event_time, "Events are not in chronological order");
            check_input_error(time > closing_time, "Event happens after closing time");
            last_event_time = time;

            id = string_to_positive_number(id_str, "event id");
            check_input_error(id < 1 || id > 4, "Invalid event ID (expected from 1 to 4)");

            check_input_error(!(ss_event >> client_name), "Event body not found");
            check_input_error(!IsValidClientName(client_name), "Invalid client name");
            std::string event_body = std::move(client_name);
            if (id != 2) {
                check_input_error(static_cast<bool>(ss_event >> extra_data), "Extra data after client name");
            } else {
                check_input_error(!(ss_event >> table_str), "Table index not found");
                check_input_error(static_cast<bool>(ss_event >> extra_data), "Extra data after table index");
                size_t table = string_to_positive_number(table_str, "table index");
                check_input_error(table > number_of_tables, "Table index is greater than total number of tables");
                event_body += " " + table_str;
            }
            events.emplace_back(time, id, std::move(event_body));
        } catch (const std::exception& e) {
             check_input_error(true, e.what());
        }
    }

    std::cout << opening_time << '\n';

    ClubManager manager(number_of_tables);
    // Обработка логики событий
    for (auto& event : events) {
        std::cout << event << '\n';
        if (event.id == 2) {
            std::stringstream ss(event.body);
            std::string client;
            size_t table;
            ss >> client >> table;
            CHECK_ERROR(!manager.IsClientInClub(client), "ClientUnknown");
            CHECK_ERROR(!manager.IsTableFree(table), "PlaceIsBusy");
            manager.RemoveFromQueue(client);
            manager.RemoveFromTable(client, event.time);
            manager.PutToTable(client, event.time, table);
            continue;
        }

        const std::string& client = event.body;

        if (event.id == 1) { // Клиент пришёл
            CHECK_ERROR(event.time < opening_time, "NotOpenYet");
            CHECK_ERROR(manager.IsClientInClub(client), "YouShallNotPass");
            manager.RegisterClient(client);
        } else if (event.id == 3) { // Клиент ожидает
            CHECK_ERROR(!manager.IsClientInClub(client), "ClientUnknown");
            CHECK_ERROR(manager.HaveFreeTables(), "ICanWaitNoLonger!");

            if (manager.QueueSize() >= number_of_tables) {
                manager.UnregisterClient(client);
                std::cout << Event(event.time, 11, client) << '\n';
            } else {
                 if (manager.GetClientTableNumber(client) != 0) {
                     throw std::runtime_error("Trying to put sitting client in the waiting queue");
                 }
                 manager.PutToQueue(client);
            }
        } else if (event.id == 4) { // Клиент ушел
            CHECK_ERROR(!manager.RemoveFromEverywhere(client, event.time), "ClientUnknown");
            if (auto propelled_client_opt = manager.PropelQueue(event.time)) {
                 const std::string& propelled_client = propelled_client_opt.value();
                 size_t assigned_table = manager.GetClientTableNumber(propelled_client);
                 std::cout << Event(event.time, 12, propelled_client + " " + std::to_string(assigned_table)) << '\n';
            }
        } else {
            throw std::logic_error("Encountered incorrect event ID when processing events - " + std::to_string(event.id));
        }
    }

    for (auto& client : manager.KickEveryoneOut(closing_time)) {
        std::cout << Event(closing_time, 11, std::move(client)) << '\n';
    }

    std::cout << closing_time << '\n';

    for (size_t table = 1; table <= number_of_tables; ++table) {
        auto [total_hours, total_time] = manager.GetTableStat(table);
        std::cout << table << ' ' << total_hours * hourly_rate << ' ' << total_time << '\n';
    }

    return 0;
}

#undef CHECK_ERROR