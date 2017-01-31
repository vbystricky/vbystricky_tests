#include <opencv2/core/core.hpp>

#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <random>
#include <algorithm>

static const size_t arms_cnt = 10;
static const size_t run_steps_cnt = 1000;
static const size_t runs_cnt = 2000;
static const double epsilons[11] = { 0., 2.0, 0.5, 0.25, 0.1, 0.05, 0.025, 0.01, 0.005, 0.0025, 0.001};


typedef std::default_random_engine              RandomGen;
typedef std::uniform_real_distribution<double>  UniformReal;
typedef std::uniform_int_distribution<int>      UniformInt;
typedef std::normal_distribution<double>        NormalReal;

template <class T, class _Pr>
static size_t argmax(const std::vector<T> &vec, _Pr comp)
{
    if (0 == vec.size())
        return -1;
    size_t idx_max = 0;
    for (size_t i = 1; i < vec.size(); i++)
    {
        if (comp(vec[idx_max], vec[i]))
            idx_max = i;
    }
    return idx_max;
}

class MultiArmsBanditModel
{
public:
    MultiArmsBanditModel(const std::vector<NormalReal::param_type> &param_rewards, unsigned seed = 1)
        : m_generator(seed)
        , m_arms_cnt(param_rewards.size())
    {
        for (size_t i = 0; i < m_arms_cnt; i++)
        {
            m_rng.push_back(NormalReal(param_rewards[i]));
        }
    }

    virtual double get_reward(size_t arms_idx, size_t step)
    {
        return m_rng[arms_idx](m_generator);
    }
protected:
    size_t m_arms_cnt;
private:
    RandomGen m_generator;
    std::vector<NormalReal> m_rng;
};

class MultiArmsBanditModelPrecalc
    : public MultiArmsBanditModel
{
public:
    MultiArmsBanditModelPrecalc(const std::vector<NormalReal::param_type> &param_rewards, size_t steps_cnt, unsigned seed = 1)
        : MultiArmsBanditModel(param_rewards, seed)
        , m_steps_cnt(steps_cnt)
    {
    }

    virtual double get_reward(size_t arms_idx, size_t step)
    {
        if (m_rewards.empty())
            precalcRewards();
        return m_rewards.at<double>((int)step, (int)arms_idx);
    }
private:
    size_t m_steps_cnt;
    cv::Mat m_rewards;
    void precalcRewards()
    {
        m_rewards.create((int)m_steps_cnt, (int)m_arms_cnt, CV_64FC1);
        for (size_t step = 0; step < m_steps_cnt; step++)
        {
            double *ptr = m_rewards.ptr<double>((int)step);
            for (size_t arm = 0; arm < m_arms_cnt; arm++)
                ptr[arm] = MultiArmsBanditModel::get_reward(arm, step);
        }
    }
};

class MultiArmsBanditEpsGreedyStrategy
{
    typedef std::pair<size_t, double>    AvgRevardsItem;
    typedef std::vector<AvgRevardsItem>  AvgRevards;
public:
    MultiArmsBanditEpsGreedyStrategy(size_t arms_cnt, double epsilon = 0., int seed = 1)
        : m_arms_cnt(arms_cnt)
        , m_epsilon(epsilon)
        , m_total_reward(0.)
        , m_generator(seed)
        , m_rng_select(0.0, 1.0)
        , m_rng_arm(0, (int)arms_cnt - 1)
    {
        start();
    }

    void start()
    {
        m_avg_rewards.clear();
        m_avg_rewards.resize(arms_cnt, std::make_pair<size_t, double>(0, 0.));
        m_total_reward = 0.;
    }

    size_t getNextStepArm()
    {
        size_t arm = 0;
        if ((DBL_EPSILON > m_epsilon) || (m_rng_select(m_generator) > m_epsilon))
        {
            arm = argmax(m_avg_rewards, [](const AvgRevardsItem &item1, const AvgRevardsItem &item2)
                                          { return item1.second < item2.second; });
        }
        else
        {
            arm = m_rng_arm(m_generator);
        }
        return arm;
    }
    void updateReward(size_t arm, double reward)
    {
        m_total_reward += reward;
        m_avg_rewards[arm].first++;
        m_avg_rewards[arm].second += (reward - m_avg_rewards[arm].second) / (double)m_avg_rewards[arm].first;
    }
private:
    size_t m_arms_cnt;
    double m_epsilon;
    double m_total_reward;

    RandomGen m_generator;
    UniformReal m_rng_select;
    UniformInt m_rng_arm;

    AvgRevards m_avg_rewards;
};


int main(int argc, char **argv)
{
    std::vector<NormalReal::param_type> param_rewards;
    {
        RandomGen generator(3);
        NormalReal rng(0.0, 1.0);

        double mean_mean = 0.;
        for (size_t arm = 0; arm < arms_cnt; arm++)
        {
            double mean = rng(generator);
            param_rewards.push_back(NormalReal::param_type(mean, 1.0));

            std::cout << mean << std::endl;

            mean_mean += mean;
        }
        std::cout << std::endl << mean_mean / arms_cnt << std::endl;
    }

    cv::Mat avg_step_rewards = cv::Mat::zeros(run_steps_cnt, 10 + 1, CV_64FC1);
    for (int run = 0; run < runs_cnt; run++)
    {
        MultiArmsBanditModelPrecalc model(param_rewards, run_steps_cnt, run);//use run index as seed for model

        std::cout << "run: " << run << "\r"; std::cout.flush();

        for (int a = 0; a < sizeof(epsilons) / sizeof(double); a++)
        {
            const double epsilon = epsilons[a];

            MultiArmsBanditEpsGreedyStrategy strategy(arms_cnt, epsilon, a + run + 1);
            for (size_t i = 0; i < run_steps_cnt; i++)
            {
                size_t arm = strategy.getNextStepArm();

                double reward = model.get_reward(arm, i);
                strategy.updateReward(arm, reward);

                avg_step_rewards.at<double>((int)i, a) += reward;
            }
        }
    }

    std::ofstream ofs("./rewards.dat");
    for (int i = 0; i < avg_step_rewards.rows; i++)
    {
        ofs << i << " ";
        double *ptr = avg_step_rewards.ptr<double>(i);
        for (int a = 0; a < 11; a++)
        {
            ofs << ptr[a] / runs_cnt << " ";
        }
        ofs << std::endl;
    }
}
