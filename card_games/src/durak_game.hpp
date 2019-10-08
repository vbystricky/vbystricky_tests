#pragma once

#include "cards_common.hpp"

#include <array>
#include <string>
#include <memory>

namespace durak_game {
constexpr size_t hands_start_amount = 6;
    
enum class Stage {
    NoneStage,
    AttackStage, // ходит аттакующая рука
    DefendStage, // отбиваем последнюю карту
    AppendStage, // добавляем 
};

enum class AttackActionType {
    Attack, // ходим картой, если стол пустой - любой, если не пустой, то достоинcтво должно совпадать с одной из тех что на столе
    Pass,   // закончили атаку
};

enum class DefendActionType {
    Beat,   // ходим картой старше чем последняя на столе
    Take    // отбиваться нечем, забираем стол в руку
};

template <class ActionType>
struct Action {
    ActionType action_type;
    cards_common::Card card;
    friend bool operator==(const Action& l, const Action& r) {
        return (l.action_type == r.action_type) && (l.card == r.card);
    }
};

using AttackAction = Action<AttackActionType>;
using DefendAction = Action<DefendActionType>;

enum class GameStepResult {
    None,
    Beat,
    Take
};

template <size_t HandsCnt>
class GameState
{
public:
    virtual ~GameState() {}

    virtual size_t get_step_start_hand_idx() const = 0;
    virtual Stage get_current_stage() const = 0;
    virtual size_t get_step_rest_append_cards_cnt() const = 0;
    virtual uint64_t get_step_attacker_hands_mask() const = 0;

    virtual size_t get_active_hand_idx() const = 0;
    virtual size_t get_attack_hand_idx() const = 0;
    virtual size_t get_defend_hand_idx() const = 0;

    virtual const cards_common::Card& get_trump_card() const = 0;

    virtual size_t get_deck_size() const = 0;
    virtual size_t get_hands_size(size_t hand_idx) const = 0;
    virtual size_t get_rest_cards_size() const = 0;

    virtual const cards_common::CardSet& get_active_hand() const = 0;
    virtual const cards_common::CardSet& get_garbage() const = 0;
    virtual const cards_common::CardList& get_table() const = 0;
    virtual cards_common::CardDeck get_rest_cards() const = 0;
public:
    cards_common::CardsSuit get_trump_suit() const {
        return get_trump_card().suit_;
    }
    size_t get_table_size() const {
        return get_table().size();
    }
    virtual size_t get_garbage_size() const {
        return get_garbage().size();
    }
    cards_common::CardSet get_active_hand_cards_valid_for_attack() const {
        cards_common::CardSet cards = get_active_hand();
        if (0 == get_table_size()) {
            return cards;
        }
        std::set<cards_common::CardsValue> valid_values;
        for (auto card : get_table()) {
            valid_values.insert(card.value_);
        }
        cards_common::CardSet valid_cards;
        for (const cards_common::Card& card : cards)
        {
            if (valid_values.count(card.value_))
                valid_cards.insert(card);
        }
        return valid_cards;
    }
    cards_common::CardSet get_active_hand_cards_valid_for_defend() const {
        cards_common::CardSet cards = get_active_hand();
        if (0 == get_table_size())
            throw "Can't defend with empty table";
        cards_common::CardSet valid_cards;
        const cards_common::Card& last_card = get_table().back();
        for (auto card : cards)
        {
            if ((last_card.suit_ == card.suit_ && last_card.value_ < card.value_) ||
                (last_card.suit_ != card.suit_ && get_trump_suit() == card.suit_))
                valid_cards.insert(card);
        }
        return valid_cards;
    }
};

template <size_t HandsCnt>
using GameStateConstPtr = std::shared_ptr<const GameState<HandsCnt>>;

template <size_t HandsCnt>
class GameRenderState;

template <size_t HandsCnt>
class Game;

template <size_t HandsCnt>
class GameHandDecision
{
public:
    GameHandDecision() {}
    virtual ~GameHandDecision() {}
        
    virtual void set_to_game(size_t hand_idx, Game<HandsCnt>& owner_game) = 0;
    virtual void game_reset(const GameStateConstPtr<HandsCnt>& state) = 0;
    virtual AttackAction attack_step(const GameStateConstPtr<HandsCnt>& state) = 0;
    virtual DefendAction defend_step(const GameStateConstPtr<HandsCnt>& state) = 0;
};

class GameChangingStageEvent
{
public:
    GameChangingStageEvent() {}
    virtual ~GameChangingStageEvent() {}

    virtual void stage_changing(Stage old_stage, Stage new_stage) = 0;
};

class GameStepEvent
{
public:
    GameStepEvent() {}
    virtual ~GameStepEvent() {}

    //virtual void step_start() = 0;
    virtual void attack_action(const AttackAction& action) = 0;
    virtual void defend_action(const DefendAction& action) = 0;
    virtual void append_action(const AttackAction& action) = 0;
    virtual void step_end(GameStepResult result) = 0;
};

template <size_t HandsCnt>
class Game
{
    using GameHandDecisionPtr = std::unique_ptr<GameHandDecision<HandsCnt> >;
    using GameChangingStageEventPtr = std::unique_ptr<GameChangingStageEvent>;
    using GameStepEventPtr = std::unique_ptr<GameStepEvent>;
    friend GameRenderState<HandsCnt>;
private:
    class GameStep
    {
    public:
        GameStep(Game<HandsCnt>& game)
            : game_(game)
            , start_hand_idx_(0)
            , attack_hand_idx_(0)
            , defend_hand_idx_(-1)
            , rest_append_cards_cnt_(0)
            , cur_stage_(Stage::NoneStage)
            , attacker_hands_mask_() {}

        void init(size_t start_hand_idx) {
            start_hand_idx_ = attack_hand_idx_ = start_hand_idx;
            defend_hand_idx_ = game_.next_nonempty_hand(start_hand_idx + 1);
            rest_append_cards_cnt_ = 0;
            reset_attacker_hands_mask();
            change_stage(Stage::AttackStage);
        }
        void init(const GameStateConstPtr<HandsCnt>& state) {
            start_hand_idx_ = state->get_step_start_hand_idx();
            attack_hand_idx_ = state->get_attack_hand_idx();
            defend_hand_idx_ = state->get_defend_hand_idx();
            rest_append_cards_cnt_ = state->get_step_rest_append_cards_cnt();
            uint64_t mask = state->get_step_attacker_hands_mask();
            for (size_t i = 0; i < HandsCnt; i++, mask >>= 1) {
                attacker_hands_mask_[i] = (0 != (mask & 0x1));
            }
            change_stage(state->get_current_stage());
        }
        GameStepResult run() {
            GameStepResult result = GameStepResult::None;
            for (; result == GameStepResult::None;) {
                result = make_step();
            }
            change_stage(Stage::NoneStage);
            game_.fire_step_end_event(result);
            return result;
        }

        Stage get_current_stage() const {
            return cur_stage_;
        }
        size_t get_step_rest_append_cards_cnt() const {
            return rest_append_cards_cnt_;
        }
        uint64_t get_step_attacker_hands_mask() const {
            uint64_t mask = 0;
            uint64_t bit = 1;
            for (size_t i = 0; i < HandsCnt; i++, bit <<= 1) {
                if (attacker_hands_mask_[i])
                    mask |= bit;
            }
            return mask;
        }

        size_t get_start_hand_idx() const {
            return start_hand_idx_;
        }
        size_t get_attack_hand_idx() const {
            return attack_hand_idx_;
        }
        size_t get_defend_hand_idx() const {
            return defend_hand_idx_;
        }
        size_t get_active_hand_idx() const {
            switch (cur_stage_) {
            case Stage::AttackStage:
            case Stage::AppendStage:
                return attack_hand_idx_;
            case Stage::DefendStage:
                return defend_hand_idx_;
            }
            return -1;
        }
    private:
        GameStepResult attack_step() {
            AttackAction action = game_.make_attack_decision(attack_hand_idx_);
            GameStepResult result = GameStepResult::None;
            switch (action.action_type) {
            case AttackActionType::Attack:
                if (!game_.can_attack(action.card))
                    throw std::runtime_error("Unable to attack by card");
                game_.hands_[attack_hand_idx_].erase(action.card);
                game_.table_.push_back(action.card);
                reset_attacker_hands_mask();
                change_stage(Stage::DefendStage);
                break;
            case AttackActionType::Pass:
                attacker_hands_mask_[attack_hand_idx_] = false;
                const int next_attack_hand_idx = next_attack_hand();
                if (-1 == next_attack_hand_idx) {
                    result = GameStepResult::Beat;
                } else {
                    attack_hand_idx_ = next_attack_hand_idx;
                }
                break;
            };
            game_.fire_attack_action_event(action);
            return result;
        }
        GameStepResult defend_step() {
            assert(!game_.hands_[defend_hand_idx_].empty());
            DefendAction action = game_.make_defend_decision(defend_hand_idx_);
            GameStepResult result = GameStepResult::None;
            switch (action.action_type) {
            case DefendActionType::Beat:
                if (!game_.can_defend(action.card))
                    throw std::runtime_error("Unable to defend by card");

                game_.hands_[defend_hand_idx_].erase(action.card);
                game_.table_.push_back(action.card);
                if (game_.hands_[defend_hand_idx_].empty()) {
                    result = GameStepResult::Beat;
                } else {
                    change_stage(Stage::AttackStage);
                }
                break;
            case DefendActionType::Take:
                rest_append_cards_cnt_ = game_.hands_[defend_hand_idx_].size() - 1;
                change_stage(Stage::AppendStage);
                break;
            };
            game_.fire_defend_action_event(action);
            return result;
        }
        GameStepResult append_step() {
            if (0 == rest_append_cards_cnt_ || -1 == attack_hand_idx_) {
                return GameStepResult::Take;
            }
            AttackAction action = game_.make_attack_decision(attack_hand_idx_);
            switch (action.action_type) {
            case AttackActionType::Attack:
                if (!game_.can_attack(action.card))
                    throw std::runtime_error("Unable to attack by card");
                game_.hands_[attack_hand_idx_].erase(action.card);
                game_.table_.push_back(action.card);
                rest_append_cards_cnt_--;
                reset_attacker_hands_mask();
                break;
            case AttackActionType::Pass:
                attacker_hands_mask_[attack_hand_idx_] = false;
                attack_hand_idx_ = next_attack_hand();
                break;
            };
            game_.fire_append_action_event(action);
            return GameStepResult::None;
        }
        GameStepResult make_step() {
            switch (cur_stage_) {
            case Stage::AttackStage:
                return attack_step();
            case Stage::DefendStage:
                return defend_step();
            case Stage::AppendStage:
                return append_step();
            };
            throw std::runtime_error("Invalid stage");
        }

        void reset_attacker_hands_mask() {
            for (int i = 0; i < HandsCnt; i++) {
                attacker_hands_mask_[i] =
                    (i != defend_hand_idx_ && game_.hands_[i].empty());
            }
        }
        int next_attack_hand() const {
            for (int i = 0; i < HandsCnt; i++) {
                const int hand_idx = (attack_hand_idx_ + i) % HandsCnt;
                if (attacker_hands_mask_[hand_idx]) {
                    return hand_idx;
                }
            }
            return -1;
        }

        void change_stage(Stage new_stage) {
            game_.fire_stage_changing_event(cur_stage_, new_stage);
            cur_stage_ = new_stage;
        }
        void start_step() {
            
        }
    private:
        Game<HandsCnt>& game_;
        size_t start_hand_idx_;
        size_t attack_hand_idx_;
        size_t defend_hand_idx_;
        size_t rest_append_cards_cnt_;

        Stage cur_stage_;

        std::array<bool, HandsCnt> attacker_hands_mask_;
    };
    class GameStateImpl
        : public GameState<HandsCnt>
    {
    public:
        GameStateImpl(const Game<HandsCnt>& game)
            : game_(game)
        {}

        size_t get_step_start_hand_idx() const override { return game_.get_step_start_hand_idx(); }
        Stage get_current_stage() const override { return game_.get_current_stage(); }
        size_t get_step_rest_append_cards_cnt() const override {
            return game_.get_step_rest_append_cards_cnt();
        }
        uint64_t get_step_attacker_hands_mask() const override {
            return game_.get_step_attacker_hands_mask();
        }

        size_t get_active_hand_idx() const override { return game_.get_active_hand_idx(); }
        size_t get_attack_hand_idx() const override { return game_.get_attack_hand_idx(); }
        size_t get_defend_hand_idx() const override { return game_.get_defend_hand_idx(); }

        const cards_common::Card& get_trump_card() const override { return game_.get_trump_card(); }

        size_t get_deck_size() const override { return game_.get_deck_size(); }
        size_t get_hands_size(size_t hand_idx) const override { return game_.get_hand_size(hand_idx); }
        size_t get_rest_cards_size() const override { return game_.get_rest_cards_size(); }

        const cards_common::CardSet& get_active_hand() const override { return game_.get_active_hand(); }
        const cards_common::CardSet& get_garbage() const override { return game_.get_garbage(); }
        const cards_common::CardList& get_table() const override { return game_.get_table(); }
        cards_common::CardDeck get_rest_cards() const override { return game_.get_rest_cards(); }
    private:
        const Game<HandsCnt>& game_;
    };
public:
    Game()
        : loser_hand_idx_(HandsCnt)
        , game_state_(std::make_shared<GameStateImpl>(*this))
        , game_step_(*this)
        , deck_(cards_common::CardDeckType::CardDeck36)
        , trump_card_() {}

    virtual ~Game() {}
public:
    void set_hand_decision(size_t hand_idx, GameHandDecisionPtr decision) {
        hand_decision_[hand_idx] = std::move(decision);
        hand_decision_[hand_idx]->set_to_game(hand_idx, *this);
    }
    GameHandDecisionPtr& get_hand_decision() {
        return hand_decision_[hand_idx];
    }
    void add_stage_changing_event(GameChangingStageEventPtr event) {
        changing_stage_events_.push_back(std::move(event));
    }
    void add_step_event(GameStepEventPtr event) {
        step_events_.push_back(std::move(event));
    }
public:
    void init(int start_hand_idx = -1, unsigned int seed = (int)time(0)) {
        clear();
        dial(seed);

        start_hand_idx =
            (start_hand_idx < 0 || start_hand_idx >= HandsCnt)
            ? (int)get_hand_with_smaller_trump()
            : start_hand_idx;

        game_step_.init((size_t)start_hand_idx);
        loser_hand_idx_ = -1;
        decision_reset_game();
    }
    void init(const GameStateConstPtr<HandsCnt>& state, unsigned int seed = (int)time(0)) {
        trump_card_ = state->get_trump_card();
        garbage_ = state->get_garbage();
        table_ = state->get_table();
        hands_[state->get_active_hand_idx()] = state->get_active_hand();

        deck_ = std::move(state->get_rest_cards());
        if (0 == state->get_deck_size())
            deck_.shuffle(seed);
        else
            deck_.shuffle_wo_last(seed); // сохраняем trump_card последней в колоде

        for (size_t hand = 0; hand < HandsCnt; hand++) {
            if (hand == state->get_active_hand_idx())
                continue;
            hands_[hand].clear();
            for (size_t cnt = 0; cnt < state->get_hands_size(hand); cnt++) {
                hands_[hand].insert(deck_.pop_front());
            }
        }
        loser_hand_idx_ = -1;
        game_step_.init(state);
        decision_reset_game();
    }
    // возвращает проигравшую руку, или -1 для ничьей
    int run() {
        for (;;) {
            GameStepResult step_result = game_step_.run();
            switch (step_result) {
            case GameStepResult::Take:
                table_to_hand(game_step_.get_defend_hand_idx());
                break;
            case GameStepResult::Beat:
                table_to_garbage();
                break;
            default:
                throw "Invalid result of game step";
            }
            pick_up_all(game_step_.get_start_hand_idx());
            if (check_game_end())
                break;
            switch (step_result) {
            case GameStepResult::Beat:
                game_step_.init(next_nonempty_hand(game_step_.get_defend_hand_idx()));
                break;
            case GameStepResult::Take:
                game_step_.init(next_nonempty_hand(game_step_.get_defend_hand_idx() + 1));
                break;
            }
        }
        return loser_hand_idx_;
    }
public:
    const cards_common::Card& get_trump_card() const {
        return trump_card_;
    }
    Stage get_current_stage() const {
        return game_step_.get_current_stage();
    }
    size_t get_step_start_hand_idx() const {
        return game_step_.get_start_hand_idx();
    }
    size_t get_step_rest_append_cards_cnt() const {
        return game_step_.get_step_rest_append_cards_cnt();
    }
    uint64_t get_step_attacker_hands_mask() const {
        return game_step_.get_step_attacker_hands_mask();
    }

    size_t get_attack_hand_idx() const {
        return game_step_.get_attack_hand_idx();
    }
    size_t get_defend_hand_idx() const {
        return game_step_.get_defend_hand_idx();
    }
    size_t get_active_hand_idx() const {
        return game_step_.get_active_hand_idx();
    }

    size_t get_deck_size() const {
        return deck_.size();
    }
    size_t get_hand_size(size_t hand_idx) const {
        return hands_[hand_idx].size();
    }
    size_t get_rest_cards_size() const {
        return 
            cards_common::get_deck_size(cards_common::CardDeckType::CardDeck36)
            - hands_[get_active_hand_idx()].size()
            - table_.size()
            - garbage_.size();
    }
    const cards_common::CardSet& get_active_hand() const {
        return hands_[get_active_hand_idx()];
    }
    const cards_common::CardSet& get_garbage() const {
        return garbage_;
    }
    const cards_common::CardList& get_table() const {
        return table_;
    }

    // возвращает объединение deck и всех неактивных рук
    // если trump_card в колоде, то она кладется вниз возвращаемой колоды
    cards_common::CardDeck get_rest_cards() const {
        cards_common::CardDeck deck;
        const size_t active_hand = get_active_hand_idx();
        for (size_t hand_idx = 1; hand_idx < HandsCnt; hand_idx++) {
            const cards_common::CardSet& hand = hands_[(active_hand + hand_idx) % HandsCnt];
            deck.append(hand.cbegin(), hand.cend());
        }
        if (!deck_.empty())
            deck.append(deck_);
        return deck;
    }
public:
    bool can_attack(const cards_common::Card& card) const {
        if (0 == hands_[get_attack_hand_idx()].count(card))
            return false;
        if (table_.empty())
            return true;
        for (auto card_on_table : table_)
        {
            if (card_on_table.value_ == card.value_)
                return true;
        }
        return false;
    }
    bool can_defend(const cards_common::Card& card) const {
        if (0 == hands_[get_defend_hand_idx()].count(card))
            return false;
        if (table_.empty())
            throw "Unable to apply defend action on empty table";

        const cards_common::Card &table_last = table_.back();
        if (table_last.suit_ == card.suit_ &&
            table_last.value_ < card.value_)
            return true;
        if (table_last.suit_ != trump_card_.suit_ &&
            card.suit_ == trump_card_.suit_)
            return true;
        return false;
    }
private:
    void clear()
    {
        deck_.clear();

        for (size_t i = 0; i < HandsCnt; i++)
            hands_[i].clear();

        garbage_.clear();
        table_.clear();
    }
    void dial(unsigned int seed)
    {
        deck_.fill(cards_common::CardDeckType::CardDeck36);
        deck_.shuffle(seed);

        pick_up_all(0);

        trump_card_ = deck_.pop_front();
        deck_.push_back(trump_card_);
    }

    void hand_pick_up(size_t hand_idx)
    {
        while (!deck_.empty() && hands_[hand_idx].size() < hands_start_amount)
        {
            hands_[hand_idx].insert(deck_.pop_front());
        }
    }
    void pick_up_all(size_t from_hand_idx)
    {
        for (size_t i = 0; i < HandsCnt; i++)
        {
            hand_pick_up((from_hand_idx + i) % HandsCnt);
        }
    }

    size_t get_hand_with_smaller_trump()
    {
        std::array<cards_common::CardsValue, HandsCnt> small_trump;
        for (size_t i = 0; i < HandsCnt; i++)
        {
            small_trump[i] = cards_common::CardsValue::ValueMax;
            for (const auto card : hands_[i])
            {
                if (trump_card_.suit_ != card.suit_)
                    continue;
                if (small_trump[i] > card.value_)
                    small_trump[i] = card.value_;
            }
        }
        size_t result = 0;
        for (size_t i = 1; i < HandsCnt; i++)
        {
            if (small_trump[i] < small_trump[result])
                result = i;
        }
        return result;
    }

    bool check_game_end()
    {
        loser_hand_idx_ = -1;
        size_t non_empty_hands_cnt = 0;
        for (int i = 0; i < HandsCnt; i++)
        {
            if (!hands_[i].empty())
            {
                non_empty_hands_cnt++;
                loser_hand_idx_ = i;
            }
        }
        switch (non_empty_hands_cnt)
        {
        case 0:
            loser_hand_idx_ = -1;
            return true;
        case 1:
            return true;
        }
        return false;
    }
    size_t next_nonempty_hand(size_t hand)
    {
        hand = hand % HandsCnt;
        while (hands_[hand].empty())
        {
            hand = (hand + 1) % HandsCnt;
        }
        return hand;
    }

    void table_to_hand(size_t hand_idx) {
        hands_[hand_idx].insert(table_.begin(), table_.end());
        table_.clear();
    }
    void table_to_garbage() {
        garbage_.insert(table_.begin(), table_.end());
        table_.clear();
    }
private:
    AttackAction make_attack_decision(size_t hand_idx) {
        return hand_decision_[hand_idx]->attack_step(game_state_);
    }
    DefendAction make_defend_decision(size_t hand_idx) {
        return hand_decision_[hand_idx]->defend_step(game_state_);
    }
    void decision_reset_game() {
        for (size_t hand_idx = 0; hand_idx < hand_decision_.size(); hand_idx++) {
            hand_decision_[hand_idx]->game_reset(game_state_);
        }
    }
private:
    void fire_stage_changing_event(Stage old_stage, Stage new_stage) {
        for (auto& stage_event : changing_stage_events_)
            stage_event->stage_changing(old_stage, new_stage);
    }
    void fire_attack_action_event(const AttackAction& action) {
        for (auto& event : step_events_)
            event->attack_action(action);
    }
    void fire_defend_action_event(const DefendAction& action) {
        for (auto& event : step_events_)
            event->defend_action(action);
    }
    void fire_append_action_event(const AttackAction& action) {
        for (auto& event : step_events_)
            event->append_action(action);
    }
    void fire_step_end_event(GameStepResult result) {
        for (auto& event : step_events_)
            event->step_end(result);
    }
private:
    int loser_hand_idx_;

    GameStateConstPtr<HandsCnt> game_state_;
    GameStep game_step_;

    cards_common::CardDeck deck_;
    cards_common::Card trump_card_;
    std::array<cards_common::CardSet, HandsCnt> hands_;
    cards_common::CardSet  garbage_;
    cards_common::CardList table_;

    std::array<GameHandDecisionPtr, HandsCnt> hand_decision_;
    std::list<GameChangingStageEventPtr> changing_stage_events_;
    std::list<GameStepEventPtr> step_events_;
};
  
template <size_t HandsCnt>
class GameRenderState
    {
    public:
        GameRenderState(const Game<HandsCnt> &game)
            : game_(game)
        {}

        size_t get_attack_hand() const { return game_.attack_hand_idx_; }
        size_t get_defend_hand() const { return game_.defend_hand_idx_; }

        cards_common::CardsSuit get_trump_suit() const { return game_.get_trump_suit(); }
        const cards_common::Card& get_trump_card() const { return game_.get_trump_card(); }
        bool is_deck_empty() const { return 0 == game_.get_deck_size(); }
        size_t get_deck_size() const { return game_.get_deck_size(); }

        const cards_common::CardSet& get_hand(int hand) const { return game_.hands_[hand]; }

        const cards_common::CardList& get_table() const { return game_.get_table(); }
    private:
        const Game<HandsCnt> &game_;
    };
};

#include "cards_renderer.hpp"

namespace durak_game {
template <typename RenderState>
class GameRenderer
{
public:
    GameRenderer(const std::string &background_img_path,
                 const std::string &cards_deck_img_path,
                 const std::string &cards_back_img_path,
                 const cv::Size &table_size)
        : renderer_(background_img_path, cards_deck_img_path, cards_back_img_path, table_size)
    {
    }
    virtual ~GameRenderer() {}
public:
    const cv::Mat &render(const RenderState &state)
    {
        renderer_.reset_cards();
        if (!state.is_deck_empty())
            draw_deck(state);

        draw_hands(state);
        draw_table(state);
        return renderer_.render();
    }
private:
    cards_renderer::TableRenderer renderer_;

    void draw_deck(const RenderState &state)
    {
        const cv::Size &table_sz = renderer_.get_table_size();
        const cv::Size &card_sz = renderer_.get_card_size();
        cv::Point temp;
        temp.x = (table_sz.width / 5 - card_sz.height) / 2;
        temp.y = table_sz.height / 2 - card_sz.width / 2;
        renderer_.add_card(temp,
            state.get_trump_card(),
            cards_renderer::rc90cw);

        if (1 < state.get_deck_size())
        {
            temp.y = table_sz.height / 2 - card_sz.height / 2;
            renderer_.add_card(temp,
                cards_common::Card(cards_common::CardsSuit::SuitNone, cards_common::CardsValue::ValueNone));
        }
    }
    void draw_hand(const cv::Rect &roi, const cards_common::CardSet &hand)
    {
        const int cards_cnt = (int)hand.size();

        const cv::Size &card_sz = renderer_.get_card_size();

        const int width_sm = roi.width - 2 * card_sz.width / 3;
        const int shift = (cards_cnt <= 1) 
                            ? 0 
                            : (width_sm - card_sz.width) / (std::max(6, cards_cnt) - 1);

        cv::Point pt = roi.tl() + cv::Point(card_sz.width / 3, card_sz.height / 6);
        for (auto item : hand)
        {
            renderer_.add_card(pt, item);
            pt.x += shift;
        }
    }
    void draw_hands(const RenderState &state)
    {
        const cv::Size &table_sz = renderer_.get_table_size();
        cv::Rect rc(table_sz.width / 5, 0, 4 * table_sz.width / 5, table_sz.height / 3);
        draw_hand(rc, state.get_hand(0));
        rc.y = 2 * table_sz.height / 3;
        draw_hand(rc, state.get_hand(1));
    }
    void draw_table (const RenderState &state)
    {
        const cv::Size &table_sz = renderer_.get_table_size();
        const cv::Size &card_sz = renderer_.get_card_size();
            
        cv::Point tl(table_sz.width / 5, 
                        table_sz.height / 3 + (table_sz.height / 3 - 6 * card_sz.height / 5));

        const cards_common::CardList &table = state.get_table();
        const int tbl_pair_cnt = ((int)table.size() + 1) / 2;
        const int width_sm = (4 * table_sz.width / 5) - 2 * card_sz.width / 3;
        const int shift = (tbl_pair_cnt <= 1)
            ? 0
            : (width_sm - card_sz.width) / (std::max(6, tbl_pair_cnt) - 1);

        cv::Point pt[2] = { tl, tl + cv::Point(card_sz.width / 2, card_sz.height / 5) };
        int delta = 0;
        for (auto item : table)
        {
            renderer_.add_card(pt[delta], item);
            delta = (delta + 1) % 2;
            if (0 == delta)
            {
                pt[0].x += shift;
                pt[1].x += shift;
            }
        }
    }
};
};