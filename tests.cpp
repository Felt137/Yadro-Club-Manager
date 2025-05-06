#include "catch_amalgamated.hpp"
#include "utils.h"
#include <sstream>

TEST_CASE("Valid time tesing", "[Time]") {
    Time t1("00:00");
    REQUIRE(t1.to_string() == "00:00");
    Time t2("23:59");
    REQUIRE(t2.GetInMinutes() == 23 * 60 + 59);
    REQUIRE(t2.to_string() == "23:59");
    Time t3 = t1 + Time("01:15");
    REQUIRE(t3.to_string() == "01:15");
    t3 += Time("00:45");
    REQUIRE(t3.to_string() == "02:00");
    Time diff = Time("02:30") - Time("01:00");
    REQUIRE(diff.GetInMinutes() == 90);
    REQUIRE(diff.RoundedHours() == 2);
}

TEST_CASE("Invalid time", "[Time]") {
    REQUIRE_THROWS_AS(Time("") , std::invalid_argument);
    REQUIRE_THROWS_AS(Time("24:00"), std::invalid_argument);
    REQUIRE_THROWS_AS(Time("12:60"), std::invalid_argument);
    REQUIRE_THROWS_AS(Time("abcd"), std::invalid_argument);
}

TEST_CASE("Event ostream", "[Event]") {
    Event e(Time("10:05"), 2, "alice 1");
    std::ostringstream oss;
    oss << e;
    REQUIRE(oss.str() == "10:05 2 alice 1");
}

TEST_CASE("Client registration and queue management", "[ClubManager]") {
    ClubManager mgr(2);
    REQUIRE(mgr.HaveFreeTables());
    REQUIRE_FALSE(mgr.IsClientInClub("bob"));
    mgr.RegisterClient("bob");
    REQUIRE(mgr.IsClientInClub("bob"));
    mgr.PutToQueue("bob");
    REQUIRE(mgr.QueueSize() == 1);
    REQUIRE(mgr.IsClientInQueue("bob"));
    REQUIRE(mgr.RemoveFromQueue("bob"));
    REQUIRE(mgr.QueueSize() == 0);
}

TEST_CASE("Tables", "[ClubManager]") {
    ClubManager mgr(1);
    mgr.RegisterClient("c1");
    REQUIRE(mgr.HaveFreeTables());
    REQUIRE(mgr.PutToTable("c1", Time("09:00"), 1));
    REQUIRE_FALSE(mgr.HaveFreeTables());
    REQUIRE(mgr.IsTableFree(1) == false);
    REQUIRE(mgr.RemoveFromTable("c1", Time("10:15")));
    auto [rounded, total] = mgr.GetTableStat(1);
    REQUIRE(rounded == 2);
    REQUIRE(total.to_string() == "01:15");
}

TEST_CASE("Help", "[ClubManager]") {
    ClubManager mgr(1);
    mgr.RegisterClient("a"); mgr.RegisterClient("b");
    REQUIRE(mgr.PutToTable("a", Time("09:00"), 1));
    mgr.PutToQueue("b");
    REQUIRE(mgr.RemoveFromTable("a", Time("09:30")));
    auto opt = mgr.PropelQueue(Time("09:30"));
    REQUIRE(opt.has_value());
    REQUIRE(opt.value() == "b");
    REQUIRE(mgr.GetClientTableNumber("b") == 1);
}

TEST_CASE("KICKEMOUT", "[ClubManager]") {
    ClubManager mgr(3);
    mgr.RegisterClient("x"); mgr.RegisterClient("y"); mgr.RegisterClient("a");
    auto list = mgr.KickEveryoneOut(Time("19:00"));
    REQUIRE(list.size() == 3);
    std::ranges::sort(list);
    REQUIRE(list == std::vector<std::string>{"a","x","y"});
}
