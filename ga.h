#ifndef GA_H
#define GA_H

#include <vector>
#include <string>

#include "graph.h"

using namespace std;

struct Individual{
    vector<int> gene;//基因长度固定为图网络结点数
    double fitness;//值越小适应度越高
    double selectRate;
};

class ATCGA//自适应两代遗传算法
{
public:
    friend class Graph;

    ATCGA(const Graph &graph):geneLen(graph.nodeNum),crossOverRate(1.0),mutationRate(0.8){}

    //获取当前交叉概率
    double getCrossOverRate(){return crossOverRate;}
    //获取当前变异概率
    double getMutationRate(){return mutationRate;}
    //获取当代种群
    void getCurPopulation(vector<Individual> &population){
        population=curPopulation;
    }
    //获取父代种群
    void getPrePopulation(vector<Individual> &population){
        population=prePopulation;
    }
    //获取最优个体
    void getBestIndividual(Individual &individual){
        individual=bestIndividual;
    }
    //根据流量消耗查询所属服务器档次，返回档次ID
    int findServerType(const Graph &graph, int flow);
    //计算个体的适应度并写入个体
    bool calFitness(const Graph &graph,const vector<int> &consumerNetNodes,vector<int> &flowNeed,
                     int flowNeedAll,Individual &someone);

    //计算当代种群的平均适应度
    double calCurAvgFitness();
    //计算当代种群的最优适应度
    double calCurBestFitness();
    //计算当代种群的最差适应度
    double calWorstFitness();

    //计算当代种群的适应度分布值
    double calFitnessDistribution();
    //计算当代种群与父代种群的代沟值
    double calGenerationGap();

    //生成初始种群
    double createFirstPopulation(const Graph &graph,int randIndividualNum);

    //选择方案1:竞争选择（两代竞争算法）
    void selectByCompete(vector<pair<Individual,Individual> > &parents,int parentsNum);
    //选择方案2：轮盘赌选择（两代比例选择算法）
    void selectByProportion(vector<pair<Individual,Individual> > &parents,int parentsNum);
    //选择方案3:竞争选择（每次只选一个）
    void selectOnlyOneByCompete(Individual &someone);
    //选择方案4:轮盘赌选择（每次只选一个）
    void selectOnlyOneByProportion(Individual &someone);

    //交叉方案1：单点交叉
    void crossOverByOneBit(const pair<Individual,Individual> &parent,pair<Individual,Individual> &children);
    //交叉方案2:随机多点交叉
    void crossOverByRandBits(const pair<Individual,Individual> &parent,pair<Individual,Individual> &children);
    //交叉方案3:自适应多点交叉，先计算当前迭代次数下适应度分布，后确定交叉的位数
    void crossOverByAutoBits(const pair<Individual,Individual> &parent,pair<Individual,Individual> &children);
    //交叉方案4:每次与固定个体交叉
    void crossOverWithFixed(const Individual &someone, pair<Individual, Individual> &children, bool crossFlag);

    //变异方案1：单点变异
    void mutateByOneBit(Individual &someone);
    //变异方案2:自适应单点变异，先计算当前迭代次数下代沟值，后确定变异的概率
    void mutateByAutoBits(Individual &someone);
    //追加变异：超级变异
    void superMutation(Individual &someone);

    //种群更新:每次种群变化时调用此函数更新数据成员
    void updatePopulation();
    //演化：迭代times次，将结果写入最佳个体（服务器布局）
    double evolve(const Graph &graph);

    //求解：演化完成后取出最佳个体bestIndividual计算结果(返回lastMinCost以及lastMinPath)
    void getResult(const Individual &bestIndividual, const Graph &graph,
                   int &lastMinCost, vector<vector<int> > &lastMinPath,
                   map<int, int> &lastServers);

private:
    int geneLen;//基因长度

    const int popsize=3;//种群规模
    const int times=100000;//演化代数
    const int bigMaxTime=88;
    const int middleMaxTime=88;
    const int smallMaxTime=28;
    const int superMutationTimes=200;//迭代较长次数后仍未出更优则发生超级变异
    const int disasterTimes=1000;//迭代更长次数后仍未出更优发生灾变
    const int smallQuitTimes=8000;//N次未出更优，提前退出，初级跑8000次
    const int middleQuitTimes=50000;//N次未出更优，提前退出，中级跑30000次
    const int maxServerType=5;

    vector<Individual> curPopulation;//当代种群
    vector<Individual> prePopulation;//父代种群
    double curAvgFitness;//当代种群的平均适应度
    double curBestFitness;//当代种群的最优适应度
    double curWorstFitness;//当代最差适应度

    double curFitnessDist;//当代适应度分布值
    vector<double> fitnessDists;//演化N代后分布值记录
    double curGenerationGap;//当代与父代代沟
    vector<double> generationGaps;//演化N代后两代代沟记录

    double crossOverRate;//全交叉
    double mutationRate;//自适应变异率
    Individual firstIndividual;//初始解
    Individual bestIndividual;//保存最佳个体

};

#endif // GA_H
