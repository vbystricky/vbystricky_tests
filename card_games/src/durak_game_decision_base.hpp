#pragma once

#include "durak_game.hpp"
#include "cards_common.hpp"

// GameHandDecisionRandom
namespace durak_game {
using RandomGen = std::default_random_engine;
using UniformInt = std::uniform_int_distribution<int>;

template <typename Action, typename ActionType>
std::vector<Action> get_valid_actions(const cards_common::CardSet& valid_cards, ActionType MeaningfulAction)
{
    std::vector<Action> actions;
    for (const auto& card : valid_cards) {
        actions.push_back({ MeaningfulAction, card });
    }
    return actions;
}

template <size_t HandsCnt>
std::vector<AttackAction> get_valid_attack_actions(const GameStateConstPtr<HandsCnt>& state) {
    std::vector<AttackAction> actions =
        get_valid_actions<AttackAction, AttackActionType>(
            state->get_active_hand_cards_valid_for_attack(),
            AttackActionType::Attack);
    if (0 < state->get_table_size())
        actions.push_back({ AttackActionType::Pass, {} });
    return actions;
}

template <size_t HandsCnt>
std::vector<DefendAction> get_valid_defend_actions(const GameStateConstPtr<HandsCnt>& state) {
    std::vector<DefendAction> actions =
        get_valid_actions<DefendAction, DefendActionType>(
            state->get_active_hand_cards_valid_for_defend(),
            DefendActionType::Beat);
    actions.push_back({ DefendActionType::Take, {} });
    return actions;
}

template <size_t HandsCnt>
class GameHandDecisionRandom
    : public GameHandDecision<HandsCnt>
{
public:
    static std::string decision_name() {
        return "GameHandDecisionRandom";
    };
public:
    GameHandDecisionRandom()
        : rnd_generator_()
        , rnd_uniform_(0, 36)
    {
        rnd_generator_.seed(make_seed());
    }
    virtual ~GameHandDecisionRandom() {}

    void set_to_game(size_t /*hand_idx*/, Game<HandsCnt>& /*owner_game*/) override {
        rnd_generator_.seed(make_seed());
    }
    void game_reset(const GameStateConstPtr<HandsCnt>& /*state*/) override {
        rnd_generator_.seed(make_seed());
    }
    AttackAction attack_step(const GameStateConstPtr<HandsCnt>& state) override {
        std::vector<AttackAction> actions = get_valid_attack_actions(state);
        return actions[rnd_uniform_(rnd_generator_) % actions.size()];
    }
    DefendAction defend_step(const GameStateConstPtr<HandsCnt>& state) override {
        std::vector<DefendAction> actions = get_valid_defend_actions(state);
        return actions[rnd_uniform_(rnd_generator_) % actions.size()];
    }
protected:
    RandomGen rnd_generator_;
    UniformInt rnd_uniform_;
private:
    static unsigned int make_seed() {
        static RandomGen g_seed_generator_;
        return g_seed_generator_();
    }
};
} // namespace durak_game

// GameHandDecisionContainer
namespace durak_game {
template <size_t HandsCnt, class T>
using DecisionFunction = T(const GameStateConstPtr<HandsCnt>&);

template <size_t HandsCnt>
using AttackDecisionFunction = DecisionFunction<HandsCnt, AttackAction>;

template <size_t HandsCnt>
using DefendDecisionFunction = DecisionFunction<HandsCnt, DefendAction>;

template <
    size_t HandsCnt,
    const char* GameHandDecisionName,
    template<size_t> typename Base,
    AttackDecisionFunction<HandsCnt> Attack,
    DefendDecisionFunction<HandsCnt> Defend>
class GameHandDecisionContainer
    : public Base<HandsCnt>
{
    using BaseClass = Base<HandsCnt>;
public:
    static std::string decision_name() {
        return GameHandDecisionName;
    };
public:
    GameHandDecisionContainer()
    {}

    AttackAction attack_step(const GameStateConstPtr<HandsCnt>& state) override {
        if (!Attack)
            return BaseClass::attack_step(state);
        return Attack(state);
    }
    DefendAction defend_step(const GameStateConstPtr<HandsCnt>& state) override {
        if (!Defend)
            return BaseClass::defend_step(state);
        return Defend(state);
    }
};
}// namespace durak_game