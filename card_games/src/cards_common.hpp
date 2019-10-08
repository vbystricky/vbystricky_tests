#pragma once

#include <deque>
#include <set>
#include <list>
#include <iostream>
#include <algorithm>
#include <random>
#include <string>

namespace cards_common
{
//save suits consecutive, we will iterate throught in some case
enum class CardsSuit
{
    SuitNone,
    Spades,
    Clubs,
    Diamonds,
    Hearts
};

//save values consecutive, we will iterate throught in some case
enum class CardsValue
{
    ValueNone,
    Deuce,
    Trey,
    Four,
    Five,
    Six,
    Seven,
    Eight,
    Nine,
    Ten,
    Jack,
    Queen,
    King,
    Ace,
    ValueMax,
};

enum class CardDeckType
{
    CardDeck32,
    CardDeck36,
    CardDeck52
};

size_t get_deck_size(CardDeckType deck_type) {
    switch (deck_type) {
    case CardDeckType::CardDeck32:
        return 32;
    case CardDeckType::CardDeck36:
        return 36;
    case CardDeckType::CardDeck52:
        return 52;
    }
    throw std::exception("Unknown card deck type");
}

struct Card
{
    Card()
        : suit_(CardsSuit::SuitNone)
        , value_(CardsValue::ValueNone)
    {
    }
    Card(const CardsSuit &suit, const CardsValue &value)
        : suit_(suit)
        , value_(value)
    {
    }
    CardsSuit suit_;
    CardsValue value_;

    friend bool operator==(const Card& l, const Card& r)
    {
        return ((l.suit_ == r.suit_) && (l.value_ == r.value_));
    }
    friend bool operator<(const Card& l, const Card& r)
    {
        return ((l.suit_ < r.suit_) ||
                (l.suit_ == r.suit_) && (l.value_ < r.value_));
    }
};

bool is_less(const Card& l, const Card& r, CardsSuit trump_suit)
{
    if (l.suit_ != trump_suit && r.suit_ == trump_suit)
        return true;
    if (l.suit_ == trump_suit && r.suit_ != trump_suit)
        return false;
    return (l.value_ < r.value_);
}

class CardDeck
{
    friend std::ostream &operator << (std::ostream &os, const CardDeck &card);
public:
    CardDeck() {}
    CardDeck(CardDeckType type)
    {
        fill(type);
    }
    virtual ~CardDeck() {}

    friend bool operator==(const CardDeck& l, const CardDeck& r)
    {
        return (l.storage_ == r.storage_);
    }
public:
    bool empty() const
    {
        return storage_.empty();
    }

    Card pop_front()
    {
        Card ret = std::move(storage_.front());
        storage_.pop_front();
        return ret;
    }
    void push_back(Card &&card)
    {
        storage_.push_back(card);
    }
    void push_back(const Card &card)
    {
        storage_.push_back(card);
    }

    const Card &back() const
    {
        return storage_.back();
    }
    size_t size() const
    {
        return storage_.size();
    }

    void shuffle(unsigned int seed)
    {
        std::shuffle(storage_.begin(), storage_.end(), std::default_random_engine(seed));
    }
    void shuffle_wo_last(unsigned int seed) {
        std::shuffle(storage_.begin(), storage_.end() - 1, std::default_random_engine(seed));
    }

    friend std::ostream &operator << (std::ostream &os, const CardDeck &card);

    template<class _Iter>
    void append(_Iter _First, _Iter _Last) {
        for (; _First != _Last; ++_First)
            storage_.emplace_back(*_First);
    }
    void append(const CardDeck& deck) {
        append(deck.storage_.cbegin(), deck.storage_.cend());
    }
    void fill(CardDeckType type)
    {
        CardsValue first = CardsValue::Deuce;
        CardsValue last = CardsValue::Ace;
        switch (type)
        {
        case CardDeckType::CardDeck32:
            first = CardsValue::Seven;
            last = CardsValue::Ace;
            break;
        case CardDeckType::CardDeck36:
            first = CardsValue::Six;
            last = CardsValue::Ace;
            break;
        case CardDeckType::CardDeck52:
            first = CardsValue::Deuce;
            last = CardsValue::Ace;
            break;
        default:
            throw std::runtime_error("Invalid card deck type");
        };

        for (int suit = (int)CardsSuit::Spades; suit <= (int)CardsSuit::Hearts; ++suit)
        {
            for (int value = (int)first; value <= (int)last; ++value)
            {
                Card card((CardsSuit)suit, (CardsValue)value);
                storage_.push_back(card);
            }
        }
    }
    void clear()
    {
        storage_.clear();
    }
private:
    std::deque<Card> storage_;
};

using PairCard = std::pair<cards_common::Card, cards_common::Card>;
using CardList = std::list< cards_common::Card >;

class CardSet
    : public std::set<Card>
{
public:
    CardSet() {}
    virtual ~CardSet() {}
public:
    bool has_suite(CardsSuit suit) const
    { 
        for (auto card : (*this))
        {
            if (suit == card.suit_)
                return true;
        }
        return false;
    }
    bool has_value(CardsValue value) const
    {
        for (auto card : (*this))
        {
            if (value == card.value_)
                return true;
        }
        return false;
    }
};

std::ostream &operator << (std::ostream &os, const Card &card)
{
    switch (card.value_)
    {
    case CardsValue::Deuce:
        os << '2';
        break;
    case CardsValue::Trey:
        os << '3';
        break;
    case CardsValue::Four:
        os << '4';
        break;
    case CardsValue::Five:
        os << '5';
        break;
    case CardsValue::Six:
        os << '6';
        break;
    case CardsValue::Seven:
        os << '7';
        break;
    case CardsValue::Eight:
        os << '8';
        break;
    case CardsValue::Nine:
        os << '9';
        break;
    case CardsValue::Ten:
        os << "10";
        break;
    case CardsValue::Jack:
        os << 'J';
        break;
    case CardsValue::Queen:
        os << 'Q';
        break;
    case CardsValue::King:
        os << 'K';
        break;
    case CardsValue::Ace:
        os << 'A';
        break;
    default:
        throw std::runtime_error("Invalid value of card");
    }
    switch (card.suit_)
    {
    case CardsSuit::Spades:
        os << 'S';
        break;
    case CardsSuit::Clubs:
        os << 'C';
        break;
    case CardsSuit::Diamonds:
        os << 'D';
        break;
    case CardsSuit::Hearts:
        os << 'H';
        break;
    default:
        throw std::runtime_error("Invalid suit of card");
    }
    return os;
}
std::ostream &operator << (std::ostream &os, const CardDeck &deck)
{
    for (auto item : deck.storage_)
    {
        os << item << ", ";
    }
    return os;
}
std::ostream &operator << (std::ostream &os, const CardSet &set)
{
    for (auto item : set)
    {
        os << item << ", ";
    }
    return os;
}
};