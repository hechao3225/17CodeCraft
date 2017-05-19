#include <iostream>
#include <vector>
#include <queue>
#include <set>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <sstream>
#include "time.h"

#include "deploy.h"
#include "graph.h"
#include "zkw.h"
#include "ga.h"
#include "solution.h"

int lastMinCost=INF;//最小费用
vector<vector<int> > lastMinPath;//最短路
Graph graph;

mt19937 mt(time(0));

//根据流量消耗查询所属服务器档次，返回档次ID
int findServerType(const Graph &graph,int flow)
{
    for(int i=0;i<graph.serverInfo.size();i++){
        if(flow<=graph.serverInfo[i].outCap)
            return i;
    }
    return -1;
}

//按格式输出接口
void outToFile(const vector<vector<int> > &lastMinPath,const map<int,int> &lastServers,char * filename)
{
    // 直接调用输出文件的方法输出到指定文件中(ps请注意格式的正确性，如果有解，第一行只有一个数据；第二行为空；第三行开始才是具体的数据，数据之间用一个空格分隔开)
    string res;
    res+=to_string(lastMinPath.size())+string("\n\n");
    size_t i,j;
    for(i=0;i<lastMinPath.size()-1;i++){
       for(j=0;j<lastMinPath[i].size()-1;j++){
           res+=to_string(lastMinPath[i][j])+string(" ");
       }
       int consumer=graph.netToConsumer[lastMinPath[i][j-1]];
       int flow=lastMinPath[i][j];
       int serverNode=lastMinPath[i][0];
       auto it=lastServers.find(serverNode);
       int serverID=it->second;
       res+=to_string(consumer)+string(" ");
       res+=to_string(flow)+string(" ");
       res+=to_string(serverID)+string("\n");
    }
    for(j=0;j<lastMinPath[i].size()-1;j++){
        res+=to_string(lastMinPath[i][j])+string(" ");
    }  
    int consumer=graph.netToConsumer[lastMinPath[i][j-1]];
    int flow=lastMinPath[i][j];
    int serverNode=lastMinPath[i][0];
    auto it=lastServers.find(serverNode);
    int serverID=it->second;
    res+=to_string(consumer)+string(" ");
    res+=to_string(flow)+string(" ");
    res+=to_string(serverID);
    //cout<<res;
    write_result(res.c_str(), filename);
}


//多源多汇最小费用流ZKW解法测试：确定服务器结点和位置，输出最小费用路
void test(const Graph &graph,int &lastMinCost)
{
    vector<int> consumerNetNodes,flowNeed;
    int flowNeedAll=0;
    for(int i=0;i<graph.consumerNum;i++){
        consumerNetNodes.push_back(graph.consumers[i].netNode);
        flowNeed.push_back(graph.consumers[i].flowNeed);
        flowNeedAll+=graph.consumers[i].flowNeed;
    }
    ZKW_MinCost zkw;
    if(graph.nodeNum>=noRecv)
        zkw.createZKW(graph,false);
    else
        zkw.createZKW(graph,true);
    pair<int,int> res;
    vector<int> servers={3,7,14,36,69,103,125,129,155};
    int maxCap=graph.serverInfo[graph.serverInfo.size()-1].outCap;
    if(graph.nodeNum>=noRecv)
        res=zkw.multiMinCostFlow(servers,consumerNetNodes,flowNeed,maxCap,graph.nodeNum,false);
    else
        res=zkw.multiMinCostFlow(servers,consumerNetNodes,flowNeed,maxCap,graph.nodeNum,true);
    if(res.second==flowNeedAll){
        int minCost=res.first;
        int serverCost=0,layoutCost=0;
        int edgeSize=zkw.zkwEdges.size();
        for(int i=2,j=1;j<=servers.size();i+=2,j++){
            int curServerNode=servers[servers.size()-j];
            int curFlow=zkw.zkwEdges[edgeSize-i].flow;
            int serverID;
            serverID=findServerType(graph,curFlow);//根据流量消耗判断服务器档次
            serverCost+=graph.serverInfo[serverID].cost;//加上该档次服务器的成本
            layoutCost+=graph.netCost[curServerNode];//加上服务器所在结点的部署成本
        }
        cout<<"server cost:"<<serverCost<<endl;
        cout<<"layout cost:"<<layoutCost<<endl;
        minCost+=serverCost;
        minCost+=layoutCost;
        lastMinCost=minCost;//基本适应度为minCost
    }
    else{
        cout<<"min path not found!"<<endl;
    }
}


//根据fitness比较
bool cmpByFitness(const Individual &someone1,const Individual &someone2){//升序
    return someone1.fitness<someone2.fitness;
}

//主任务入口
void deploy_server(char * topo[MAX_EDGE_NUM], int line_num,char * filename)
{

    graph.createGraph(topo);
    //test(graph,lastMinCost);
    //cout<<"test result:"<<lastMinCost<<endl;
    clock_t start=clock();
    unsigned int randSeed=time(NULL);

    srand(randSeed);
    cout<<randSeed<<endl;

    Individual lastBest;
    map<int,int> servers;

    ATCGA ga(graph);
    ga.evolve(graph);
    ga.getBestIndividual(lastBest);
    cout<<"lastBest:"<<lastBest.fitness<<endl;
    ga.getResult(lastBest,graph,lastMinCost,lastMinPath,servers);

    outToFile(lastMinPath,servers,filename);
    clock_t finish=clock();
    double time = (double)(finish-start)/CLOCKS_PER_SEC*1000;
    printf("all used time:%.1f ms\n",time);

}
