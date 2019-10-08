#pragma once

#include "cards_common.hpp"

#include "durak_game.hpp"
#include "durak_game_decision_base.hpp"

namespace durak_game {
template <size_t HandsCnt>
AttackAction attack_step_opt_less_card(const GameStateConstPtr<HandsCnt>& state) {
    cards_common::CardSet valid_cards = state->get_active_hand_cards_valid_for_attack();
    if (valid_cards.empty())
        return { AttackActionType::Pass, {} };

    cards_common::CardsSuit trump_suit = state->get_trump_suit();
    cards_common::CardSet::const_iterator
        it_min = std::min_element(
            valid_cards.begin(),
            valid_cards.end(),
            [&](const cards_common::Card& a,
                const cards_common::Card& b) {
                    return is_less(a, b, trump_suit);
            });

    if (!state->get_table().empty() && it_min->suit_ == trump_suit)
        return { AttackActionType::Pass, {} };
    return { AttackActionType::Attack, *it_min };
}

template <size_t HandsCnt>
DefendAction defend_step_opt_less_card(const GameStateConstPtr<HandsCnt>& state) {
    cards_common::CardSet valid_cards = state->get_active_hand_cards_valid_for_defend();
    if (valid_cards.empty())
        return { DefendActionType::Take, {} };
    cards_common::CardsSuit trump_suit = state->get_trump_suit();
    return {
        DefendActionType::Beat,
        *(std::min_element(
            valid_cards.begin(),
            valid_cards.end(),
            [&](const cards_common::Card& a,
                const cards_common::Card& b) {
                    return is_less(a, b, trump_suit);
            }))
    };
}

const char GameHandDecisionAttackLessCardName[] = "GameHandDecisionAttackLessCard";
template <size_t HandsCnt>
using GameHandDecisionAttackLessCard =
    GameHandDecisionContainer<HandsCnt, GameHandDecisionAttackLessCardName, GameHandDecisionRandom, attack_step_opt_less_card, nullptr>;

const char GameHandDecisionDefendLessCardName[] = "GameHandDecisionDefendLessCard";
template <size_t HandsCnt>
using GameHandDecisionDefendLessCard =
    GameHandDecisionContainer<HandsCnt, GameHandDecisionDefendLessCardName, GameHandDecisionRandom, nullptr, defend_step_opt_less_card>;
    
const char GameHandDecisionAttackDefendLessCardName[] = "GameHandDecisionAttackDefendLessCard";
template <size_t HandsCnt>
using GameHandDecisionAttackDefendLessCard =
    GameHandDecisionContainer<HandsCnt, GameHandDecisionAttackDefendLessCardName, GameHandDecisionRandom, attack_step_opt_less_card, defend_step_opt_less_card>;
}// namespace durak_game