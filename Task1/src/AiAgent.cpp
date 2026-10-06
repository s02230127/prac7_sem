#include "mafia/AiAgent.hpp"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <sstream>
#include <string>

namespace mafia {
namespace {

using json = nlohmann::json;

constexpr const char* kOllamaUrl = "http://127.0.0.1:11434/api/generate";
constexpr const char* kModel = "qwen2.5:3b";

std::size_t write_callback(char* data, std::size_t size,
                           std::size_t count, void* output) {
    auto* text = static_cast<std::string*>(output);
    const std::size_t bytes = size * count;
    text->append(data, bytes);
    return bytes;
}

std::optional<std::string> post_to_ollama(const json& request) {
    static const bool curl_ready = [] {
        return curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK;
    }();

    if (!curl_ready) {
        return std::nullopt;
    }

    CURL* curl = curl_easy_init();
    if (curl == nullptr) {
        return std::nullopt;
    }

    std::string response;
    const std::string body = request.dump();

    curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, kOllamaUrl);
    curl_easy_setopt(curl, CURLOPT_NOPROXY, "*");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,
                     static_cast<long>(body.size()));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    const CURLcode result = curl_easy_perform(curl);

    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (result != CURLE_OK || status != 200) {
        return std::nullopt;
    }

    return response;
}

bool valid_target(const GameView& view, int self_id, int target_id) {
    if (target_id == self_id) {
        return false;
    }

    for (const VisiblePlayer& player : view.players) {
        if (player.id == target_id && player.alive) {
            return true;
        }
    }

    return false;
}

std::string make_prompt(const GameView& view, int self_id,
                        std::string_view role) {
    std::ostringstream prompt;

    prompt << "Ты игрок " << self_id << " в игре Мафия.\n"
           << "Твоя роль: " << role << ".\n"
           << "Характер: недоверчивый, говоришь кратко и обосновываешь голос.\n"
           << "Раунд: " << view.round << ".\n"
           << "Живые игроки:";

    for (const VisiblePlayer& player : view.players) {
        if (player.alive) {
            prompt << ' ' << player.id;
        }
    }

    prompt << "\nИзвестные союзники мафии:";
    for (int id : view.mafia_allies) {
        prompt << ' ' << id;
    }

    prompt << "\nПубличная история:\n";
    for (const std::string& event : view.history) {
        prompt << event << '\n';
    }

    prompt << "Выбери другого живого игрока для голосования. "
              "Короткая реплика должна подходить дневному обсуждению.";

    return prompt.str();
}

} // namespace

std::optional<AiDecision> AiAgent::decide(
    const GameView& view,
    int self_id,
    std::string_view role
) {
    const json schema = {
        {"type", "object"},
        {"properties", {
            {"action", {{"type", "string"}, {"enum", {"vote"}}}},
            {"message", {{"type", "string"}}},
            {"target", {{"type", "integer"}}},
            {"reasoning", {{"type", "string"}}}
        }},
        {"required", {"action", "message", "target", "reasoning"}}
    };

    const json request = {
        {"model", kModel},
        {"stream", false},
        {"format", schema},
        {"options", {{"temperature", 0}}},
        {"prompt", make_prompt(view, self_id, role)}
    };

    const auto response = post_to_ollama(request);
    if (!response) {
        return std::nullopt;
    }

    try {
        const json outer = json::parse(*response);
        const std::string generated = outer.at("response").get<std::string>();
        const json answer = json::parse(generated);

        if (answer.at("action").get<std::string>() != "vote") {
            return std::nullopt;
        }

        AiDecision decision;
        decision.message = answer.at("message").get<std::string>();
        decision.target_id = answer.at("target").get<int>();
        decision.reasoning = answer.at("reasoning").get<std::string>();

        if (decision.message.empty() || decision.reasoning.empty() ||
            !valid_target(view, self_id, decision.target_id)) {
            return std::nullopt;
        }

        return decision;
    }
    catch (const json::exception&) {
        return std::nullopt;
    }
}

} // namespace mafia
