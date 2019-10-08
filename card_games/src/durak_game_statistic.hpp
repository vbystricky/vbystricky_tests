#pragma once
#include "durak_game.hpp"

#include <iostream>
#include <iomanip>
#include <thread>
#include <future>

// GameStatistic
namespace durak_game {
struct GameStatistic {
    size_t first_decision_win = 0;
    size_t second_decision_win = 0;
    size_t draw = 0;
    size_t games_count() const {
        return first_decision_win + second_decision_win + draw;
    }
};
struct FullStatistic {
    std::string first_decision_name;
    std::string second_decision_name;

    GameStatistic first_decision_start;
    GameStatistic second_decision_start;

    size_t first_decision_win() const {
        return first_decision_start.first_decision_win + second_decision_start.first_decision_win;
    }
    size_t second_decision_win() const {
        return first_decision_start.second_decision_win + second_decision_start.second_decision_win;
    }
    size_t draw() const {
        return first_decision_start.draw + second_decision_start.draw;
    }

    size_t start_decision_win() const {
        return first_decision_start.first_decision_win + second_decision_start.second_decision_win;
    }
    size_t not_start_decision_win() const {
        return first_decision_start.second_decision_win + second_decision_start.first_decision_win;
    }
    size_t games_count() const {
        return first_decision_start.games_count() + second_decision_start.games_count();
    }

    friend std::ostream& operator<< (std::ostream& stream, const FullStatistic& statistic) {
        stream
            << statistic.first_decision_name
            << " vs "
            << statistic.second_decision_name
            << std::endl;
        /////////////////////////////////////////////////////////////
        {
            stream
                << "Full statistic: " << std::endl;
            stream
                << "  first decision win:     "
                << std::setw(7) << statistic.first_decision_win()
                << " ("
                << std::setprecision(4) << 100. * statistic.first_decision_win() / statistic.games_count()
                << "%)"
                << std::endl;
            stream
                << "  second decision win:    "
                << std::setw(7) << statistic.second_decision_win()
                << " ("
                << 100. * statistic.second_decision_win() / statistic.games_count()
                << "%)"
                << std::endl;
            stream
                << "  draw:                   "
                << std::setw(7) << statistic.draw()
                << " ("
                << 100. * statistic.draw() / statistic.games_count()
                << "%)"
                << std::endl;
        }
        /////////////////////////////////////////////////////////////
        if (statistic.first_decision_name != statistic.second_decision_name) {
            {
                stream
                    << "First decision start statistic: " << std::endl;
                stream
                    << "  first decision win:     "
                    << std::setw(7) << statistic.first_decision_start.first_decision_win
                    << " ("
                    << 100. * statistic.first_decision_start.first_decision_win / statistic.first_decision_start.games_count()
                    << "%)"
                    << std::endl;
                stream
                    << "  second decision win:    "
                    << std::setw(7) << statistic.first_decision_start.second_decision_win
                    << " ("
                    << 100. * statistic.first_decision_start.second_decision_win / statistic.first_decision_start.games_count()
                    << "%)"
                    << std::endl;
                stream
                    << "  draw:                   "
                    << std::setw(7) << statistic.first_decision_start.draw
                    << " ("
                    << 100. * statistic.first_decision_start.draw / statistic.first_decision_start.games_count()
                    << "%)"
                    << std::endl;
            }
            /////////////////////////////////////////////////////////////
            {
                stream
                    << "Second decision start statistic: " << std::endl;
                stream
                    << "  first decision win:     "
                    << std::setw(7) << statistic.second_decision_start.first_decision_win
                    << " ("
                    << 100. * statistic.second_decision_start.first_decision_win / statistic.second_decision_start.games_count()
                    << "%)"
                    << std::endl;
                stream
                    << "  second decision win:    "
                    << std::setw(7) << statistic.second_decision_start.second_decision_win
                    << " ("
                    << 100. * statistic.second_decision_start.second_decision_win / statistic.second_decision_start.games_count()
                    << "%)"
                    << std::endl;
                stream
                    << "  draw:                   "
                    << std::setw(7) << statistic.second_decision_start.draw
                    << " ("
                    << 100. * statistic.second_decision_start.draw / statistic.second_decision_start.games_count()
                    << "%)"
                    << std::endl;
            }
        }
        else {
            stream
                << "With start statistic: " << std::endl;
            stream
                << "  start decision win:     "
                << std::setw(7) << statistic.start_decision_win()
                << " ("
                << 100. * statistic.start_decision_win() / statistic.games_count()
                << "%)"
                << std::endl;
            stream
                << "  not start decision win: "
                << std::setw(7) << statistic.not_start_decision_win()
                << " ("
                << 100. * statistic.not_start_decision_win() / statistic.games_count()
                << "%)"
                << std::endl;
            stream
                << "  draw:                   "
                << std::setw(7) << statistic.draw()
                << " ("
                << 100. * statistic.draw() / statistic.games_count()
                << "%)"
                << std::endl;
        }
        return stream;
    }
};

int run_game(Game<2>* pgame, int start_hand_idx, unsigned int seed) {
    pgame->init(start_hand_idx, seed);
    return pgame->run();
}

template <class GameHandDecisionFirst, class GameHandDecisionSecond>
class GameStatistician
{
public:
    GameStatistician()
    {
        game_first_.set_hand_decision(0, std::unique_ptr< GameHandDecision<2> >(new GameHandDecisionFirst()));
        game_first_.set_hand_decision(1, std::unique_ptr< GameHandDecision<2> >(new GameHandDecisionSecond()));

        game_second_.set_hand_decision(0, std::unique_ptr< GameHandDecision<2> >(new GameHandDecisionSecond()));
        game_second_.set_hand_decision(1, std::unique_ptr< GameHandDecision<2> >(new GameHandDecisionFirst()));
    }

    FullStatistic run(int test_steps_cnt, unsigned int seed = -1) {
        if (-1 == seed)
            seed = (unsigned int)time(0);
        std::mt19937 generator(seed);
        FullStatistic result_stat;
        result_stat.first_decision_name = GameHandDecisionFirst::decision_name();
        result_stat.second_decision_name = GameHandDecisionSecond::decision_name();
        std::string clear;
        for (int i = 0; i < test_steps_cnt; i++) {
            unsigned int game_seed = generator();
            //std::cout << game_seed << std::endl;
            {
                auto future_first = std::async(run_game, &game_first_, 0, game_seed);
                auto future_second = std::async(run_game, &game_second_, 0, game_seed);
                switch (future_first.get()) {
                case 0:
                    result_stat.first_decision_start.second_decision_win++;
                    break;
                case 1:
                    result_stat.first_decision_start.first_decision_win++;
                    break;
                case -1:
                    result_stat.first_decision_start.draw++;
                    break;
                };
                switch (future_second.get()) {
                case 0:
                    result_stat.second_decision_start.first_decision_win++;
                    break;
                case 1:
                    result_stat.second_decision_start.second_decision_win++;
                    break;
                case -1:
                    result_stat.second_decision_start.draw++;
                    break;
                };
            }
            //std::cout << "---" << std::endl;
            {
                auto future_first = std::async(run_game, &game_first_, 1, game_seed);
                auto future_second = std::async(run_game, &game_second_, 1, game_seed);
                switch (future_first.get()) {
                case 0:
                    result_stat.second_decision_start.second_decision_win++;
                    break;
                case 1:
                    result_stat.second_decision_start.first_decision_win++;
                    break;
                case -1:
                    result_stat.second_decision_start.draw++;
                    break;
                };
                switch (future_second.get()) {
                case 0:
                    result_stat.first_decision_start.first_decision_win++;
                    break;
                case 1:
                    result_stat.first_decision_start.second_decision_win++;
                    break;
                case -1:
                    result_stat.first_decision_start.draw++;
                    break;
                };
            }
            if (i * 50 > (int)clear.size() * test_steps_cnt) {
                std::cout << "."; std::cout.flush();
                clear += " ";
            }
        }
        std::cout << "\r" << clear << "\r";
        return result_stat;
    }
private:
    Game<2> game_first_;
    Game<2> game_second_;
};

template <int TestCount, typename GameHandDecisionFirst, typename GameHandDecisionSecond>
void calc_statistic() {
    GameStatistician<GameHandDecisionFirst, GameHandDecisionSecond> statistician;
    //std::cout << 
    std::cout << statistician.run(TestCount) << std::endl;
}

template <int TestCount, typename GameHandDecisionFirst, typename ... GameHandDecisionTypes>
void calc_one_to_many_decision_statistic() {
    calc_statistic<TestCount, GameHandDecisionFirst, GameHandDecisionFirst>();
    (calc_statistic<TestCount, GameHandDecisionFirst, GameHandDecisionTypes>(), ...);
}

template <int TestCount>
void calc_multi_decision_statistic() {}

template <int TestCount, typename GameHandDecisionFirst, typename ... GameHandDecisionTypes>
void calc_multi_decision_statistic() {
    calc_one_to_many_decision_statistic<TestCount, GameHandDecisionFirst, GameHandDecisionTypes...>();
    calc_multi_decision_statistic<TestCount, GameHandDecisionTypes...>();
}
} //namespace durak_game