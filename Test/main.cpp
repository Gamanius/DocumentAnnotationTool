#include <Windows.h>
#include "json.hpp"
#include <iostream>
#include <string>

using json = nlohmann::json;

struct Test {
    int a = 0;
    bool is_false = true;
    std::string w = "Word";
};

void to_json(json& j, const Test& t) {
	j = json{ {"a", t.a}, {"is_false", t.is_false}, {"w", t.w} };
}

void from_json(const json& j, Test& t) {
	j.at("a").get_to(t.a);
	j.at("is_false").get_to(t.is_false);
	j.at("w").get_to(t.w);
}

int main() {
    json j;
    j["Test"] = Test();
    std::cout << j.dump(4);
    
}