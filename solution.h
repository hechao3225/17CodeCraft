#ifndef SOLUTION_H
#define SOLUTION_H

#include <vector>
#include <string>
#include <queue>
#include <set>

#include "graph.h"

using namespace std;

struct Result{
    vector<bool> netNodes;//网络结点：true---服务器结点 false---非服务器结点
    int minCost;

};

class CostCmp{
public:
    bool operator()(const Result &res1,const Result &res2){
        return res1.minCost>res2.minCost;//cost小的优先级高
    }
};

class Solution
{
public:

    void getBestRes(Result &res){
        res=bestRes;
    }

    void init(const Graph &graph, const Result &firstResult, double leftTime);
    void init(const Graph &graph);
    int findServerType(const Graph &graph,int flow);
    bool calMinCost(const Graph &graph, const vector<int> &consumerNetNodes, vector<int> &flowNeed,
                    int flowNeedAll, Result &result);
    void exchangeServer(const Result &motherRes, Result &res, const Graph &graph);
    void increaseServer(const Result &motherRes,Result &res);
    void deleteServer(const Graph &graph, const Result &motherRes, Result &res);
    void solve(const Graph &graph);
    void getResult(const Result &bestRes,const Graph &graph,
                   int &lastMinCost,vector<vector<int> > &lastMinPath,
                   map<int,int> &lastServers);

private:

    const int times=1000;//迭代代数
    int bigMaxTime=87;
    int middleMaxTime=87;
    const int smallMaxTime=87;
    const int smallQuitTimes=8000;

    priority_queue<Result,vector<Result>,CostCmp> serverLocations;
    set<Result> newResults;//去重
    Result bestRes;

};

#endif // SOLUTION_H
