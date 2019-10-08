#include "cards_common.hpp"
#include "cards_renderer.hpp"
#include "durak_game.hpp"
#include "durak_game_decision_base.hpp"
#include "durak_game_decision_less_card.hpp"
#include "durak_game_statistic.hpp"

#include <windows.h>
#include <opencv2/highgui.hpp>

#include <time.h>
#include <iostream>
#include <chrono>
#include <thread>

using namespace durak_game;

// GameVisualizer
namespace {
class GameVisualizer
{
    class GameVisualizerStageEvent
        : public GameChangingStageEvent
    {
    public:
        GameVisualizerStageEvent(GameVisualizer &owner)
            : owner_(owner)
        {
        }
        void stage_changing(Stage old_stage, Stage new_stage) override {
            if (Stage::NoneStage != new_stage)
                owner_.render();
        }
    private:
        GameVisualizer &owner_;
    };
public:
    GameVisualizer()
        : durak_renderer_(
            "",
            "../res/cards_deck_sm.png",
            "../res/back_sm.png",
            cv::Size(1000, 600)
        )
    {}
    ~GameVisualizer() {}

    void set_hand_decision(size_t hand_idx, std::unique_ptr< GameHandDecision<2> > decision)
    {
        game_.set_hand_decision(hand_idx, std::move(decision));
    }
    void run(int start_hand_idx = -1, unsigned int seed = (int)time(0))
    {
        std::cout << std::endl;
        std::cout << "Start hand: " << start_hand_idx << std::endl;
        std::cout << "Seed: " << seed << std::endl;
        game_.add_stage_changing_event(std::unique_ptr< GameChangingStageEvent>(new GameVisualizerStageEvent(*this)));
        game_.init(start_hand_idx, seed);
        int lose_hand = game_.run();
        std::cout << "Lose hand: " << lose_hand << std::endl;
        cv::destroyWindow("table");
    }
private:
    GameRenderer<GameRenderState<2>> durak_renderer_;
    Game<2> game_;

    void render()
    {
        GameRenderState<2> game_state(game_);
        const cv::Mat &table_img = durak_renderer_.render(game_state);
        cv::imshow("table", table_img);
        cv::waitKey(100);
    }
};
}// namespace

int main(int argc, const char **argv) {
    GameVisualizer visualizer;
    visualizer.set_hand_decision(1, std::unique_ptr< GameHandDecision<2> >(new GameHandDecisionAttackDefendLessCard<2>()));
    visualizer.set_hand_decision(0, std::unique_ptr< GameHandDecision<2> >(new GameHandDecisionAttackDefendLessCard<2>()));
    visualizer.run();

    calc_multi_decision_statistic<
        20000,
        GameHandDecisionRandom<2>,
        GameHandDecisionAttackDefendLessCard<2>
    >();
    return 0;
}
